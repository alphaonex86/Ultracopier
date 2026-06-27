#!/usr/bin/env python3
"""io_uring port-check of the #9 OVERWRITE + SOURCE-VANISH data-loss.

collision_overwrite_source_vanish (async) uses the libc `gone` verb; io_uring opens the source via
io_uring_prep_openat (bypasses libc), so that test can't reach it. Here the source is mounted through
fuse_fault with UC_FUSE_OPENFAIL=victim.dat: the victim is LISTED at scan (getattr/readdir succeed) but
its transfer-time OPEN returns ENOENT -- a TOCTOU vanish that DOES reach the ring openat.

The destination victim is pre-seeded with OLD content; fileCollision=OVERWRITE, fileError=SKIP.

REQUIRED (resilient, mirrors the async fix): the dest victim STILL holds OLD byte-for-byte -- a source
that vanishes before the first byte must NOT destroy the user's pre-existing copy (cp/rsync leave it
untouched). The good files copy; the job completes; no crash/leak. If OLD is gone/truncated, io_uring has
the #9 source-vanish data-loss and the pipelined dest-removal/truncate sites need destinationIsOursToRemove().

io_uring ONLY; needs libfuse3 + fusermount3 (skipped cleanly otherwise)."""
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

OLD = b"OLD-destination-content-that-MUST-survive-the-vanished-source\n" * 16
NEW = b"NEW-source-content-that-never-arrives-because-the-source-vanished\n" * 16
GOOD1 = b"good-file-one-content-copied-correctly\n" * 64
GOOD2 = b"good-file-two-content-copied-correctly\n" * 64
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
        print("    [skip] iouring_source_vanish is io_uring-only")
        return True
    fuse = _fuse_bin()
    if fuse is None:
        print("    [skip] libfuse3/fusermount3 unavailable -> cannot fault the io_uring source openat")
        return True

    back = pathlib.Path("/tmp/uc-sv-back")
    mnt = pathlib.Path("/tmp/uc-sv-mnt")
    subprocess.run(["fusermount3", "-u", str(mnt)], stderr=subprocess.DEVNULL)
    for d in (back, mnt):
        shutil.rmtree(d, ignore_errors=True); d.mkdir(parents=True)
    (back / "victim.dat").write_bytes(NEW)
    (back / "good1.dat").write_bytes(GOOD1)
    (back / "good2.dat").write_bytes(GOOD2)

    fenv = dict(os.environ, UC_FUSE_BACKING=str(back), UC_FUSE_OPENFAIL="victim.dat")
    subprocess.run([str(fuse), str(mnt), "-s"], env=fenv, check=True)
    time.sleep(1)
    if subprocess.run(["mountpoint", "-q", str(mnt)]).returncode != 0:
        print("    [skip] FUSE mount failed (sandbox restriction)")
        return True
    try:
        dest = K.fresh_dest("sv_dest")
        copied = os.path.join(dest, mnt.name)
        os.makedirs(copied, exist_ok=True)
        pathlib.Path(copied, "victim.dat").write_bytes(OLD)   # pre-seed the colliding destination

        r = H.run(H.IO_URING, "cp", [str(mnt)], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                  expect_dir=None)

        vpath = pathlib.Path(copied, "victim.dat")
        vb = vpath.read_bytes() if vpath.exists() else b"<ABSENT>"
        victim_intact = (vb == OLD)
        g1 = pathlib.Path(copied, "good1.dat")
        g2 = pathlib.Path(copied, "good2.dat")
        good1 = g1.exists() and g1.read_bytes() == GOOD1
        good2 = g2.exists() and g2.read_bytes() == GOOD2
        ok = (r.completed and r.stayed_alive and victim_intact and good1 and good2
              and not r.oom_killed and r.mem_errors == 0)
        print(f"      victim_OLD_preserved={victim_intact} good1={good1} good2={good2} "
              f"completed={r.completed} alive={r.stayed_alive} mem_err={r.mem_errors}")
        if not victim_intact:
            print(f"        DATA LOSS: dest victim != OLD (size={len(vb)}); first 60 bytes: {vb[:60]!r}")
        print(f"    [iouring_source_vanish] {'PASS' if ok else 'FAIL'}")
        return ok
    finally:
        subprocess.run(["fusermount3", "-u", str(mnt)], stderr=subprocess.DEVNULL)
        subprocess.run(["pkill", "-9", "-x", "ultracopier"], capture_output=True)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
