#!/usr/bin/env python3
"""Operation-decomposition DEPENDENCY ordering (reorder is allowed, data dependencies are NOT).

A pipelining scheduler may run the basic FS ops in any order and across parallel pools -- EXCEPT it
must never violate a real filesystem data dependency. This case asserts, from the op trace, that
however the engine ordered things, these hard edges hold:

  * a file's create-open comes BEFORE any write to it, and every read/write is BETWEEN its open and
    its close -- never a data op after close (or before open);
  * a file's set-perm / set-date happens AFTER that file's close (a later write would re-touch mtime);
  * a FOLDER's FINAL set-date / set-perm happens AFTER every file under it is written and closed --
    because on some FS/OS `close` bumps the parent directory mtime, so whatever folder-date runs LAST
    must be after all its files, or the stored folder time is wrong.

This is the property the future pipelining rewrite must preserve; run against the current engine it
sets the baseline (the stable async reference already satisfies it -- see CLAUDE.md "The pipelining
design"). Observed from OUTSIDE via the LD_PRELOAD op trace; no engine code is touched.

SCOPE: full on ASYNC (open/read/write/close visible). On io_uring the data + open/close go through
the ring (invisible), so only the metadata-subset (mkdir presence, no duplicate) applies there.
"""
import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K


def _check(backend):
    # Validates the COPY decomposition's ordering. A same-FS move is one atomic rename() (no
    # decomposition), so it is out of scope here (see move_* cases); copy is what the pipelining reorders.
    check_data = (backend == H.ASYNC)
    src = K.fresh_src_root("opsdep_src")
    files, dirs = K.make_optrace_tree(src)
    dest = K.fresh_dest("opsdep_dest")
    r, t, _ef, _ed = K.run_traced(
        backend, "cp", src, dest, files, dirs,
        file_collision=H.FileCollision.OVERWRITE, folder_collision=H.FolderCollision.MERGE,
        expect_dir=src)
    ok = True
    if not (r.ok and r.content_ok):
        print(f"      [cp] run not ok: completed={r.completed} alive={r.stayed_alive} "
              f"content={r.content_ok} mem_errors={r.mem_errors}")
        ok = False
    probs = t.problems_dependency(check_data=check_data)
    if probs:
        print("      [cp] FS dependency-order violations:")
        for p in probs:
            print(f"        - {p}")
        ok = False
    if check_data and len(t.segments) == 0:
        print("      [cp] op trace has no open/close segments -- shim not active on data plane?")
        ok = False
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    return K.for_backends(backends, _check)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
