#!/usr/bin/env python3
"""Operation-decomposition INTEGRITY: no duplicate op, no forgotten op.

The copy engine decomposes each file/folder copy into basic FS operations (mkdir/open/read/write/
close/chmod/utime). Whatever ORDER the engine (or a future pipelining scheduler) runs them in, the
SET of ops must be right:

  * NO DUPLICATE op -- a dir created once, a file create-opened once, no byte range written twice,
    a file's perm/date set once. A duplicate means wasted I/O or, worse, a double copy.
  * NO FORGOTTEN op -- every source file yields a dest create-open + its FULL-size writes + a close;
    every needed directory is mkdir'd. Nothing silently dropped.

Observed from OUTSIDE via the LD_PRELOAD op trace (see lib/optrace.py) -- no engine code is touched.

SCOPE: the shim sees libc, so on ASYNC the whole decomposition (open/read/write/close + metadata) is
checked. On io_uring the data + open/close go through the ring and are invisible, so only the shared
metadata path (mkdir + no-duplicate-mkdir) is asserted there (check_data=False). IOCP's libc trace
does not exist on Linux; IOCP integrity is covered by the content diff + iocp_parity on the laptop.
"""
import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K


def _check(backend):
    # These cases validate the COPY decomposition (open/read/write/close + mkdir/chmod/utime). A
    # same-FS MOVE takes the whole-tree rename() fast-path -- a single atomic metadata op with NO
    # decomposition to check -- so it is orthogonal here (move op-safety is covered by the dedicated
    # move_* cases). The copy path is exactly what the future pipelining scheduler reorders.
    src = K.fresh_src_root("opsint_src")
    files, dirs = K.make_optrace_tree(src)

    ok = True
    dest = K.fresh_dest("opsint_dest_cp")
    check_data = (backend == H.ASYNC)   # data plane only visible on async
    r, t, exp_files, exp_dirs = K.run_traced(
        backend, "cp", src, dest, files, dirs,
        file_collision=H.FileCollision.OVERWRITE, folder_collision=H.FolderCollision.MERGE,
        expect_dir=src)
    if not (r.ok and r.content_ok):
        print(f"      [cp] run not ok: completed={r.completed} alive={r.stayed_alive} "
              f"content={r.content_ok} mem_errors={r.mem_errors}\n{r.diff_text}")
        ok = False
    dup = t.problems_no_duplicate(check_data=check_data)
    miss = t.problems_no_missing(exp_files, exp_dirs, check_data=check_data)
    if dup:
        print("      [cp] DUPLICATE ops:")
        for p in dup:
            print(f"        - {p}")
        ok = False
    if miss:
        print("      [cp] FORGOTTEN/incomplete ops:")
        for p in miss:
            print(f"        - {p}")
        ok = False
    # sanity: the trace must not be empty (proves the shim + env plumbing actually fired)
    if len(t.events) == 0:
        print("      [cp] op trace EMPTY -- shim not active? (would make the check vacuous)")
        ok = False
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    return K.for_backends(backends, _check)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
