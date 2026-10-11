#!/usr/bin/env python3
"""io_uring: a Retry that RESTARTS FROM 0 must not treat our own partial destination as the user's
pre-existing file.

After a destination WRITE error the retry path restarts from offset 0 (resume is only safe after a
pure source read error) and re-runs preOperation(). preOperation() then re-captured
destinationPreExisted (our non-empty partial -> "the user's file", so the #9/#25 cleanups and the
partial trim were disabled for the rest of the file) AND re-ran the collision policy against it:
with fileCollision=Skip the file was SKIPPED -- the partial stayed on disk and the entry was counted
done. "Overwrite if newer" skipped it too (our partial is newer than the source). A USB target that
hiccups once + the user clicking Retry = a silently truncated file reported as copied.

Rig: dest on a FUSE mount, UC_FUSE_WFAIL fails the first write at/after 64 KiB of the *RETRYME*
file once (unplugged, then replugged); fileError=Ask + the FileErrorDialogHook answering Retry;
fileCollision=Skip (the policy a careful user picks so nothing is overwritten); 1 inode thread.
Asserts: completed, the victim is byte-identical (retried from 0 onto our own partial, never
skipped), the sibling is intact, no crash."""
import sys, os, pathlib, filecmp
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K

VICTIM = "big_RETRYME.dat"


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IO_URING not in backends:
        print("    [skip] iouring_retry_restart_collision is io_uring-only")
        return True
    with K.FuseMount("retryrestart", UC_FUSE_WFAIL="RETRYME:65536:1") as m:
        if m is None:
            print("    [skip] libfuse3/fusermount3 unavailable or FUSE mount failed")
            return True
        src = pathlib.Path(K.fresh_dest("retryrestart_src"))
        (src / VICTIM).write_bytes(synthtree._seeded_bytes_fast(81, 1024 * 1024 + 333))
        (src / "other.dat").write_bytes(synthtree._seeded_bytes_fast(82, 90 * 1024))
        (m.back / "dest").mkdir()
        os.environ["ULTRACOPIER_TEST_FILE_ERROR_ACTION"] = "retry"
        try:
            r = H.run(H.IO_URING, "cp", [str(src)], str(m.mnt / "dest"), file_collision=H.FileCollision.SKIP,
                      folder_collision=H.FolderCollision.MERGE, file_error=H.FileError.ASK, expect_dir=None,
                      inode_threads=1)
        finally:
            os.environ.pop("ULTRACOPIER_TEST_FILE_ERROR_ACTION", None)
        copied = m.back / "dest" / src.name
        vd = copied / VICTIM
        vsize = vd.stat().st_size if vd.exists() else -1
        victim_ok = vd.exists() and filecmp.cmp(str(src / VICTIM), str(vd), shallow=False)
        other_ok = (copied / "other.dat").exists() and filecmp.cmp(str(src / "other.dat"), str(copied / "other.dat"), shallow=False)
    ok = r.completed and r.stayed_alive and victim_ok and other_ok and r.mem_errors == 0 and not r.oom_killed
    print(f"    [io_uring retry restart vs own partial] completed={r.completed} alive={r.stayed_alive} "
          f"victim_identical={victim_ok} victim_size={vsize}/{(src / VICTIM).stat().st_size} other_ok={other_ok} "
          f"mem_errors={r.mem_errors} -> {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
