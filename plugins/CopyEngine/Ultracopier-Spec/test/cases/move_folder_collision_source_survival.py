#!/usr/bin/env python3
"""MOVE x folderCollision Skip(2)/Rename(3): a skipped colliding FOLDER must leave the entire
source subtree intact -- the move-side mirror of collision_folder_modes (which is cp-only).

Why this is a data-safety tripwire (tree-level, worse than per-file): on Skip the scan lists
ZERO transfers for the folder, so the engine's "everything transferred" condition for the
source-folder cleanup (ActionType_MovePath -> rmpath) is reached immediately. If the
non-empty guard in that path ever regresses -- or the skip outcome is mistaken for
"moved" -- the WHOLE source subtree is destroyed with zero bytes copied. ../CLAUDE.md:
"A still-non-empty source directory means something was NOT moved ... and the directory
(and that content) MUST be kept."

Scenario (cross-filesystem: source on /tmp tmpfs, dest on the scratch fs -> the copy+delete
move path; checkDestinationFolder=true is REQUIRED to reach the Skip/Rename switch, exactly
as in collision_folder_modes):
  * Skip(2)   -> the colliding folder is not entered: the pre-existing <dest>/<base> keeps
                 ONLY its pre-seeded file, no renamed sibling appears, and the ENTIRE source
                 tree survives byte-identical (nothing was moved).
  * Rename(3) -> the destination is renamed aside and the source is moved INTO the new
                 sibling: the renamed sibling holds the full source tree, the original
                 <dest>/<base> keeps only its pre-seeded file, and the SOURCE tree is
                 removed (a completed move must clean up the source, folders bottom-up).
The two directions diverge observably both at the dest AND at the source, so neither could
pass for the wrong reason.
"""
import sys, pathlib, os, shutil, tempfile
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

SRC_NAME = "mfcs_src"   # dot-free so the "%name% - copy" rename is unambiguous

PREEXIST_REL = "marker_preexisting.txt"
PREEXIST_DATA = b"PRE-EXISTING destination file - must survive untouched\n"
SRC_FILE_REL = "marker_source.txt"
SRC_FILE_DATA = b"SOURCE file - must stay at source on Skip, move on Rename\n"
SRC_NESTED_REL = os.path.join("sub", "nested.dat")
SRC_NESTED_DATA = b"nested source payload - subtree survival marker\n"


def _make_source(tmp_parent: str) -> str:
    root = os.path.join(tmp_parent, SRC_NAME)
    os.makedirs(root, exist_ok=True)
    K.write_file(os.path.join(root, SRC_FILE_REL), SRC_FILE_DATA)
    K.write_file(os.path.join(root, SRC_NESTED_REL), SRC_NESTED_DATA)
    return root


def _src_intact(root: str) -> bool:
    return (os.path.isdir(root)
            and os.path.exists(os.path.join(root, SRC_FILE_REL))
            and K.read_file(os.path.join(root, SRC_FILE_REL)) == SRC_FILE_DATA
            and os.path.exists(os.path.join(root, SRC_NESTED_REL))
            and K.read_file(os.path.join(root, SRC_NESTED_REL)) == SRC_NESTED_DATA)


