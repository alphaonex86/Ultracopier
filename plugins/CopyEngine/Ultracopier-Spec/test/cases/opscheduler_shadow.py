#!/usr/bin/env python3
"""STAGE 1b.0 shadow observer: build a DEBUG io_uring binary and drive a real recursive cp through
it, asserting the ListThread shadow observer (pipeline/OpScheduler.h wired into ListThread.cpp per
pipeline/STAGE_1B0_INTEGRATION.md) reports ZERO violations -- proving the engine's REAL op-emission
order is a valid topological linearization of the decomposition DAG.

This is a DIFFERENT proof than ops_integrity.py/ops_dependency.py: those infer the op order from an
OUTSIDE trace (the LD_PRELOAD shim) after the fact; this one is the engine's OWN internal scheduler
model checking itself live, IN-PROCESS, against every real dispatch/finish call. It only exists in a
DEBUG build (`ULTRACOPIER_PLUGIN_DEBUG`, i.e. dropping `CONFIG+=nodebug`) -- the NODEBUG release the
rest of the suite runs, and the async backend, compile it out entirely (verified separately: `strings`
on a NODEBUG binary must contain zero "opShadow"/"OPSHADOW" occurrences -- see the observer's design
doc). So this case builds its OWN debug io_uring binary in a private build dir, entirely separate from
the harness's shared `build/test-io_uring` (which must stay NODEBUG for every other case).

The observer's first cut covers only a plain-Copy, no-fault, no-collision, no-reorder job with
keepDate/doRightTransfer on (see pipeline/STAGE_1B0_INTEGRATION.md for why) -- this case drives exactly
that shape. It asserts:
  * the copy completes and content is correct (diff -rq --no-dereference == 0);
  * the observer actually engaged (an "OPSHADOW built" line appears -- proves opShadowActive, not a
    silent no-op on a shape it declined);
  * EVERY op it modeled was checked (checks == nodes, i.e. == opCount()) -- non-vacuous: no op was
    silently skipped;
  * ZERO violations.
"""
import sys, pathlib, os, subprocess, tempfile, shutil, time
# Strip cases/ from sys.path so stdlib `import copy` (pulled in by dataclasses, used by harness.py)
# is not shadowed by cases/copy.py when this file is run directly (see windows_ads.py for the same fix).
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K
from lib import synthtree

