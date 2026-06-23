#!/usr/bin/env python3
"""Symlinks: copy the 'symlinks' profile and require a clean
diff -rq --no-dereference (symlink targets recreated verbatim, dangling preserved,
escaping link preserved). Covered by r.content_ok."""
import sys, pathlib, os
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K


def run(backends=None, memcheck=H.NONE) -> bool:
    src = synthtree.make_tree(K.fresh_src_root("sym_src"), "symlinks")
    base = os.path.basename(src)

    def one(backend):
        dest = K.fresh_dest("sym_dest")
        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.ASK,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=src, memcheck=memcheck)

        # Spot-check: the dangling symlink must be recreated AS a symlink (not chased).
        dangling = os.path.join(dest, base, "links", "dangling")
        dangling_ok = os.path.islink(dangling)
        if not (r.ok and r.content_ok and dangling_ok):
            print(f"      not ok: completed={r.completed} content={r.content_ok} "
                  f"dangling_is_symlink={dangling_ok}\n{r.diff_text}")
        return r.ok and r.content_ok and dangling_ok

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