def run(backends=None, memcheck=H.NONE) -> bool:

    def one(backend):
        ok = True

        # ---- Skip(2): nothing moves; the ENTIRE source tree must survive ----
        tmp_parent = tempfile.mkdtemp(prefix="uc-mfcs-skip-", dir="/tmp")
        try:
            src = _make_source(tmp_parent)
            base = os.path.basename(src)
            dest = K.fresh_dest("mfcs_skip_dest")
            K.write_file(os.path.join(dest, base, PREEXIST_REL), PREEXIST_DATA)
            orig = os.path.join(dest, base)
            r1 = H.run(backend, "mv", [src], dest,
                       file_collision=H.FileCollision.OVERWRITE,
                       folder_collision=H.FolderCollision.SKIP,
                       folder_error=H.FolderError.SKIP,
                       extra_options={"checkDestinationFolder": "true"},
                       expect_dir=None, memcheck=memcheck)
            pre_survived = (os.path.exists(os.path.join(orig, PREEXIST_REL)) and
                            K.read_file(os.path.join(orig, PREEXIST_REL)) == PREEXIST_DATA)
            src_in_orig = os.path.exists(os.path.join(orig, SRC_FILE_REL))
            renamed_created = (sorted(os.listdir(dest)) != [base])
            src_ok = _src_intact(src)
            sub_ok = (r1.completed and r1.stayed_alive and r1.mem_errors == 0
                      and pre_survived and not src_in_orig and not renamed_created
                      and src_ok)
            if not sub_ok:
                print(f"      [mv-skip] FAIL: completed={r1.completed} alive={r1.stayed_alive} "
                      f"mem_err={r1.mem_errors} pre_survived={pre_survived} "
                      f"src_in_orig={src_in_orig} renamed_created={renamed_created} "
                      f"SOURCE_TREE_INTACT={src_ok}"
                      + ("  <-- TREE-LEVEL DATA LOSS" if not src_ok else ""))
            ok = ok and sub_ok
        finally:
            shutil.rmtree(tmp_parent, ignore_errors=True)

        # ---- Rename(3): moved into the renamed sibling; source must be cleaned up ----
        tmp_parent = tempfile.mkdtemp(prefix="uc-mfcs-ren-", dir="/tmp")
        try:
            src = _make_source(tmp_parent)
            base = os.path.basename(src)
            dest = K.fresh_dest("mfcs_ren_dest")
            K.write_file(os.path.join(dest, base, PREEXIST_REL), PREEXIST_DATA)
            orig = os.path.join(dest, base)
            r2 = H.run(backend, "mv", [src], dest,
                       file_collision=H.FileCollision.OVERWRITE,
                       folder_collision=H.FolderCollision.RENAME,
                       folder_error=H.FolderError.SKIP,
                       extra_options={"checkDestinationFolder": "true"},
                       expect_dir=None, memcheck=memcheck)
            orig_pre_ok = (os.path.exists(os.path.join(orig, PREEXIST_REL)) and
                           K.read_file(os.path.join(orig, PREEXIST_REL)) == PREEXIST_DATA)
            orig_clean = not os.path.exists(os.path.join(orig, SRC_FILE_REL))
            # Discover the renamed sibling by content (exact suffix is engine-internal).
            siblings = [d for d in os.listdir(dest)
                        if d != base and os.path.isdir(os.path.join(dest, d))]
            moved_ok = False
            for d in siblings:
                p = os.path.join(dest, d)
                if (os.path.exists(os.path.join(p, SRC_FILE_REL))
                        and K.read_file(os.path.join(p, SRC_FILE_REL)) == SRC_FILE_DATA
                        and os.path.exists(os.path.join(p, SRC_NESTED_REL))
                        and K.read_file(os.path.join(p, SRC_NESTED_REL)) == SRC_NESTED_DATA
                        and not os.path.exists(os.path.join(p, PREEXIST_REL))):
                    moved_ok = True
            # A COMPLETED move must have cleaned the source (all files moved -> dirs empty ->
            # removed bottom-up, root included).
            src_gone = not os.path.exists(src)
            sub_ok = (r2.completed and r2.stayed_alive and r2.mem_errors == 0
                      and orig_pre_ok and orig_clean and moved_ok and src_gone)
            if not sub_ok:
                leftover = []
                if os.path.isdir(src):
                    for dp, _dn, fn in os.walk(src):
                        leftover += [os.path.join(dp, f) for f in fn]
                print(f"      [mv-rename] FAIL: completed={r2.completed} alive={r2.stayed_alive} "
                      f"mem_err={r2.mem_errors} orig_pre_ok={orig_pre_ok} orig_clean={orig_clean} "
                      f"moved_ok={moved_ok} src_gone={src_gone} leftover={leftover[:5]}")
            ok = ok and sub_ok
        finally:
            shutil.rmtree(tmp_parent, ignore_errors=True)

        return ok

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
