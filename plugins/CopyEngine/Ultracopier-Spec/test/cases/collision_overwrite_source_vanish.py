#!/usr/bin/env python3
"""Collision OVERWRITE + the source file VANISHES between scan and transfer (TOCTOU) must NOT
destroy the pre-existing destination (coverage-audit finding #9 -- FIXED).

This was a CONFIRMED DATA-LOSS bug in the shared/async transfer core and is now FIXED; the test is
GREEN and guards the fix against regression. The bug: on an OVERWRITE the engine (a) truncated the
destination to 0 at open and (b) unlink()'d it on the skip/stop cleanup -- BOTH before the source was
confirmed readable -- so a source that vanished before the first write destroyed the user's only
surviving copy. cp/rsync open the source FIRST and leave the dest untouched on failure. The fix
(async/WriteThread.cpp + async/TransferThreadAsync.cpp): skip the open-time destTruncate for a fresh
overwrite (startSize==0) -- the close-time truncate-to-written-size handles the final size -- and
guard every dest-unlink-on-error site with destinationIsOursToRemove(), where the destination counts
as the user's untouched data only when it pre-existed with NON-EMPTY content AND we wrote 0 bytes
(getDestinationPreExisted() = is_file && size>0; the NON-EMPTY part is what keeps the put-to-end
retry's own 0-byte file removable -- regression #1 of the first attempt -- and file_error_dialog
green).

Scenario (ASYNC only -- the `gone` verb is a libc open() -> ENOENT; the io_uring backend opens via
io_uring_prep_openat and bypasses libc, like faulty_hdd / write_faults):
  * the destination victim is pre-seeded with OLD content (forces a collision),
  * fileCollision=OVERWRITE (the engine intends to replace it),
  * the victim's SOURCE open() returns ENOENT at transfer time (gone:<full-src-path>) -- it was
    listed (stat OK at scan) then removed before the transfer opened it,
  * fileError=SKIP.

REQUIRED (now asserted GREEN): the destination victim STILL holds the OLD bytes byte-for-byte; the
other (good) files copy correctly; the job completes; no crash/leak. A CLEAN run still correctly
replaces OLD->NEW (the collision/overwrite path is unchanged for a readable source).
"""
import os
import sys
import shutil
import pathlib

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

OLD = b"OLD-destination-content-that-MUST-survive-the-vanished-source\n" * 16
NEW = b"NEW-source-content-that-never-arrives-because-the-source-vanished\n" * 16
GOOD1 = b"good-file-one-content-copied-correctly\n" * 64
GOOD2 = b"good-file-two-content-copied-correctly\n" * 64


def run(backends=None, memcheck=H.NONE) -> bool:
    src = pathlib.Path(K.fresh_src_root("ovw_vanish_src"))
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    (src / "victim.dat").write_bytes(NEW)        # source the engine will try (and fail) to read
    (src / "good1.dat").write_bytes(GOOD1)
    (src / "good2.dat").write_bytes(GOOD2)

    dest = K.fresh_dest("ovw_vanish_dest")
    base = os.path.basename(str(src))
    copied = os.path.join(dest, base)
    os.makedirs(copied, exist_ok=True)
    pathlib.Path(copied, "victim.dat").write_bytes(OLD)   # pre-seed the colliding dest

    # Match ONLY the source victim's open -- use its FULL path. The dest copied path is
    # .../ovw_vanish_dest/<base>/victim.dat (different prefix), so this substring does NOT match the
    # destination open. (Matching the bare "victim" would also ENOENT the dest open = a test artifact.)
    K.with_scenario(f"gone:{src}/victim.dat")    # victim's SOURCE open -> ENOENT at transfer time
    r = H.run(H.ASYNC, "cp", [str(src)], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,
              folder_error=H.FolderError.SKIP,
              expect_dir=None,
              fs_preload=K.fs_so())
    K.with_scenario("")

    vpath = pathlib.Path(copied, "victim.dat")
    victim_bytes = vpath.read_bytes() if vpath.exists() else b"<ABSENT>"
    victim_intact = (victim_bytes == OLD)        # OLD preserved, NOT truncated/corrupted/NEW
    g1 = pathlib.Path(copied, "good1.dat")
    g2 = pathlib.Path(copied, "good2.dat")
    good1_ok = g1.exists() and g1.read_bytes() == GOOD1
    good2_ok = g2.exists() and g2.read_bytes() == GOOD2

    ok = (r.completed and r.stayed_alive and victim_intact and good1_ok and good2_ok
          and not r.oom_killed and r.mem_errors == 0)
    print(f"      victim_OLD_preserved={victim_intact} good1={good1_ok} good2={good2_ok} "
          f"completed={r.completed} alive={r.stayed_alive} mem_err={r.mem_errors}")
    if not victim_intact:
        print(f"        DATA LOSS: dest victim != OLD; first 70 bytes: {victim_bytes[:70]!r}")
    print(f"    [collision_overwrite_source_vanish] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
