#!/usr/bin/env python3
"""io_uring: the post-open size query of the SOURCE fails -> the file must be reported as an ERROR,
never copied as an EMPTY file that is counted complete.

TransferThreadUring::openSourceFile() fstat()s the fd it just got from the ring to learn the size
to read. A failing fstat (a flaky network / FUSE source answering the metadata query late) used
to leave sourceFileSize=0 -> the pipeline read nothing, "0 == 0 bytes transferred", the file was
reported COMPLETE and the destination was a 0-byte wreck (on a MOVE the source would then be
deleted). Same shape as GetFileSizeEx() on IOCP.

Rig: the LD_PRELOAD shim verb `fstatfail:<substr>:<n>` fails ONLY the fd-based fstat (the fd path is
resolved through /proc/self/fd, so it reaches an fd opened by io_uring openat); every path-based
stat keeps working, so the scan lists the file normally. n=2 -> the first two attempts fail, then
succeed, and fileError=Ask + the FileErrorDialogHook answering Retry proves the strong contract:
the file ends up byte-identical through the error policy (never a phantom empty file)."""
import sys, os, pathlib, filecmp
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K

VICTIM = "meta_FSTATFAIL.bin"


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IO_URING not in backends:
        print("    [skip] iouring_fstat_fail is io_uring-only")
        return True
    src = pathlib.Path(K.fresh_dest("fstatfail_src"))
    (src / VICTIM).write_bytes(synthtree._seeded_bytes_fast(31, 300 * 1024 + 17))
    for i in range(4):
        (src / f"sibling_{i}.bin").write_bytes(synthtree._seeded_bytes_fast(60 + i, 40 * 1024 + i))
    dest = K.fresh_dest("fstatfail_dest")
    copied = pathlib.Path(dest) / src.name
    K.with_scenario(f"fstatfail:{VICTIM}:2")
    os.environ["ULTRACOPIER_TEST_FILE_ERROR_ACTION"] = "retry"
    try:
        r = H.run(H.IO_URING, "cp", [str(src)], dest, file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE, file_error=H.FileError.ASK,
                  keep_date=False, do_right=False, expect_dir=None,
                  fs_preload=K.fs_so(), inode_threads=1)
    finally:
        K.with_scenario("")
        os.environ.pop("ULTRACOPIER_TEST_FILE_ERROR_ACTION", None)
    vd = copied / VICTIM
    victim_size = vd.stat().st_size if vd.exists() else -1
    victim_ok = vd.exists() and filecmp.cmp(str(src / VICTIM), str(vd), shallow=False)
    siblings_ok = all((copied / f"sibling_{i}.bin").exists()
                      and filecmp.cmp(str(src / f"sibling_{i}.bin"), str(copied / f"sibling_{i}.bin"), shallow=False)
                      for i in range(4))
    ok = r.completed and r.stayed_alive and victim_ok and siblings_ok and r.mem_errors == 0 and not r.oom_killed
    print(f"    [io_uring fstat fail] completed={r.completed} alive={r.stayed_alive} victim_identical={victim_ok} "
          f"victim_size={victim_size}/{(src / VICTIM).stat().st_size} siblings_ok={siblings_ok} "
          f"mem_errors={r.mem_errors} -> {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
