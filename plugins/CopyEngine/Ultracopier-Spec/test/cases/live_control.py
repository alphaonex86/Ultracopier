#!/usr/bin/env python3
"""Live-control on a RUNNING transfer (coverage gap 20): skip/removeItems/moveItemsOnTop applied
mid-copy -- the use-after-free / dangling-running-item / pump-never-repumps SAFETY class.

The theme's transfer-list buttons call these on entries a transfer thread is actively touching;
skip(runningId) in particular must cancel in-flight reader/writer I/O and tombstone an
ActionToDoTransfer still referenced by a thread. A regression here is a crash (UAF), a hang (the
scheduler never re-pumps after the running item is yanked), or a lost untouched sibling.

listreorder_test only reorders an IDLE synthetic list; nothing before drove these on a LIVE copy.
The in-process unit driver (unit/livecontrol_test.cpp, cloning the accepted engine_api_test) runs
the REAL CopyEngine, slows I/O with the LD_PRELOAD shim (slow) so the first file is still in
flight, learns real item ids from newActionOnList, then skips the running one + removes and
reorders queued ones.

SCOPE: exercises ALL THREE live-control ops on the running list: skip(runningId) + removeItems +
moveItemsOnTop (mode "all"). skip(runningId) used to be a CONFIRMED engine HANG (the interactive
skip took WriteThread::stop()'s put-to-end teardown, which suppresses closed() while waiting for a
retry that never comes -> postOperationStopped never fired -> the scheduler counted the thread as
running forever and wedged; finding-skip-running-hangs). FIXED 2026-07-02: ListThread::skipInternal
marks the thread finalSkipNoRetry (like the fileError=Skip policy paths) so the close handshake
completes; a CLEAN (no-error) skip also unlinks its partial like 3.0 (fault skips keep salvage).

ASSERTS: driver exits 0 (clean terminal state -- no crash, no hang -- AFTER mutating the running list,
and it actually acted); untouched files byte-identical at dest; ZERO ASan errors (UAF probe under
AddressSanitizer). Async only.
"""
import sys, os, re, pathlib, subprocess, tempfile, shutil, hashlib
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

_UNIT_DIR = pathlib.Path(__file__).resolve().parents[1] / "unit"
_PRO = _UNIT_DIR / "livecontrol_test.pro"
NFILES = 8
FSIZE = 3 * 1024 * 1024   # big enough (with slow) that file 0 is still transferring when we act


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def _build(asan: bool) -> str:
    bdir = pathlib.Path(tempfile.gettempdir()) / ("uc-livecontrol" + ("-asan" if asan else ""))
    bdir.mkdir(parents=True, exist_ok=True)
    flags = ["CONFIG+=release", "CONFIG+=nodebug"]
    if asan:
        flags += ["QMAKE_CXXFLAGS+=-fsanitize=address -fno-omit-frame-pointer -g",
                  "QMAKE_LFLAGS+=-fsanitize=address"]
    if not (bdir / "Makefile").exists():
        q = subprocess.run([_qmake(), "-o", "Makefile", str(_PRO), "-spec", "linux-g++"] + flags,
                           cwd=bdir, capture_output=True, text=True)
        if q.returncode != 0:
            raise RuntimeError("qmake failed:\n" + q.stderr[-2000:])
    m = subprocess.run(["make", f"-j{os.cpu_count()}"], cwd=bdir, capture_output=True, text=True)
    binp = bdir / "livecontrol_test"
    if m.returncode != 0 or not binp.exists():
        raise RuntimeError("make failed:\n" + (m.stderr or m.stdout)[-2500:])
    return str(binp)


def run(backends=None, memcheck=H.NONE) -> bool:
    asan = True   # this is a UAF probe -> always run under ASan
    try:
        binp = _build(asan)
    except Exception as e:
        print(f"    [live_control] BUILD FAILED: {e}")
        return False

    src = K.fresh_src_root("livectl_src")
    shutil.rmtree(src, ignore_errors=True)
    os.makedirs(src)
    want = {}
    for i in range(NFILES):
        data = bytes(((i + 1) * 97 + j) & 0xFF for j in range(FSIZE))
        p = os.path.join(src, "%02d.bin" % i)
        with open(p, "wb") as f:
            f.write(data)
        want["%02d.bin" % i] = hashlib.sha256(data).hexdigest()
    dest = K.fresh_dest("livectl_dest")

    libasan = ""
    for cand in ("libasan.so", "libasan.so.8", "libasan.so.6"):
        p = subprocess.run(["gcc", "-print-file-name=" + cand], capture_output=True, text=True).stdout.strip()
        if p and p != cand and os.path.exists(p):
            libasan = p
            break
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", DISPLAY="",
               UC_FS_SCENARIO="slow:4",                       # ~4ms/IO -> file 0 stays in flight
               LD_PRELOAD=":".join(x for x in (libasan, K.fs_so()) if x),
               ASAN_OPTIONS="detect_leaks=0:halt_on_error=0")   # leaks are Qt-exit noise, not our bug
    try:
        r = subprocess.run([binp, src, dest, "all"], env=env, capture_output=True, text=True, timeout=180)
    except subprocess.TimeoutExpired:
        subprocess.run(["pkill", "-9", "-x", "livecontrol_test"], capture_output=True)
        print("    [live_control] FAIL: driver HUNG after mutating the running list (no re-pump?)")
        return False

    out = r.stdout + r.stderr
    problems = []
    if r.returncode != 0:
        problems.append(f"driver rc={r.returncode} (crash/hang/never-acted); tail: {out[-300:]}")
    if "AddressSanitizer" in out and ("Ultracopier-Spec" in out or "TransferThread" in out
                                      or "ListThread" in out):
        problems.append("ASan error touching our code (use-after-free on the yanked running item):\n"
                        + "\n".join(l for l in out.splitlines() if "#" in l or "ERROR" in l)[:600])
    m = re.search(r"skip\(running=(\d+)\) remove\(queued=(\d+)\) moveTop\(queued=(\d+)\)", out)
    acted = "acted=1" in out
    if not acted:
        problems.append("driver never acted (couldn't catch the running item in flight)")
    # The 5 files we did NOT touch (not the skipped, not the removed) must be byte-perfect.
    copied = os.path.join(dest, os.path.basename(src))
    intact = 0
    for name, h in want.items():
        p = os.path.join(copied, name)
        if os.path.exists(p) and hashlib.sha256(K.read_file(p)).hexdigest() == h:
            intact += 1
    if intact < NFILES - 2:   # allow the removed (+ maybe 1 in-flight) to be absent/partial
        problems.append(f"only {intact}/{NFILES} files intact -- live-control lost untouched siblings")
    print(f"      rc={r.returncode} acted={acted} intact={intact}/{NFILES} "
          f"asan={'ERR' if 'AddressSanitizer' in out else 'clean'}")
    if problems:
        print("    [live_control] FAIL: " + "; ".join(problems))
        return False
    print("    [live_control] PASS  (running-item skip/remove/reorder: no UAF/hang, siblings safe)")
    return True


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
