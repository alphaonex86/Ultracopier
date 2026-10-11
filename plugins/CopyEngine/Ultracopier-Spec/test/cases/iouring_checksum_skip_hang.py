#!/usr/bin/env python3
"""io_uring: a CHECKSUM MISMATCH must leave the thread in a state the error policy can finish.

After runChecksumVerify() failed, checkIfAllIsClosedAndDoOperations() removed the corrupt
destination and parked the thread in TransferStat_PostTransfer, waiting for the policy. But the
pipelined skip()/stop() treat PostTransfer as "the worker is inside its own completion tail, it
reaches Idle by itself" and only raise needSkip -- nothing ever completes it. fileError=Skip,
Put-to-end (-> skip()) and Cancel therefore WEDGED the thread: the job never reached Idle, every
file queued behind it on that thread was never copied, the window could not close. Only Retry
worked. The async reference completes the same skip (PostTransfer -> PostOperation -> Idle).

Rig: dest on a FUSE mount whose UC_FUSE_WFAULT bit-flips one byte of the *CORRUPT* file on its way
to disk (the write still returns the full count), transferChecksum on, 1 inode thread, and a
sibling that SORTS AFTER the victim so it is queued behind it on the same thread. Rounds: Skip and
Put-to-end. Asserts: the job completes, the later sibling IS copied byte-identical, the corrupt
destination is gone (never a full-size wrong file), no crash."""
import sys, os, pathlib, filecmp
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K

VICTIM = "m_CORRUPT.dat"
AFTER = "zz_after.dat"


def _round(m, file_error: int, tag: str) -> bool:
    src = pathlib.Path(K.fresh_dest(f"cksumhang_src_{tag}"))
    (src / "a_good.dat").write_bytes(synthtree._seeded_bytes_fast(70, 150 * 1024))
    (src / VICTIM).write_bytes(synthtree._seeded_bytes_fast(71, 200 * 1024))
    (src / AFTER).write_bytes(synthtree._seeded_bytes_fast(72, 180 * 1024))
    dest_sub = f"dest_{tag}"
    (m.back / dest_sub).mkdir()
    os.environ["ULTRACOPIER_TEST_FILE_ERROR_ACTION"] = "skip"   # the put-to-end round escalates to Ask once
    try:
        r = H.run(H.IO_URING, "cp", [str(src)], str(m.mnt / dest_sub), file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE, file_error=file_error, expect_dir=None,
                  inode_threads=1, extra_options={"checksum": "true"})
    finally:
        os.environ.pop("ULTRACOPIER_TEST_FILE_ERROR_ACTION", None)
    copied = m.back / dest_sub / src.name
    after_ok = (copied / AFTER).exists() and filecmp.cmp(str(src / AFTER), str(copied / AFTER), shallow=False)
    good_ok = (copied / "a_good.dat").exists() and filecmp.cmp(str(src / "a_good.dat"), str(copied / "a_good.dat"), shallow=False)
    victim_gone = not (copied / VICTIM).exists()
    ok = r.completed and r.stayed_alive and after_ok and good_ok and victim_gone and r.mem_errors == 0 and not r.oom_killed
    print(f"      [{tag}] completed={r.completed} alive={r.stayed_alive} later_sibling_copied={after_ok} "
          f"good_ok={good_ok} corrupt_dest_removed={victim_gone} mem_errors={r.mem_errors} -> {'PASS' if ok else 'FAIL'}")
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IO_URING not in backends:
        print("    [skip] iouring_checksum_skip_hang is io_uring-only")
        return True
    with K.FuseMount("cksumhang", UC_FUSE_WFAULT="CORRUPT:4096") as m:
        if m is None:
            print("    [skip] libfuse3/fusermount3 unavailable or FUSE mount failed")
            return True
        ok = _round(m, H.FileError.SKIP, "skip")
        ok = _round(m, H.FileError.PUT_TO_END, "puttoend") and ok
    print(f"    [io_uring checksum mismatch + policy] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
