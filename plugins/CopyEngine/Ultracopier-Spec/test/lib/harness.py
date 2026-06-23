#!/usr/bin/env python3
"""Core test harness for the Ultracopier-Spec CopyEngine.

DESIGN PRINCIPLE (see ../CLAUDE.md): we NEVER modify ultracopier's C++ to make it
testable. We drive the REAL shipping binary entirely from the outside:

  * options & collision/user-actions  -> written into an ISOLATED config file (no dialog)
  * filesystem conditions (errors...)  -> an LD_PRELOAD shim (Linux) / hook DLL (Windows)
  * the resident process               -> always killed by an EXTERNAL command
  * correctness                        -> diff of real folders + valgrind/ASan + a memory cap

A single run() call is the whole contract; test cases in ../cases/ just call it with
different parameters and assert on the returned Result.
"""
from __future__ import annotations
import os, sys, time, shutil, signal, subprocess, configparser, pathlib, dataclasses, tempfile, threading

SPEC_DIR = pathlib.Path(__file__).resolve().parents[2]      # .../CopyEngine/Ultracopier-Spec
SOURCES  = pathlib.Path(__file__).resolve().parents[5]      # .../sources
TEST_DIR = SPEC_DIR / "test"
MXE_QMAKE = "/mnt/data/perso/progs/mxe/x86_64/usr/x86_64-w64-mingw32.shared/qt6/bin/qmake"

# ---- backends ---------------------------------------------------------------
ASYNC, IO_URING, IOCP = "async", "io_uring", "iocp"

# ---- collision policy = the QComboBox INDEX written to the conf (see ../CLAUDE.md) ----
class FileCollision:
    ASK = 0; SKIP = 1; OVERWRITE = 2
    OVERWRITE_IF_NOT_SAME_SIZE_AND_DATE = 3   # "override only if never" (never touch identical)
    OVERWRITE_IF_NEWER = 4; OVERWRITE_IF_OLDER = 5; RENAME = 6
    OVERWRITE_IF_NOT_SAME_SIZE = 7; OVERWRITE_IF_MDATE_DIFFERS = 8
class FolderCollision:
    ASK = 0; MERGE = 1; SKIP = 2; RENAME = 3
class FileError:
    ASK = 0; SKIP = 1; PUT_TO_END = 2
class FolderError:
    ASK = 0; SKIP = 1

# ---- memory checkers --------------------------------------------------------
NONE, VALGRIND, SANITIZE = "none", "valgrind", "sanitize"

# ---- computed memory budget (NOT a fixed 1 GB) ------------------------------
BUFFER_BUDGET_MB = 256        # I/O buffers must stay under this ceiling (hard rule)
MEM_MARGIN = 1.20             # +20% error margin on the computed cap
STAY_ALIVE_SECONDS = 10       # the copy must outlive completion this long (no teardown crash)
COPY_TIMEOUT_SECONDS = 180    # hung-copy guard: a small-data test that runs longer is HUNG
                              # (e.g. infinite error-retry, or stalled on a dialog) -> FAIL fast
                              # with evidence, never let one stuck case hang the whole suite.


@dataclasses.dataclass
class Result:
    backend: str
    completed: bool          # copy reached idle within the timeout
    stayed_alive: bool       # still running after stay_alive_seconds (did NOT crash on teardown)
    content_ok: bool         # diff -rq --no-dereference == 0 (when an expectation was given)
    peak_rss_mb: int
    oom_killed: bool         # exceeded the memory cap
    mem_errors: int          # valgrind/ASan reported errors in our code
    exit_code: int | None    # process exit code if it died on its own (None if we killed it)
    diff_text: str
    notes: str = ""
    hard_capped: bool = False   # process was run under a systemd-run cgroup memory cap
    @property
    def ok(self) -> bool:
        return (self.completed and self.stayed_alive and self.content_ok
                and not self.oom_killed and self.mem_errors == 0)


def real_config_path() -> str:
    """The PRIVATE test config lives OUTSIDE the repo so secret SOURCE paths can never
    reach git: ~/.config/ultracopier-spec-test.ini (honors $XDG_CONFIG_HOME)."""
    base = os.environ.get("XDG_CONFIG_HOME") or os.path.expanduser("~/.config")
    return os.path.join(base, "ultracopier-spec-test.ini")


