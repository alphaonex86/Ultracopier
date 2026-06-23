#!/usr/bin/env python3
"""Path-length robustness: copy a tree containing the LENGTH extremes the engine's
PathTree storage must reproduce byte-for-byte -- a 255-byte (ext4 NAME_MAX) file name,
a 255-byte directory name, and a path nested deep enough that the cumulative length far
exceeds 255 bytes (over 1 KiB) while every component stays <=255, with a 255-byte name
at the very bottom of that deep path.

This is the on-disk half of the user request "file name more than 255 char, and path
more than 255 char". A single component CANNOT exceed 255 bytes on ext4 (ENAMETOOLONG),
so 255 is the storable maximum here; the genuinely >255-byte single component (and the
>4096 internal path) is an INTERNAL data-length concern exercised in the PathTree C++
unit test (test/unit/pathtree_test.cpp), mirroring how weird_names splits the embedded-NUL
invariant out of its on-disk set.

Asserts r.ok and r.content_ok: a clean `diff -rq --no-dereference` of the copied tree
against the source proves the engine interned every long component and resolve()d each
node back to the exact original path -- any off-by-one or truncation in the PathTree
join would corrupt a name or a directory and fail the diff. A length issue is a
metadata/scan concern shared by async and io_uring, so it runs on BOTH backends."""
import sys, pathlib, os
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
# When run directly Python prepends cases/ to sys.path, where cases/copy.py would SHADOW
# the stdlib `copy` module (imported transitively by dataclasses inside lib.harness).
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K


def run(backends=None, memcheck=H.NONE) -> bool:
    src = synthtree.make_tree(K.fresh_src_root("long_paths_src"), "long_paths")
    base = os.path.basename(src)

    def one(backend):
        K.with_scenario("")   # no fault injection; clear any scenario a prior case left set
        dest = K.fresh_dest("long_paths_dest")
        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.ASK,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=src, memcheck=memcheck)

        # Spot-check at the bytes level that the 255-byte name actually landed (the diff
        # already covers everything; this makes a truncation failure legible).
        copied = os.path.join(dest, base)
        names255_ok = False
        if os.path.isdir(copied):
            entries = os.listdir(os.fsencode(copied))
            names255_ok = any(len(e) == 255 for e in entries)

        if not (r.ok and r.content_ok and names255_ok):
            print(f"      not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"content={r.content_ok} name255_present={names255_ok} "
                  f"mem_errors={r.mem_errors}\n{r.diff_text}")
        return r.ok and r.content_ok and names255_ok

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
