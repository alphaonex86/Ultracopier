#!/usr/bin/env python3
"""Move: dest must match the source content AND the original sources must be gone."""
import sys, pathlib, os
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K


def run(backends=None, memcheck=H.NONE) -> bool:
    def one(backend):
        # Rebuild the source fresh for each backend (a move consumes it).
        src = synthtree.make_tree(K.fresh_src_root("move_src"), "default")
        # Keep a pristine reference copy to diff against (move deletes the original).
        ref = K.fresh_src_root("move_ref")
        import shutil
        if os.path.exists(ref):
            shutil.rmtree(ref)
        shutil.copytree(src, ref, symlinks=True)

        dest = K.fresh_dest("move_dest")
        r = H.run(backend, "mv", [src], dest,
                  file_collision=H.FileCollision.ASK,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=ref, memcheck=memcheck)

        # Original source tree must be removed by the move.
        moved_out = not os.path.exists(os.path.join(src, "tiny.txt"))
        if not (r.ok and r.content_ok and moved_out):
            print(f"      not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"content={r.content_ok} src_removed={moved_out} "
                  f"mem_errors={r.mem_errors}\n{r.diff_text}")
        return r.ok and r.content_ok and moved_out

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