UC_PRO = pathlib.Path(__file__).resolve().parents[5] / "ultracopier.pro"  # top-level all-in-one .pro


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def _build_debug_uring() -> str:
    """Build (or reuse) a DEBUG io_uring binary: same DEFINES as the harness's io_uring build, but
    WITHOUT CONFIG+=nodebug (which is what activates ULTRACOPIER_PLUGIN_DEBUG -- see
    CopyEngineUltracopier-SpecVariable.h + the top-level ultracopier.pro debug/nodebug switch),
    PLUS ULTRACOPIER_DEBUG_CONSOLE so the debug log (where the OPSHADOW lines land) goes to STDOUT --
    without it DebugEngine writes only to its HTML log file and this case sees nothing (the exact
    first-run failure mode of this case)."""
    bdir = pathlib.Path(tempfile.gettempdir()) / "uc-opshadow-debug-uring"
    binp = bdir / "ultracopier"
    mk = bdir / "Makefile"
    # qmake Makefiles do NOT track flag changes (no .cmd files): if the Makefile predates the
    # ULTRACOPIER_DEBUG_CONSOLE define, stale objects built without it would be reused silently.
    # Wipe the (case-owned, /tmp-private) dir and regenerate in that case.
    if mk.exists() and "ULTRACOPIER_DEBUG_CONSOLE" not in mk.read_text(errors="replace"):
        shutil.rmtree(bdir, ignore_errors=True)
    bdir.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    if not mk.exists():
        q = subprocess.run([_qmake(), "-o", "Makefile", str(UC_PRO), "-spec", "linux-g++",
                            "CONFIG+=release", "DEFINES+=ULTRACOPIER_PLUGIN_IO_URING",
                            "DEFINES+=ULTRACOPIER_DEBUG_CONSOLE"],
                           cwd=bdir, env=env, capture_output=True, text=True)
        if q.returncode != 0:
            raise RuntimeError("qmake failed:\n" + q.stderr[-2000:])
    m = subprocess.run(["make", f"-j{os.cpu_count()}"], cwd=bdir, env=env, capture_output=True, text=True)
    if m.returncode != 0:
        raise RuntimeError("make failed:\n" + (m.stderr or m.stdout)[-3000:])
    if not binp.exists():
        raise RuntimeError(f"build finished but {binp} is missing")
    # Sanity: the DEBUG build must actually CONTAIN the observer (a NODEBUG build compiles it out).
    # Distinguishes "binary lacks the observer" (a build/wiring problem) from "observer declined the
    # job shape" (a runtime problem) -- both would otherwise look identical in the log.
    # NB the "OPSHADOW ..." log literals are QStringLiteral = UTF-16, INVISIBLE to default (ASCII)
    # `strings`; check the ASCII SYMBOL names (opShadowBuild etc., present unless stripped) and the
    # UTF-16 literals (`strings -el`, robust even if symbols are stripped) -- either proves it's in.
    ascii_s = subprocess.run(["strings", str(binp)], capture_output=True, text=True).stdout
    utf16_s = subprocess.run(["strings", "-el", str(binp)], capture_output=True, text=True).stdout
    if "opShadow" not in ascii_s and "OPSHADOW" not in utf16_s:
        raise RuntimeError("built binary contains no opShadow symbols nor UTF-16 OPSHADOW literals -- "
                           "the observer was compiled out (ULTRACOPIER_PLUGIN_DEBUG not active?)")
    return str(binp)


def _run_and_collect(binpath: str, src: str, dest: str, timeout_s: int = 120):
    """Drive one cp through `binpath` (isolated HOME/conf), capturing stdout (the debug console
    output the shadow observer logs to -- see ListThread.cpp's ULTRACOPIER_DEBUGCONSOLE usage).
    Returns (content_diff_text, log_text)."""
    home = pathlib.Path(tempfile.mkdtemp(prefix="uc-opshadow-home-"))
    try:
        # inode_threads=1 is REQUIRED, not a convenience: the shadow observer is a SINGLE-inode-thread
        # linearization proof. Its three hooks fire at different lifecycle points (mkdir/setMetaDir at
        # action-dispatch, file at transferInodeIsClosed), which line up with the physical FS-op order
        # ONLY at one inode thread. At the engine default (16) the hook-firing order decouples from
        # physical completion across threads -> spurious `unmet` (cascade non-violations) + the Idle
        # summary racing the last deferred SyncDate -> checks<nodes. That is NOT an engine bug (content
        # always diffs clean; the multi-thread dependency edges are gated by ops_dependency.py + the
        # thread-local, here-proven per-file open<data<close chain). Matches CLAUDE.md "get the scheduler
        # correct first (even single-inode-thread)... keep the option to pin 1 inode thread", and the
        # observer itself now skips (honest no-op) at inodeThreads>1. Deterministic 196/196/0, incl. under load.
        H.write_config(home, file_collision=H.FileCollision.OVERWRITE, folder_collision=H.FolderCollision.MERGE,
                       file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                       keep_date=True, do_right=True,   # REQUIRED: see STAGE_1B0_INTEGRATION.md shapeOk
                       inode_threads=1)
        H._kill_all_ultracopier()
        env = dict(os.environ)
        env.update(HOME=str(home), QT_QPA_PLATFORM="offscreen", DISPLAY="",
                   XDG_CONFIG_HOME=str(home / ".config"), ULTRACOPIER_SOCKET_SUFFIX=H.TEST_SOCKET_SUFFIX)
        out_path = home / "stdout.log"
        with open(out_path, "wb") as fout:
            subprocess.Popen([binpath, "cp", src, dest], env=env, stdout=fout, stderr=subprocess.STDOUT)

        expected = sum(len(files) for _dp, _d, files in os.walk(src))
        copied = os.path.join(dest, os.path.basename(src))

        def copied_count():
            if not os.path.isdir(copied):
                return -1
            return sum(len(files) for _dp, _d, files in os.walk(copied))

        t0 = time.time()
        last = -2
        stable_since = None
        while time.time() - t0 < timeout_s:
            n = copied_count()
            if n != last:
                last = n
                stable_since = time.time()
            if n >= expected and stable_since is not None and (time.time() - stable_since) > 2.5:
                break
            time.sleep(0.05)
        time.sleep(1.5)  # let the deferred folder-meta (SyncDate) actions + the Idle summary log settle

        subprocess.run(["pkill", "-x", "ultracopier"])
        time.sleep(0.5)

        diff = subprocess.run(["diff", "-rq", "--no-dereference", src, copied],
                              capture_output=True, text=True)
        log = out_path.read_text(errors="replace")
        return diff.stdout, log
    finally:
        shutil.rmtree(home, ignore_errors=True)