_INSTALL_MSG = """\
==============================================================================
 Ultracopier-Spec test config not found (or missing [paths] DESTINATIONLINUX):
     {path}

 Only the environment-specific [paths] live there; all other settings (memory
 cap, timeouts, binaries, the optional [windows] box) are defaulted in code.
 Create it like this:

     mkdir -p "{cfgdir}"
     cat > "{path}" <<'EOF'
     [paths]
     # REQUIRED scratch destination (wiped between cases; must have room):
     DESTINATIONLINUX = /var/tmp/ultracopier-spec-test
     # optional real source to copy (empty -> synthetic tree, leaks nothing):
     SOURCELINUX =
     EOF

 Template: plugins/CopyEngine/Ultracopier-Spec/test/config.example.ini
==============================================================================
"""


def load_config() -> configparser.ConfigParser:
    """Read ONLY the private [paths] config from ~/.config/ultracopier-spec-test.ini.
    Everything else is defaulted in code (the getX(..., fallback=) calls below). If the
    file is absent or has no [paths] DESTINATIONLINUX, stop with install instructions."""
    real = real_config_path()
    cp = configparser.ConfigParser()
    cp.read([real])
    if not cp.has_option("paths", "DESTINATIONLINUX") or not cp.get("paths", "DESTINATIONLINUX").strip():
        raise SystemExit(_INSTALL_MSG.format(path=real, cfgdir=os.path.dirname(real)))
    return cp


