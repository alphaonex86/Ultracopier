#!/usr/bin/env python3
"""Overwrite collision: pre-fill dest with a DIFFERENT file at a colliding path,
copy with file_collision=OVERWRITE, assert the final dest equals the source."""
import sys, pathlib, os
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K


def run(backends=None, memcheck=H.NONE, _mode="cp") -> bool:
    src = synthtree.make_tree(K.fresh_src_root("ovr_src"), "default")

    def one(backend):
        dest = K.fresh_dest(f"ovr_{_mode}_dest")
        base = os.path.basename(src)
        # Pre-place a DIFFERENT file where 'tiny.txt' will land (forces a collision).
        collide = os.path.join(dest, base, "tiny.txt")
        K.write_file(collide, b"STALE PRE-EXISTING CONTENT - must be overwritten\n")

        ref = src
        if _mode == "mv":
            # move CONSUMES the source. Re-make a FRESH src_live for THIS backend FIRST, then copy
            # IT (not the shared `src`) to the reference -- the previous backend's move already
            # deleted the shared `src` path ("ovr_src"), so copytree(src) raised FileNotFoundError
            # whenever both backends ran (it only "passed" in single-backend isolation).
            import shutil
            src_live = synthtree.make_tree(K.fresh_src_root("ovr_src"), "default")
            ref = K.fresh_src_root("ovr_ref")
            if os.path.exists(ref):
                shutil.rmtree(ref)
            shutil.copytree(src_live, ref, symlinks=True)
        else:
            src_live = src

        r = H.run(backend, _mode, [src_live], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=ref, memcheck=memcheck)
        # Direct check: the colliding file now holds the SOURCE content.
        final = K.read_file(collide) if os.path.exists(collide) else b""
        overwritten = (final == b"hello ultracopier\n")
        if not (r.ok and r.content_ok and overwritten):
            print(f"      not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"content={r.content_ok} overwritten={overwritten}\n{r.diff_text}")
        return r.ok and r.content_ok and overwritten

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
