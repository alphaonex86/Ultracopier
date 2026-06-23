#!/usr/bin/env python3
"""MERGE folder collision: pre-create PART of the dest tree, then copy; the result
must be the UNION (pre-existing files survive, source files are added in)."""
import sys, pathlib, os
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K

EXTRA_REL = os.path.join("a", "b", "preexisting.txt")   # in a dir the source also uses
EXTRA_DATA = b"this file pre-existed and must survive the merge\n"


def run(backends=None, memcheck=H.NONE) -> bool:
    src = synthtree.make_tree(K.fresh_src_root("merge_src"), "default")
    base = os.path.basename(src)

    def one(backend):
        dest = K.fresh_dest("merge_dest")
        # Pre-create the destination folder <dest>/<base>/ with one extra file inside
        # a subdir that the source ALSO populates -> exercises folder merge.
        extra = os.path.join(dest, base, EXTRA_REL)
        K.write_file(extra, EXTRA_DATA)

        # Expectation = source UNION the extra pre-existing file.
        import shutil
        exp = K.fresh_src_root("merge_expect")
        if os.path.exists(exp):
            shutil.rmtree(exp)
        shutil.copytree(src, exp, symlinks=True)
        K.write_file(os.path.join(exp, EXTRA_REL), EXTRA_DATA)

        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=exp, memcheck=memcheck)

        union_ok = (os.path.exists(extra) and K.read_file(extra) == EXTRA_DATA and
                    os.path.exists(os.path.join(dest, base, "tiny.txt")))
        if not (r.ok and r.content_ok and union_ok):
            print(f"      not ok: completed={r.completed} content={r.content_ok} "
                  f"union_ok={union_ok}\n{r.diff_text}")
        return r.ok and r.content_ok and union_ok

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