# ---------------------------------------------------------------------------
# Building / locating the per-backend binary (nodebug so DebugModel never inflates RAM)
# ---------------------------------------------------------------------------
def binary_for(backend: str, cfg: configparser.ConfigParser, asan=False) -> str:
    key = {ASYNC: "async", IO_URING: "io_uring"}.get(backend)
    if asan:
        key = "async_asan"
    pre = cfg.get("binaries", key, fallback="") if key else ""
    if pre and os.path.exists(pre):
        return pre
    bdir = SOURCES / "build" / f"test-{backend}{'-asan' if asan else ''}"
    binpath = bdir / "ultracopier"
    bdir.mkdir(parents=True, exist_ok=True)
    # FileErrorDialogHook.cpp (test-only, under test/hooks/) subclasses FileErrorDialog and installs
    # itself through FileErrorDialog::overrideFactory at static-init, so the otherwise-interactive
    # fileError dialog is driven headlessly (env ULTRACOPIER_TEST_FILE_ERROR_ACTION) WITHOUT any test
    # code, #ifdef or env read in the engine. Appended to SOURCES only here -> a SHIPPING build never
    # compiles it (overrideFactory stays nullptr, the real GUI dialog runs). Replaces the old in-engine
    # env hook. See test/cases/file_error_dialog.py.
    hook = pathlib.Path(__file__).resolve().parents[1] / "hooks" / "FileErrorDialogHook.cpp"
    flags = ["CONFIG+=release", "CONFIG+=nodebug", f"SOURCES+={hook}"]
    env = dict(os.environ)
    if backend == ASYNC:
        # Force the async backend even on a liburing box: hide liburing from pkg-config.
        env["PKG_CONFIG_LIBDIR"] = "/tmp/uc-empty-pc"; os.makedirs("/tmp/uc-empty-pc", exist_ok=True)
    elif backend == IO_URING:
        flags.append("DEFINES+=ULTRACOPIER_PLUGIN_IO_URING")
    if asan:
        flags += ['QMAKE_CXXFLAGS+=-fsanitize=address -fno-omit-frame-pointer -g',
                  'QMAKE_LFLAGS+=-fsanitize=address']
    # qmake only when the Makefile is missing (it just (re)generates build rules), but ALWAYS run
    # make: it is incremental -- a near-instant no-op when nothing changed, and it rebuilds when an
    # engine source is edited. The previous "return the binary if it already exists" CACHED a stale
    # binary and silently tested OLD code after every source change -- a regression-hiding trap.
    if not (bdir / "Makefile").exists():
        subprocess.run(["qmake6", "-o", "Makefile", str(SOURCES / "ultracopier.pro"),
                        "-spec", "linux-g++"] + flags, cwd=bdir, env=env, check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(["make", f"-j{os.cpu_count()}"], cwd=bdir, env=env, check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return str(binpath)


# ---------------------------------------------------------------------------
# Computed memory budget:  cap = (base_RAM + files_list_RAM + buffer_ceiling) * margin
# ---------------------------------------------------------------------------
def _files_list_mb(sources, dest) -> float:
    """RAM a reasonable transfer list needs for THIS source: per file/dir, the source path +
    the derived destination path (dest/<srcname>/<rel>) + ~96 B per-entry overhead (struct +
    two std::string headers). The legitimate, data-proportional component of the budget."""
    total = 0
    for s in sources:
        s = os.path.normpath(s); base = os.path.basename(s)
        for root, dirs, files in os.walk(s):
            for nm in dirs + files:
                src = os.path.join(root, nm)
                rel = os.path.relpath(src, s)
                dst = os.path.join(dest, base, rel)
                total += (len(src.encode("utf-8", "surrogatepass"))
                          + len(dst.encode("utf-8", "surrogatepass")) + 96)
    return total / (1024 * 1024)


_BASE_CACHE = {}
def _base_ram_mb(backend, cfg) -> int:
    """Calibrate ultracopier's BASE resident RAM (Qt + app, ~empty list) once per backend by
    copying a 1-file tree and taking its peak RSS. Cached in-process and in /tmp."""
    if backend in _BASE_CACHE:
        return _BASE_CACHE[backend]
    cache = f"/tmp/uc-test-base-{backend}.txt"
    try:
        _BASE_CACHE[backend] = int(open(cache).read().strip()); return _BASE_CACHE[backend]
    except (OSError, ValueError):
        pass
    src = pathlib.Path(tempfile.mkdtemp(prefix="uc-base-src-")); (src / "f").write_bytes(b"x")
    dst = tempfile.mkdtemp(prefix="uc-base-dst-")
    r = run(backend, "cp", [str(src)], dst, cfg=cfg, mem_limit_mb=4096, stay_alive_seconds=2)
    base = max(60, r.peak_rss_mb)        # floor against a missed sample
    shutil.rmtree(src, ignore_errors=True); shutil.rmtree(dst, ignore_errors=True)
    _BASE_CACHE[backend] = base
    try: open(cache, "w").write(str(base))
    except OSError: pass
    return base


def compute_mem_cap_mb(sources, dest, backend, cfg) -> int:
    """cap = (calibrated base RAM + this source's files-list RAM + 256 MB buffer ceiling) x 1.20."""
    base = _base_ram_mb(backend, cfg)
    files = _files_list_mb(sources, dest)
    cap = int(round((base + files + BUFFER_BUDGET_MB) * MEM_MARGIN)) + 1
    print(f"    [{backend}] mem cap = ({base} base + {files:.1f} files-list + "
          f"{BUFFER_BUDGET_MB} buffer) x{MEM_MARGIN} = {cap} MB")
    return cap


# ---------------------------------------------------------------------------
# Isolated config: a throwaway HOME so we never touch the user's ~/.config/ultracopier.conf
# ---------------------------------------------------------------------------
def write_config(home: pathlib.Path, *, file_collision, folder_collision, file_error,
                 folder_error, keep_date=True, do_right=True, inode_threads=None):
    # The OptionEngine reads QSettings("Ultracopier","Ultracopier") -> $XDG_CONFIG_HOME/Ultracopier/
    # Ultracopier.conf (capital-U dir AND file; ResourcesManager EXTRA_HOME_PATH "/.config/Ultracopier/").
    cdir = home / ".config" / "Ultracopier"; cdir.mkdir(parents=True, exist_ok=True)
    conf = cdir / "Ultracopier.conf"
    # inodeThreads pins the number of PARALLEL per-file transfer threads (default 16).
    # Forcing 1 makes the transfer strictly SEQUENTIAL in list order -- the only way to
    # make "delete the victim after the big first file starts but before its own turn"
    # race-free (with parallel threads a free thread grabs the victim immediately).
    inode_line = f"inodeThreads={int(inode_threads)}\n" if inode_threads else ""
    # QSettings INI. The engine reads the COMBO INDEX, not the enum value.
    conf.write_text(
        "[CopyEngine-Ultracopier Spec]\n"
        f"fileCollision={file_collision}\n"
        f"folderCollision={folder_collision}\n"
        f"fileError={file_error}\n"
        f"folderError={folder_error}\n"
        f"keepDate={'true' if keep_date else 'false'}\n"
        f"doRightTransfer={'true' if do_right else 'false'}\n"
        f"{inode_line}"
        "autoStart=true\n"
        "checkDiskSpace=false\n"
        "[CopyEngine]\n"
        "List=Ultracopier-Spec\n"
        "[Ultracopier]\n"
        "GroupWindowWhen=0\n"
        "confirmToGroupWindows=false\n")
    return conf


def _kill_all_ultracopier():
    """Single-instance lock is per-UID: clear any resident instance + its socket first."""
    subprocess.run(["pkill", "-x", "ultracopier"], check=False)   # EXACT name only (never -f)
    time.sleep(1.0)
    for s in pathlib.Path("/tmp").glob(f"ultracopier-{os.getuid()}*"):
        try: s.unlink()
        except OSError: pass


def _rss_mb(pid: int) -> int:
    try:
        for ln in open(f"/proc/{pid}/status"):
            if ln.startswith("VmRSS"):
                return int(ln.split()[1]) // 1024
    except OSError:
        pass
    return 0


def _cpu_ticks(pid: int) -> int:
    try:
        f = open(f"/proc/{pid}/stat").read().split()
        return int(f[13]) + int(f[14])   # utime + stime
    except (OSError, IndexError):
        return -1


def _io_bytes(pid: int) -> int:
    """wchar from /proc/pid/io: total bytes the process has WRITTEN (passed to write()). A copy
    keeps this climbing at MB/s; an idle resident tray app barely moves it. So a low WRITE-RATE
    (not exactly zero -- the app does incidental I/O) is the reliable 'copy finished' signal,
    robust to a slow/low-CPU large transfer that CPU-idle alone would misjudge."""
    try:
        for ln in open(f"/proc/{pid}/io"):
            if ln.startswith("wchar:"):
                return int(ln.split()[1])
    except (OSError, ValueError):
        pass
    return -1


# ---------------------------------------------------------------------------
# The one entry point every test case uses.
# ---------------------------------------------------------------------------
def run(backend: str, mode: str, sources, dest, *, cfg=None,
        file_collision=FileCollision.ASK, folder_collision=FolderCollision.MERGE,
        file_error=FileError.SKIP, folder_error=FolderError.SKIP,
        keep_date=True, do_right=True, expect_dir=None, memcheck=NONE,
        fs_preload=None, mem_limit_mb=None, stay_alive_seconds=None,
        append_after=None, remove_after=None, inode_threads=None) -> Result:
    """Run one real cp/mv through `backend` and observe it from the outside.

    mode: 'cp' or 'mv'.  sources: list[str] (absolute).  dest: str (absolute).
    expect_dir: if given, diff -rq --no-dereference expect_dir against the copied tree.
    append_after: optional list of {'delay': seconds, 'sources': [paths], 'mode': 'cp'/'mv'}.
        Each entry fires, after `delay` seconds, a SECOND `ultracopier <mode> <sources> <dest>`
        invocation in the SAME env -- which Ultracopier's single-instance LocalListener forwards
        to the already-running instance, appending those sources to the live transfer list. Used
        by the mid-copy "append" test; the extra sources land in `dest` and are verified by the
        caller (the main expect_dir diff only covers the primary tree's subdir)."""
    assert backend in (ASYNC, IO_URING), "IOCP runs via the Windows lane (run_windows)"
    cfg = cfg or load_config()
    stay = stay_alive_seconds if stay_alive_seconds is not None else STAY_ALIVE_SECONDS
    timeout = COPY_TIMEOUT_SECONDS
    # The memory cap is COMPUTED per test, not a fixed 1 GB: base RAM (calibrated once per
    # backend) + the files-list RAM this source legitimately needs + a 256 MB I/O-buffer
    # ceiling, all x 1.20 margin. A run that uses far more than that budget (the O(#files)
    # transfer-list bloat, a leak, or unbounded read-ahead) trips the cap and is caught.
    if mem_limit_mb is None:
        mem_limit_mb = compute_mem_cap_mb(sources, dest, backend, cfg)
    binpath = binary_for(backend, cfg, asan=(memcheck == SANITIZE))

    home = pathlib.Path(tempfile.mkdtemp(prefix="uc-test-home-"))
    write_config(home, file_collision=file_collision, folder_collision=folder_collision,
                 file_error=file_error, folder_error=folder_error, keep_date=keep_date, do_right=do_right,
                 inode_threads=inode_threads)
    _kill_all_ultracopier()

    env = dict(os.environ)
    env.update(HOME=str(home), QT_QPA_PLATFORM="offscreen", XDG_CONFIG_HOME=str(home / ".config"))
    if fs_preload:
        env["LD_PRELOAD"] = f"{fs_preload}:" + env.get("LD_PRELOAD", "")
        env["UC_FS_SCENARIO"] = env.get("UC_FS_SCENARIO", "")
    if memcheck == SANITIZE:
        env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=0:log_path=" + str(home / "asan")

    argv = [binpath, mode] + list(sources) + [dest]
    vg_log = home / "valgrind.log"
    if memcheck == VALGRIND:
        argv = ["valgrind", "--tool=memcheck", "--leak-check=full", "--track-origins=yes",
                "--error-exitcode=0", f"--log-file={vg_log}", "--"] + argv
    # Hard memory cap: ONLY a cgroup RSS limit is correct here. ultracopier reserves ~21 GB
    # of *virtual* address space for its Qt worker threads, so a `ulimit -v` VM cap would
    # kill it at startup (not a real OOM). systemd-run --scope -pMemoryMax caps *RSS* and
    # the kernel OOM-kills + logs the evidence on overshoot. If systemd-run is missing we do
    # NOT fake it with ulimit -v -- the soft RSS monitor in the loop below enforces the cap.
    hard_capped = (shutil.which("systemd-run") is not None and memcheck != VALGRIND)
    if hard_capped:
        argv = ["systemd-run", "--user", "--scope", "-q",
                f"-pMemoryMax={mem_limit_mb}M", "-pMemorySwapMax=0"] + argv

    stderr_path = home / "stderr.log"
    with open(stderr_path, "wb") as ferr:
        proc = subprocess.Popen(argv, env=env, stdout=subprocess.DEVNULL, stderr=ferr)

    # Mid-copy "remove": physically delete source paths once the copy is underway, to
    # exercise the listed-then-source-gone race (open()/openat() of the victim now
    # ENOENTs at transfer time). Either delay-based ({'delay': s}) or, more robustly,
    # event-based ({'when_dest_exists': <abs dest path>, 'min_size': bytes}) which waits
    # until that destination file has grown past min_size -- proof the scan finished and
    # the transfer is mid-flight, so the victim is guaranteed already listed.
    # STARTED RIGHT AFTER Popen (before _wait_for_pid): a fast io_uring copy can finish a
    # small tree in well under a second, so the poller must already be watching the dest
    # from t=0 or it would miss the window entirely and the victim would copy clean.
    remove_threads = []
    for spec in (remove_after or []):
        def _remove(spec=spec):
            trig = spec.get("when_dest_exists")
            if trig is not None:
                t1 = time.time()
                need = spec.get("min_size", 1)
                while time.time() - t1 < timeout:
                    try:
                        if os.path.getsize(trig) >= need:
                            break
                    except OSError:
                        pass
                    time.sleep(0.005)
            else:
                time.sleep(spec.get("delay", 1))
            for p in spec.get("delete", []):
                try:
                    os.remove(p)
                except OSError:
                    pass
        th = threading.Thread(target=_remove, daemon=True)
        th.start(); remove_threads.append(th)

    # Find the real ultracopier pid (systemd-run/valgrind wrap it).
    pid = _wait_for_pid()
    peak = 0; oom = False; completed = False; exit_code = None
    started = time.time()   # epoch -> kernel-log evidence window

    # Mid-copy "append": fire extra cp/mv invocations that the running single-instance forwards
    # and appends to the live transfer list. NOT cgroup-wrapped (they just forward and exit).
    append_threads = []
    for spec in (append_after or []):
        def _fire(spec=spec):
            time.sleep(spec.get("delay", 1))
            extra = [binpath, spec.get("mode", mode)] + list(spec["sources"]) + [dest]
            subprocess.run(extra, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        th = threading.Thread(target=_fire, daemon=True)
        th.start(); append_threads.append(th)

    # 2. wait for the copy to go idle (CPU stops growing) -- robust to IOCP file pre-sizing.
    # If we're injecting read faults (fs_preload), the engine may pause longer on errors.
    # Loosen the idle detection: larger I/O delta threshold (1 MiB) and longer idle period (30 s).
    fault_injection = (fs_preload is not None)
    io_delta_limit = 1024 * 1024 if fault_injection else 256 * 1024   # 1 MiB vs 256 KiB
    idle_threshold = 30 if fault_injection else 10                    # seconds

    last_cpu, last_io, idle_for, t0 = -1, -1, 0, time.time()
    while time.time() - t0 < timeout:
        rss = _rss_mb(pid) if pid else 0
        peak = max(peak, rss)
        # Soft RSS cap: portable enforcement (works without systemd-run) + a belt for the
        # cgroup. Exceeding the budget IS a failure (a memory problem), not a crash.
        if rss > mem_limit_mb:
            oom = True
            break
        alive = (pid and pathlib.Path(f"/proc/{pid}").exists())
        if not alive:
            exit_code = proc.poll()
            oom = oom or _was_oom(proc, exit_code) or _dmesg_oom(started)
            break
        cpu = _cpu_ticks(pid) if pid else -1
        io = _io_bytes(pid) if pid else -1
        cpu_delta = (cpu - last_cpu) if (cpu >= 0 and last_cpu >= 0) else 1 << 30
        io_delta = (io - last_io) if (io >= 0 and last_io >= 0) else 1 << 30
        # "quiet" = not actively copying: near-idle CPU AND < io_delta_limit written this interval.
        # A real transfer writes MB/s (even slow/large, even mid-error with files pending);
        # a resident tray app only does incidental writes -- so this is robust where exact-zero
        # or CPU-alone were not (the latter falsely "finished" during an error pause).
        quiet = (cpu_delta <= 3 and io_delta < io_delta_limit)
        idle_for = idle_for + 2 if quiet else 0
        last_cpu, last_io = cpu, io
        if idle_for >= idle_threshold:          # ~idle_threshold seconds near-idle == truly finished (resident in tray)
            completed = True
            break
        time.sleep(2)

    stayed_alive = False
    if completed:
        # 3. it must still be alive `stay` seconds later (proves no teardown crash)...
        for _ in range(stay):
            time.sleep(1); peak = max(peak, _rss_mb(pid) if pid else 0)
        stayed_alive = bool(pid and pathlib.Path(f"/proc/{pid}").exists())

    # 4. ...then kill it from OUTSIDE -- no in-code "exit after copy" shortcut.
    _kill_all_ultracopier()
    for th in append_threads:   # the forwarding invocations have long since fired; join for hygiene
        th.join(timeout=5)
    for th in remove_threads:   # the deletions have long since fired; join for hygiene
        th.join(timeout=5)

    # 5. verify content + memory.
    content_ok, diff_text = (True, "")
    if expect_dir is not None:
        content_ok, diff_text = _diff(expect_dir, _copied_tree(dest, sources))
    mem_errors = _valgrind_errors(vg_log) if memcheck == VALGRIND else (
        _asan_errors(home) if memcheck == SANITIZE else 0)

    # 6. EVIDENCE: when anything went wrong (OOM / over the RSS cap / abnormal exit / did
    #    not stay alive / did not complete), pull the relevant kernel-log lines (OOM-kill of
    #    ultracopier, ATA bus resets, EXT4/I-O errors) and surface them as proof, with peak RSS.
    notes = f"peak_rss={peak}MB cap={mem_limit_mb}MB hard_cgroup_cap={hard_capped}"
    problem = (oom or not completed or (completed and not stayed_alive)
               or (exit_code not in (None, 0)))
    if problem:
        ev = _kernel_evidence(started)
        notes += "\n" + ev
        print(f"    [{backend}] PROBLEM ({'OOM/over-cap' if oom else 'crash/incomplete'}); "
              f"peak_rss={peak}MB/{mem_limit_mb}MB exit_code={exit_code}. KERNEL EVIDENCE:")
        # Print stderr if available
        try:
            with open(stderr_path, "r", errors="replace") as f:
                stderr_content = f.read().strip()
                if stderr_content:
                    print("        STDERR:")
                    for l in stderr_content.splitlines()[-10:]:  # last 10 lines
                        print("          " + l)
        except Exception:
            pass
    shutil.rmtree(home, ignore_errors=True)
    return Result(backend, completed, stayed_alive, content_ok, peak, oom, mem_errors,
                  exit_code, diff_text, notes=notes, hard_capped=hard_capped)


def _wait_for_pid(deadline=20):
    t0 = time.time()
    while time.time() - t0 < deadline:
        out = subprocess.run(["pgrep", "-x", "ultracopier"], capture_output=True, text=True)
        pids = [int(x) for x in out.stdout.split()]
        if pids:
            return pids[0]
        time.sleep(0.3)
    return None


def _was_oom(proc, exit_code) -> bool:
    return exit_code is not None and exit_code < 0 and -exit_code in (signal.SIGKILL,)


def _kernel_log(since_epoch) -> str:
    """Best-effort recent kernel log. Tries journalctl -k (works without root if in the
    systemd-journal group) then dmesg. '' if neither is readable (dmesg_restrict)."""
    since = max(5, int(time.time() - since_epoch) + 5)
    for cmd in (["journalctl", "-k", "--since", f"-{since}s", "-o", "short-precise", "--no-pager"],
                ["dmesg", "--ctime"]):
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=12)
            if r.returncode == 0 and r.stdout.strip():
                return r.stdout
        except (OSError, subprocess.SubprocessError):
            continue
    return ""


_EVIDENCE_TERMS = ("ultracopier", "oom", "out of memory", "killed process", "ata",
                   "i/o error", "ext4-fs error", "buffer i/o", "hard resetting link",
                   "medium error", "unrecovered read error", "failcnt")


def _kernel_evidence(since_epoch) -> str:
    """Extract the lines that prove a problem (OOM-kill of ultracopier, ATA resets,
    EXT4 / I/O / medium errors) from the recent kernel log -- the 'text extract' a failing
    test must show. Falls back to a hint if the kernel log is not accessible."""
    txt = _kernel_log(since_epoch)
    if not txt:
        return ("(kernel log not accessible: run the tests as root, or set "
                "kernel.dmesg_restrict=0, or join the systemd-journal group, to capture "
                "OOM / ATA / EXT4 / I-O evidence)")
    lines = [l for l in txt.splitlines() if any(t in l.lower() for t in _EVIDENCE_TERMS)]
    return "\n".join(lines[-40:]) if lines else "(no OOM/ATA/EXT4/I-O lines in the kernel log window)"


def _dmesg_oom(since_epoch) -> bool:
    t = _kernel_log(since_epoch).lower()
    return ("out of memory: killed process" in t and "ultracopier" in t) \
        or ("killed process" in t and "ultracopier" in t)


def _copied_tree(dest, sources):
    # ultracopier copies the source FOLDER into dest -> dest/<basename(src)>.
    base = os.path.basename(os.path.normpath(sources[0]))
    cand = os.path.join(dest, base)
    return cand if os.path.isdir(cand) else dest


def _diff(a, b):
    r = subprocess.run(["diff", "-rq", "--no-dereference", a, b], capture_output=True, text=True)
    return (r.returncode == 0, r.stdout + r.stderr)


def _valgrind_errors(log):
    if not pathlib.Path(log).exists():
        return 0
    txt = pathlib.Path(log).read_text(errors="ignore")
    import re
    m = re.search(r"ERROR SUMMARY:\s+(\d+)\s+errors", txt)
    # count only errors whose stack touches our code, not Qt/glib one-time noise
    real = sum(1 for blk in txt.split("\n\n")
               if ("Invalid" in blk or "uninitialised" in blk.lower())
               and ("Ultracopier-Spec" in blk or "TransferThread" in blk or "ListThread" in blk))
    return real if real else (0 if not m else 0)


def _asan_errors(home):
    n = 0
    for f in pathlib.Path(home).glob("asan*"):
        t = f.read_text(errors="ignore")
        if "ERROR: AddressSanitizer" in t and ("Ultracopier-Spec" in t or "TransferThread" in t):
            n += t.count("ERROR: AddressSanitizer")
    return n
