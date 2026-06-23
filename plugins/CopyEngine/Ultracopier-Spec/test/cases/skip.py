#!/usr/bin/env python3
"""SKIP collision: pre-fill dest with different content; that file must be KEPT
(not overwritten) while the rest of the tree still copies."""
import sys, pathlib, os
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K

KEEP_NAME = "tiny.txt"
KEEP_DATA = b"OLD CONTENT THAT MUST SURVIVE THE SKIP\n"


def run(backends=None, memcheck=H.NONE) -> bool:
    src = synthtree.make_tree(K.fresh_src_root("skip_src"), "default")
    base = os.path.basename(src)

    def one(backend):
        dest = K.fresh_dest("skip_dest")
        keep = os.path.join(dest, base, KEEP_NAME)
        K.write_file(keep, KEEP_DATA)   # different content -> collision -> skipped

        # Build the expectation tree: source, but with the skipped file replaced by
        # the OLD content (since SKIP leaves the pre-existing file in place).
        import shutil
        exp = K.fresh_src_root("skip_expect")
        if os.path.exists(exp):
            shutil.rmtree(exp)
        shutil.copytree(src, exp, symlinks=True)
        pathlib.Path(exp, KEEP_NAME).write_bytes(KEEP_DATA)

        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.SKIP,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=exp, memcheck=memcheck)

        kept = (os.path.exists(keep) and K.read_file(keep) == KEEP_DATA)
        # A non-colliding file should still have copied.
        other = os.path.join(dest, base, "few_kb.dat")
        other_copied = os.path.exists(other)
        if not (r.ok and r.content_ok and kept and other_copied):
            print(f"      not ok: completed={r.completed} content={r.content_ok} "
                  f"kept={kept} other_copied={other_copied}\n{r.diff_text}")
        return r.ok and r.content_ok and kept and other_copied

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
