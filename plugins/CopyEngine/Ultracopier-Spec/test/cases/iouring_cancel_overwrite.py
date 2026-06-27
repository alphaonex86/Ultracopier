#!/usr/bin/env python3
"""#9 on CANCEL: cancelling an OVERWRITE job must NOT delete the user's pre-existing destination
(the pipelined stopIt unlinks were ungated -- the cancel-path counterpart of iouring_source_vanish).

Source mounted through fuse_fault with UC_FUSE_OPENFAIL=victim.dat: the victim's transfer-open fails,
raising the FileError dialog; the test FileErrorDialog hook returns CANCEL
(ULTRACOPIER_TEST_FILE_ERROR_ACTION=cancel) -> the whole job is cancelled (stopIt).

RESILIENCE GUARD: cancelling such a job must leave the pre-seeded OLD destination intact and not crash.
HONEST CAVEAT: in THIS scenario the victim's dest is never opened (its source open fails first), so the
OLD survives whether or not the stopIt unlinks are gated -- it exercises cancel resilience but NOT the
gated deletion directly. A clean cancel-of-OVERWRITE deletion (cancel mid-write, dest opened, OLD still
present) needs the UI cancel button, which is not headlessly drivable. The 4 pipelined stopIt unlinks
(382/433/586/604) ARE gated with destinationIsOursToRemove() anyway, for consistency with the PROVEN
skip-path #9 fix ([[project_iouring_source_vanish_fix]], red->green via iouring_source_vanish).

REQUIRED: the dest victim STILL holds OLD byte-for-byte after the cancel; the process stays alive, no
crash/leak. io_uring ONLY; needs libfuse3 + fusermount3 (skipped cleanly otherwise)."""
import os
import sys
import time
import shutil
import pathlib
import subprocess

_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import casekit as K

OLD = b"OLD-destination-content-that-MUST-survive-a-CANCEL\n" * 16
NEW = b"NEW-source-content-the-engine-cannot-open\n" * 16
_FSO = _TEST_DIR / "fsoverride"


def _fuse_bin():
    if not (pathlib.Path("/dev/fuse").exists() and shutil.which("fusermount3")):
        return None
    if subprocess.run(["pkg-config", "--exists", "fuse3"]).returncode != 0:
        return None
    out = _FSO / "build" / "fuse_fault"
    subprocess.run(["make", "-C", str(_FSO), "fuse_fault"], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    return out if out.exists() else None


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IO_URING not in backends:
        print("    [skip] iouring_cancel_overwrite is io_uring-only")
        return True
    fuse = _fuse_bin()
    if fuse is None:
        print("    [skip] libfuse3/fusermount3 unavailable")
        return True

    back = pathlib.Path("/tmp/uc-cancel-back")
    mnt = pathlib.Path("/tmp/uc-cancel-mnt")
    subprocess.run(["fusermount3", "-u", str(mnt)], stderr=subprocess.DEVNULL)
    for d in (back, mnt):
        shutil.rmtree(d, ignore_errors=True); d.mkdir(parents=True)
    (back / "victim.dat").write_bytes(NEW)
    (back / "good1.dat").write_bytes(b"good-one\n" * 50)

    fenv = dict(os.environ, UC_FUSE_BACKING=str(back), UC_FUSE_OPENFAIL="victim.dat")
    subprocess.run([str(fuse), str(mnt), "-s"], env=fenv, check=True)
    time.sleep(1)
    if subprocess.run(["mountpoint", "-q", str(mnt)]).returncode != 0:
        print("    [skip] FUSE mount failed")
        return True
    os.environ["ULTRACOPIER_TEST_FILE_ERROR_ACTION"] = "cancel"   # the dialog hook cancels the whole job
    try:
        dest = K.fresh_dest("cancel_dest")
        copied = os.path.join(dest, mnt.name)
        os.makedirs(copied, exist_ok=True)
        pathlib.Path(copied, "victim.dat").write_bytes(OLD)       # pre-seed the colliding destination

        r = H.run(H.IO_URING, "cp", [str(mnt)], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.ASK,                      # Ask -> the hook injects Cancel on the fault
                  folder_error=H.FolderError.SKIP,
                  expect_dir=None)

        vpath = pathlib.Path(copied, "victim.dat")
        vb = vpath.read_bytes() if vpath.exists() else b"<ABSENT>"
        victim_intact = (vb == OLD)
        ok = (r.completed and r.stayed_alive and victim_intact
              and not r.oom_killed and r.mem_errors == 0)
        print(f"      victim_OLD_preserved={victim_intact} completed={r.completed} alive={r.stayed_alive} "
              f"mem_err={r.mem_errors}")
        if not victim_intact:
            print(f"        DATA LOSS on CANCEL: dest victim != OLD (size={len(vb)}); first 50: {vb[:50]!r}")
        print(f"    [iouring_cancel_overwrite] {'PASS' if ok else 'FAIL'}")
        return ok
    finally:
        os.environ.pop("ULTRACOPIER_TEST_FILE_ERROR_ACTION", None)
        subprocess.run(["fusermount3", "-u", str(mnt)], stderr=subprocess.DEVNULL)
        subprocess.run(["pkill", "-9", "-x", "ultracopier"], capture_output=True)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
