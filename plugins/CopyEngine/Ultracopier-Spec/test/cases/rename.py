#!/usr/bin/env python3
"""RENAME collision: pre-fill dest at a colliding path; the original dest file must
be KEPT and a renamed copy of the source must appear alongside it."""
import sys, pathlib, os, glob
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K

COLLIDE_NAME = "tiny.txt"
SRC_DATA = b"hello ultracopier\n"
OLD_DATA = b"PRE-EXISTING - keep me, the source must be renamed\n"


def run(backends=None, memcheck=H.NONE) -> bool:
    src = synthtree.make_tree(K.fresh_src_root("ren_src"), "default")
    base = os.path.basename(src)

    def one(backend):
        dest = K.fresh_dest("ren_dest")
        target_dir = os.path.join(dest, base)
        original = os.path.join(target_dir, COLLIDE_NAME)
        K.write_file(original, OLD_DATA)

        # No expect_dir diff: a renamed extra file legitimately makes dest != source.
        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.RENAME,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=None, memcheck=memcheck)

        # Original kept untouched.
        kept = (os.path.exists(original) and K.read_file(original) == OLD_DATA)
        # A renamed copy of the source exists (tiny*.txt other than the original,
        # holding the SOURCE content).
        stem, ext = os.path.splitext(COLLIDE_NAME)
        candidates = [p for p in glob.glob(os.path.join(target_dir, f"{stem}*{ext}"))
                      if os.path.abspath(p) != os.path.abspath(original)]
        renamed = any(K.read_file(p) == SRC_DATA for p in candidates)
        ran = r.completed and r.stayed_alive and r.mem_errors == 0
        if not (ran and kept and renamed):
            print(f"      not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"kept={kept} renamed={renamed} candidates={candidates}")
        return ran and kept and renamed

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
