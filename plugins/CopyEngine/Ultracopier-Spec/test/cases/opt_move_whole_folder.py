#!/usr/bin/env python3
"""Option verification: moveTheWholeFolder (key "moveTheWholeFolder").

This is a MOVE optimisation in SHARED engine code, NOT a per-backend feature:
ScanFileOrFolder.cpp:296 fires `addToRealMove(source, dest)` iff
    moveTheWholeFolder  AND  mode==Move  AND  the dest folder does NOT yet exist
    AND  driveManagement.isSameDrive(source, dest)
That action becomes ActionType_RealMove, which MkPath.cpp:368 executes as a SINGLE
`TransferThread::rename(source, dest)` (POSIX ::rename / Win32 MoveFileExW) -- a
metadata-only rename of the whole directory. The folder object is NOT recreated:

  * moveTheWholeFolder=TRUE  -> the destination folder is the SAME inode as the
    original source folder (it was renamed, not rebuilt file-by-file). Source gone.
  * moveTheWholeFolder=FALSE -> ScanFileOrFolder recurses via listFolder(); MkPath
    mkdir()s a BRAND-NEW destination folder and the engine moves each file into it.
    The destination folder is a DIFFERENT inode. Source gone.

Both states must end with identical CONTENT and the source removed; the ONLY
observable that diverges is the destination-folder inode identity vs. the original
source-folder inode. We capture the source folder's inode BEFORE the move (a move
consumes the source) and assert BOTH directions, so neither side can pass for the
wrong reason:

    on  : dest_folder.st_ino == original_src_folder.st_ino   (renamed)
    off : dest_folder.st_ino != original_src_folder.st_ino   (recreated)

The off-state difference is DETERMINISTIC (not a probabilistic "inodes usually
differ"): in the file-by-file path MkPath creates the destination directory while
the source directory still exists and still holds its files (the source dir is only
rmdir'd at the very end, via ActionType_MovePath cleanup). The source inode is
therefore still allocated at dest-creation time, so the freshly mkdir'd dest cannot
reuse that inode number -- the two inodes MUST differ.

SAME-FILESYSTEM REQUIREMENT: isSameDrive() must be true or the whole-folder rename
is skipped even with the option on. Source and destination both live under the test
scratch root (DESTINATIONLINUX), so they are on one filesystem -- the precondition
the option needs.

IOCP: iocp_one=None (recorded SKIP), with justification. The whole-folder-rename
logic is SHARED code (ScanFileOrFolder.cpp / MkPath.cpp) and runs identically on the
IOCP backend -- on NTFS MoveFileExW on the same volume is a metadata-only rename that
preserves the NTFS FileId (the inode analogue). But the *divergence* this case checks
is "dest folder reuses the SOURCE folder's id", which requires snapshotting the source
folder's FileId BEFORE the move. winlane (the IOCP lane) stages a fresh source on the
box per run and exposes NO pre-run hook -- post_verify runs only AFTER the move, when
the source is already gone -- so the rename-vs-recreate identity divergence is NOT
observable through the existing (unmodifiable) winlane API. A run-to-run FileId/creation
-time comparison would NOT be divergent (a recreated dir and a renamed dir both get a
"recent" creation time from the just-staged source), so it cannot stand in. Faking it
or weakening it is forbidden; the option's CORRECTNESS on IOCP (move completes, content
correct, source removed) is already exercised by cases/iocp_parity.py's `move` sub-run,
so coverage is not lost. Hence None, not a vacuous green.
"""
import sys, pathlib, os, shutil
# Strip cases/ from sys.path so stdlib imports are not shadowed by sibling case files
# when this file is run directly (mirrors opt_perms_dates.py / windows_ads.py).
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K


def _fresh_src_and_ref(src_name, ref_name):
    """Build a fresh synthetic source (a move consumes it) plus a pristine reference
    copy to diff the moved tree against. Returns (src, ref, src_folder_inode)."""
    src = synthtree.make_tree(K.fresh_src_root(src_name), "default")
    ref = K.fresh_src_root(ref_name)
    if os.path.exists(ref):
        shutil.rmtree(ref)
    shutil.copytree(src, ref, symlinks=True)
    src_ino = os.stat(src).st_ino        # the source FOLDER's inode, captured BEFORE the move
    return src, ref, src_ino


def run(backends=None, memcheck=H.NONE) -> bool:

    def one(backend):
        ok = True

        # ---- moveTheWholeFolder = TRUE : dest folder is the SAME inode (renamed) -------
        src, ref, src_ino = _fresh_src_and_ref("mwf_on_src", "mwf_on_ref")
        dest = K.fresh_dest("mwf_on_dest")          # dest dir exists but dest/<base> does NOT yet
        base = os.path.basename(src)
        r = H.run(backend, "mv", [src], dest,
                  file_collision=H.FileCollision.ASK,
                  folder_collision=H.FolderCollision.MERGE,
                  extra_options={"moveTheWholeFolder": "true"},
                  expect_dir=ref, memcheck=memcheck)
        copied = os.path.join(dest, base)
        src_gone = not os.path.exists(src)
        dest_ino = os.stat(copied).st_ino if os.path.isdir(copied) else None
        renamed = (dest_ino is not None and dest_ino == src_ino)
        if not (r.ok and r.content_ok and src_gone and renamed):
            print(f"      [on] not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"content={r.content_ok} src_gone={src_gone} mem_errors={r.mem_errors}\n"
                  f"      [on] expected dest folder to REUSE source inode (whole-folder rename): "
                  f"src_ino={src_ino} dest_ino={dest_ino} renamed={renamed}\n{r.diff_text}")
            ok = False
        else:
            print(f"      [on] whole-folder rename: dest inode == source inode "
                  f"({dest_ino}) -- folder was renamed, not recreated -- correct")

        # ---- moveTheWholeFolder = FALSE : dest folder is a DIFFERENT inode (recreated) --
        src2, ref2, src2_ino = _fresh_src_and_ref("mwf_off_src", "mwf_off_ref")
        dest2 = K.fresh_dest("mwf_off_dest")
        base2 = os.path.basename(src2)
        r2 = H.run(backend, "mv", [src2], dest2,
                   file_collision=H.FileCollision.ASK,
                   folder_collision=H.FolderCollision.MERGE,
                   extra_options={"moveTheWholeFolder": "false"},
                   expect_dir=ref2, memcheck=memcheck)
        copied2 = os.path.join(dest2, base2)
        src2_gone = not os.path.exists(src2)
        dest2_ino = os.stat(copied2).st_ino if os.path.isdir(copied2) else None
        recreated = (dest2_ino is not None and dest2_ino != src2_ino)
        if not (r2.ok and r2.content_ok and src2_gone and recreated):
            print(f"      [off] not ok: completed={r2.completed} alive={r2.stayed_alive} "
                  f"content={r2.content_ok} src_gone={src2_gone} mem_errors={r2.mem_errors}\n"
                  f"      [off] expected dest folder to be a NEW inode (file-by-file recreate): "
                  f"src_ino={src2_ino} dest_ino={dest2_ino} recreated={recreated}\n{r2.diff_text}")
            ok = False
        else:
            print(f"      [off] file-by-file move: dest inode ({dest2_ino}) != source inode "
                  f"({src2_ino}) -- folder was recreated, not renamed -- correct")

        return ok

    # IOCP: None -> recorded SKIP (the inode-identity divergence is not observable through
    # winlane; see the module docstring). The option's correctness on IOCP is covered by
    # cases/iocp_parity.py's `move` sub-run.
    return K.for_option_backends(backends, one, iocp_one=None)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
