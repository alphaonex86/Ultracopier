#!/usr/bin/env python3
"""fileCollision=RENAME + renameTheOriginalDestination=true, and the rename of the EXISTING
destination to its backup name FAILS -> the engine must NOT overwrite the original destination
(coverage-audit finding "RENAME collision handling fails when rename fails").

With renameTheOriginalDestination=true the engine renames the existing dest "victim.dat" to a backup
"victim - copy.dat" before writing the incoming file. checkAlwaysRename() (TransferThread.cpp ~686)
sets readError=true if that rename fails. BUT destinationExists() then returns FALSE (it returns false
whenever readError is set, ~line 355), so preOperation() continues PAST the (commented-out) readError
guard at async/TransferThreadAsync.cpp ~357, ifCanStartTransfer() clears readError, and the transfer
overwrites the original "victim.dat" with the incoming bytes -- destroying the user's only copy (the
backup was never made because the rename failed). The fix restores the readError early-return so a
failed backup-rename is reported (errorOnFile already fired) and the file is skipped, leaving the
original intact.

We force the backup rename to fail deterministically by pre-creating the backup NAME
("victim - copy.dat", the default "%name% - copy%suffix%" rule) as a NON-EMPTY DIRECTORY: rename() of
a file onto a non-empty directory fails, while a write to the original path would still succeed -- so
this isolates the rename failure (not a write failure).

REQUIRED: dest "victim.dat" still holds the OLD bytes (the original was NOT overwritten); the job
completes; no crash/leak.
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

OLD = b"OLD-original-destination-that-MUST-survive-a-failed-backup-rename\n" * 16
NEW = b"NEW-incoming-bytes-that-must-NOT-clobber-OLD-when-the-rename-fails\n" * 16


def _one(backend) -> bool:
    src = pathlib.Path(K.fresh_src_root("renfail_" + backend + "_src"))
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    (src / "victim.dat").write_bytes(NEW)

    dest = K.fresh_dest("renfail_" + backend + "_dest")
    base = os.path.basename(str(src))
    copied = pathlib.Path(dest, base)
    copied.mkdir(parents=True, exist_ok=True)
    (copied / "victim.dat").write_bytes(OLD)                 # the pre-existing dest to protect
    # Pre-create the DEFAULT backup name as a NON-EMPTY directory so rename(victim.dat, backup) fails.
    backup_dir = copied / "victim - copy.dat"
    backup_dir.mkdir()
    (backup_dir / "blocker").write_bytes(b"makes the dir non-empty so rename cannot replace it\n")

    r = H.run(backend, "cp", [str(src)], dest,
              file_collision=H.FileCollision.RENAME,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,                   # a failed rename -> error -> skip (keep OLD)
              folder_error=H.FolderError.SKIP,
              expect_dir=None,
              extra_options={"renameTheOriginalDestination": "true"})

    vpath = copied / "victim.dat"
    got = vpath.read_bytes() if vpath.exists() else b"<ABSENT>"
    old_preserved = (got == OLD)                             # OLD must NOT be clobbered by NEW
    ok = (r.completed and r.stayed_alive and old_preserved
          and not r.oom_killed and r.mem_errors == 0)
    print(f"      OLD_preserved={old_preserved} (got {'OLD' if got==OLD else 'NEW' if got==NEW else 'OTHER'}) "
          f"completed={r.completed} alive={r.stayed_alive} mem_err={r.mem_errors}")
    if not old_preserved:
        print("        DATA LOSS: a FAILED backup-rename let the engine overwrite the original destination")
    print(f"    [collision_rename_fail:{backend}] {'PASS' if ok else 'FAIL'}")
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    # The RENAME collision handling (checkAlwaysRename / destinationExists / preOperation) is shared by
    # async AND the io_uring/IOCP pipelined backend, so the data-loss + its guard must hold on both.
    bks = backends or [H.ASYNC, H.IO_URING]
    avail = []
    for b in bks:
        try:
            H.binary_for(b, H.load_config(), asan=(memcheck == H.SANITIZE))
            avail.append(b)
        except Exception:
            pass
    return all(_one(b) for b in avail) if avail else False


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
