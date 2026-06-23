#!/usr/bin/env python3
"""Clean copy: copy a synthetic tree and verify content matches the source."""
import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K


def run(backends=None, memcheck=H.NONE) -> bool:
    src = synthtree.make_tree(K.fresh_src_root("copy_src"), "default")

    def one(backend):
        dest = K.fresh_dest("copy_dest")
        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.ASK,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=src, memcheck=memcheck)
        if not r.ok:
            print(f"      not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"content={r.content_ok} mem_errors={r.mem_errors}\n{r.diff_text}")
        return r.ok and r.content_ok

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
