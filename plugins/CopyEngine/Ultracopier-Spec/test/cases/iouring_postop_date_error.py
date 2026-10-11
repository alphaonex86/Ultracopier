#!/usr/bin/env python3
"""io_uring: a post-operation failure (keepDate on, the destination refuses the mtime) must be
decided by the error policy on THAT file -- not reported and completed at the same time.

checkIfAllIsClosedAndDoOperations() ran doFilePostOperation(), which emitted errorOnFile and
returned false, and then went on to Idle + postOperationStopped anyway. The ListThread immediately
handed the thread its NEXT file; the error reached CopyEngine two queued hops later and the policy's
skip() landed on that next file -> silently dropped (put-to-end: the wrong entry requeued). A NAS /
uid-squash mount that stores the bytes but refuses utimensat (EPERM) does this on every file.

Rig: dest on a FUSE mount whose utimens() fails EPERM for every *NODATE* file (data writes fine),
keepDate=true (shipping default), fileError=Skip, 1 inode thread, ten victims INTERLEAVED with ten
good files by name so that each misdirected skip has a good file queued right behind it (the race
between the main thread's error handling and the ListThread's next dispatch falls either way on
one pair; over ten pairs the drop is all but certain). Asserts: completed, EVERY file is present
and byte-identical (a refused date is not a reason to destroy a good copy or drop its neighbour),
no crash."""
import sys, os, pathlib, filecmp
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K

N_PAIRS = 10


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IO_URING not in backends:
        print("    [skip] iouring_postop_date_error is io_uring-only")
        return True
    with K.FuseMount("utimefail", UC_FUSE_UTIMEFAIL="NODATE") as m:
        if m is None:
            print("    [skip] libfuse3/fusermount3 unavailable or FUSE mount failed")
            return True
        src = pathlib.Path(K.fresh_dest("utimefail_src"))
        names = []
        for i in range(N_PAIRS):
            names += [f"f{i:02d}a_NODATE.dat", f"f{i:02d}b_good.dat"]   # victim, then the good file queued behind it
        for i, n in enumerate(names):
            (src / n).write_bytes(synthtree._seeded_bytes_fast(900 + i, 100 * 1024 + i))
        (m.back / "dest").mkdir()
        r = H.run(H.IO_URING, "cp", [str(src)], str(m.mnt / "dest"), file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE, file_error=H.FileError.SKIP, keep_date=True,
                  expect_dir=None, inode_threads=1)
        copied = m.back / "dest" / src.name
        missing = [n for n in names if not ((copied / n).exists()
                                            and filecmp.cmp(str(src / n), str(copied / n), shallow=False))]
    goods_dropped = [n for n in missing if "good" in n]
    victims_bad = [n for n in missing if "NODATE" in n]
    ok = r.completed and r.stayed_alive and not missing and r.mem_errors == 0 and not r.oom_killed
    print(f"    [io_uring post-op date error] completed={r.completed} alive={r.stayed_alive} "
          f"goods_dropped={len(goods_dropped)}/{N_PAIRS} {goods_dropped[:3]} victims_not_complete={len(victims_bad)} "
          f"mem_errors={r.mem_errors} -> {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
