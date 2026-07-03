#!/usr/bin/env python3
"""#9 data-loss regression (found by the 2026-07-02 adversarial review): a SAME-DRIVE MOVE whose
atomic rename() FAILS (non-EXDEV -- a stuck mount / EACCES) while an interactive SKIP is in flight
must leave the user's PRE-EXISTING destination UNTOUCHED. A failed rename is atomic: it made NO
partial of ours, so the engine must never unlink the destination.

BUG: the skip/stop teardown in TransferThreadAsync (the site-C/A `if(stopIt)` unlink) called
unlink(destination). destinationIsOursToRemove() reads STALE writeThread state for a realMove (the
write pipeline is never opened -- rename replaces the file directly), so it returned true and the
user's untouched destination was DELETED. Fix: never unlink at those sites when realMove is true.

RIG: the in-process driver (unit/moveskip_test.cpp, cloning the accepted engine_api/livecontrol
drivers) runs the REAL CopyEngine; the LD_PRELOAD shim's `renamefail:<dstname>` BLOCKS rename() 800ms
then fails EACCES, so the driver can engine.skip(runningId) while the transfer thread is parked in
rename() (transfer_stat==Transfer -- the exact state that reaches the buggy unlink). After the
transfer thread finishes its teardown, the destination MUST still hold the pre-seed bytes.

ASSERTS: driver exits 0 (destination survived byte-identical) AND the source also survives (a
skipped/failed move must lose nothing). Verified red->green: with the fix removed the dest is deleted
(exit 1). Async backend only (realMove/rename is the async data plane; io_uring/IOCP inherit the same
shared teardown but a same-fs rename is identical there).
"""
import sys, os, pathlib, subprocess, tempfile, shutil
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

_UNIT_DIR = pathlib.Path(__file__).resolve().parents[1] / "unit"
_PRO = _UNIT_DIR / "moveskip_test.pro"
SRC_BYTES = b"SOURCE-move-bytes-should-stay-in-src\n"
PRESEED = b"PRESEED-user-destination-MUST-SURVIVE-the-failed-move\n"


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def _build() -> str:
    bdir = pathlib.Path(tempfile.gettempdir()) / "uc-moveskip"
    bdir.mkdir(parents=True, exist_ok=True)
    if not (bdir / "Makefile").exists():
        q = subprocess.run([_qmake(), "-o", "Makefile", str(_PRO), "-spec", "linux-g++",
                            "CONFIG+=release", "CONFIG+=nodebug"],
                           cwd=bdir, capture_output=True, text=True)
        if q.returncode != 0:
            raise RuntimeError("qmake failed:\n" + q.stderr[-2000:])
    m = subprocess.run(["make", f"-j{os.cpu_count()}"], cwd=bdir, capture_output=True, text=True)
    binp = bdir / "moveskip_test"
    if m.returncode != 0 or not binp.exists():
        raise RuntimeError("make failed:\n" + (m.stderr or m.stdout)[-2500:])
    return str(binp)


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.ASYNC not in backends:
        print("    [skip] move_rename_skip is async-only (shared realMove teardown)")
        return True
    try:
        binp = _build()
    except Exception as e:
        print(f"    [move_rename_skip] BUILD FAILED: {e}")
        return False

    # src and dest under ONE scratch dir so isSameDrive() -> realMove(rename) is taken.
    root = K.fresh_src_root("mvrenskip")
    shutil.rmtree(root, ignore_errors=True)
    src_dir = os.path.join(root, "src")
    dst_dir = os.path.join(root, "dst")
    os.makedirs(src_dir); os.makedirs(dst_dir)
    src_file = os.path.join(src_dir, "victim.dat")
    dst_file = os.path.join(dst_dir, "victim.dat")
    K.write_file(src_file, SRC_BYTES)
    K.write_file(dst_file, PRESEED)   # the user's pre-existing destination (Overwrite collision)

    ok = True
    for i in range(3):   # timing-sensitive rig -> require deterministic pass
        K.write_file(src_file, SRC_BYTES)
        K.write_file(dst_file, PRESEED)
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen", DISPLAY="",
                   UC_FS_SCENARIO="renamefail:victim.dat",
                   LD_PRELOAD=K.fs_so())
        try:
            r = subprocess.run([binp, src_file, dst_dir], env=env,
                               capture_output=True, text=True, timeout=120)
        except subprocess.TimeoutExpired:
            subprocess.run(["pkill", "-9", "-x", "moveskip_test"], capture_output=True)
            print(f"    [move_rename_skip] FAIL: driver HUNG (iter {i})")
            return False
        out = r.stdout + r.stderr
        dest_ok = os.path.exists(dst_file) and K.read_file(dst_file) == PRESEED
        src_ok = os.path.exists(src_file) and K.read_file(src_file) == SRC_BYTES
        acted = "acted=1" in out
        good = (r.returncode == 0 and dest_ok and src_ok and acted)
        print(f"      iter{i}: rc={r.returncode} acted={acted} dest_survived={dest_ok} src_survived={src_ok}")
        if not good:
            print("    [move_rename_skip] FAIL: a skipped/failed same-drive move destroyed data -- "
                  + ("dest DELETED/altered" if not dest_ok else "source lost" if not src_ok else
                     "never caught the move in flight" if not acted else f"rc={r.returncode}"))
            print("      tail: " + out[-400:])
            ok = False
            break
    if ok:
        print("    [move_rename_skip] PASS  (failed same-drive move + skip: destination + source both intact)")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
