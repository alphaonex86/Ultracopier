#!/usr/bin/env python3
"""MOVE a folder in which ONE nested file is unreadable -> the failed file's SOURCE must be
preserved, and the source folder must NOT be recursively removed (coverage-audit finding
"move_folder_partial_failure").

MOVE = copy + delete. The per-file guard (TransferThreadAsync copyFailed) already keeps the source of
a file whose copy failed. The question this case settles empirically: after every transfer is
PROCESSED, ListThread_InodeAction.cpp ActionType_MovePath launches rmpath() on the SOURCE folder when
actionToDoListTransferEmpty() is true -- i.e. whether or not every file SUCCEEDED. If rmpath force-
removed a non-empty tree, the skipped file's source (still sitting in the source folder) would be
destroyed = DATA LOSS.

Reading MkPath::rmpath shows it refuses to delete a directory that still contains a FILE (it only
rmdir's empty trees: on encountering a regular file it sets allHaveWork=false and bails), so the
skipped file's source SHOULD survive. This test asserts that REQUIRED safe behaviour; if it ever goes
red, ActionType_MovePath has started destroying partially-moved source folders and must be guarded.

Scenario (async; readfail faults the libc source read only -- io_uring opens via io_uring_prep_openat):
  * source folder has a readable good.dat + an unreadable BADSECTOR file,
  * mode = MOVE, fileError = SKIP (skip the unreadable file, keep going),
  * REQUIRED: the unreadable file's SOURCE still exists (folder not nuked); good.dat moved to dest
    (its source gone); the job completes; no crash/leak.
"""
import os
import sys
import shutil
import tempfile
import pathlib

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

GOOD = b"good-file-fully-moved\n" * 64
BAD = b"unreadable-file-source-must-survive\n" * 64


def run(backends=None, memcheck=H.NONE) -> bool:
    # Source on /tmp so it is on a DIFFERENT filesystem than the scratch dest -> the MOVE is a real
    # copy+delete (realMove=false) with a per-file READ (which readfail can fault); a same-fs folder
    # move would be an atomic rename with no read and would not exercise the partial-failure path.
    src = pathlib.Path(tempfile.mkdtemp(prefix="uc-movefolder-src-", dir="/tmp")) / "movefolder_src"
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    (src / "good.dat").write_bytes(GOOD)
    (src / "BADSECTOR_keep.dat").write_bytes(BAD)        # readfail target (source read only)

    dest = K.fresh_dest("movefolder_dest")
    base = os.path.basename(str(src))
    copied = os.path.join(dest, base)

    K.with_scenario("readfail:BADSECTOR")                # the unreadable file's SOURCE read -> EIO
    r = H.run(H.ASYNC, "mv", [str(src)], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,               # skip the unreadable file, keep going
              folder_error=H.FolderError.SKIP,
              expect_dir=None,
              fs_preload=K.fs_so())
    K.with_scenario("")

    bad_src = src / "BADSECTOR_keep.dat"
    bad_src_survived = bad_src.exists() and bad_src.read_bytes() == BAD   # DATA must NOT be lost
    good_moved = (os.path.exists(os.path.join(copied, "good.dat"))
                  and open(os.path.join(copied, "good.dat"), "rb").read() == GOOD)
    good_src_gone = not (src / "good.dat").exists()      # a real MOVE deleted the moved source

    ok = (r.completed and r.stayed_alive and bad_src_survived and good_moved
          and not r.oom_killed and r.mem_errors == 0)
    print(f"      bad_source_survived={bad_src_survived} good_moved={good_moved} "
          f"good_source_gone={good_src_gone} completed={r.completed} alive={r.stayed_alive} "
          f"mem_err={r.mem_errors}")
    if not bad_src_survived:
        print("        DATA LOSS: the unreadable file's source folder was removed (skipped file lost)")
    print(f"    [move_folder_partial] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