def _parse_summary(log: str):
    """Return (built: bool, nodes, checks, violations) parsed from the OPSHADOW log lines, or
    (False, 0, 0, 0) if the observer never engaged (declined the job shape)."""
    built = "OPSHADOW built:" in log
    nodes = checks = violations = 0
    for line in log.splitlines():
        if "OPSHADOW summary:" in line:
            # "... OPSHADOW summary: nodes=28 checks=28 violations=0"
            for tok in line.split():
                if tok.startswith("nodes="):
                    nodes = int(tok.split("=", 1)[1])
                elif tok.startswith("checks="):
                    checks = int(tok.split("=", 1)[1])
                elif tok.startswith("violations="):
                    violations = int(tok.split("=", 1)[1])
    return built, nodes, checks, violations


def run(backends=None, memcheck=H.NONE) -> bool:
    # Backend-independent gate name (io_uring/IOCP-only feature; async/other requested backends are
    # a silent pass since the observer simply does not apply there -- mirrors preallocate.py's pattern).
    try:
        binpath = _build_debug_uring()
    except Exception as e:
        print(f"    [opscheduler_shadow] BUILD FAILED: {e}")
        return False

    ok = True
    src = synthtree.make_tree(K.fresh_src_root("opshadow_src"), "comprehensive")
    dest = K.fresh_dest("opshadow_dest")
    diff_text, log = _run_and_collect(binpath, src, dest)

    if diff_text.strip():
        print("      content diff (must be empty):\n" + diff_text)
        ok = False

    built, nodes, checks, violations = _parse_summary(log)
    if not built:
        print("      OPSHADOW never engaged (job shape declined) -- expected it to for a plain "
              "Copy/keepDate/doRightTransfer job; the shape-detection or the test tree changed")
        ok = False
    if nodes == 0:
        print("      OPSHADOW summary missing or nodes=0 -- observer did not build a real model")
        ok = False
    if checks != nodes:
        print(f"      OPSHADOW non-vacuous check FAILED: checks={checks} != nodes(ops)={nodes} "
              f"-- some op in the DAG was never exercised by a hook")
        ok = False
    if violations != 0:
        print(f"      OPSHADOW_VIOLATION(S): {violations} -- the engine's real dispatch order broke "
              f"a modeled FS dependency edge; see the OPSHADOW_VIOLATION lines below")
        for line in log.splitlines():
            if "OPSHADOW_VIOLATION" in line:
                print("        " + line)
        ok = False
    print(f"      OPSHADOW: built={built} nodes={nodes} checks={checks} violations={violations}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
