#!/usr/bin/env python3
"""Critical data-integrity test for MOVE: when a MOVE (cross-drive) hits a destination write
error and the user chooses SKIP, the source file MUST be preserved. Before the fix, the
engine deleted the source after the copy attempt (checking only `if (exists(destination))`,
which is true even for a partial file created by a failed write) -- silently losing the
user's data.

Scenario (simulates NAS disconnect / unwritable destination):
  1. Source: a normal file in a scratch dir.
  2. Destination: a READ-ONLY file at the destination path. The engine opens it for write,
     fails with EACCES / EBUSY / read-only, and reports a write error.
  3. fileError policy = SKIP. The engine must skip the file (not remove the destination, not
     remove the source).
  4. After the copy ends, the source MUST still exist with its original content.

ASSERTS (per backend):
  * r.completed and r.stayed_alive (no crash, no hang)
  * the source file exists AND has the original content (byte-identical)
  * (the destination may or may not exist -- we don't assert on it, because the engine may
    have left a partial file there; we only assert on the source preservation)
"""
import os, sys, pathlib, shutil, tempfile

_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import casekit as K


def _one_run(backend: str) -> bool:
    """Run the move-with-destination-error scenario; return True iff the source survives.

    To exercise the COPY+DELETE (cross-drive) move path (realMove=false), the source must be
    on a different filesystem than the destination. On Linux, /tmp is tmpfs while the
    scratch root is on the root filesystem (ext4/btrfs/...), so isSameDrive() returns
    false -> the engine does a real copy then deletes the source. That is the path that
    had the data-loss bug: a destination write error followed by SKIP used to delete the
    source anyway, because the post-op check was only `if (exists(destination))`.
    """
    # Source on a DIFFERENT filesystem than the scratch dest, to force realMove=false.
    src_dir = tempfile.mkdtemp(prefix="uc-move-err-src-", dir="/tmp")
    src_file = os.path.join(src_dir, "data.bin")
    payload = os.urandom(1024 * 1024)  # 1 MB of random data
    with open(src_file, "wb") as f:
        f.write(payload)

    dest_dir = K.fresh_dest("move_err_dest")
    # Make the destination path a READ-ONLY file: open it for write will fail with EACCES.
    dest_file = os.path.join(dest_dir, "data.bin")
    with open(dest_file, "wb") as f:
        f.write(b"original")  # pre-existing content
    os.chmod(dest_file, 0o444)  # read-only: write will fail

    try:
        r = H.run(backend, "mv", [src_file], dest_dir,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.SKIP,
                  expect_dir=None)  # we do the assertions by hand

        # The copy must complete without crashing.
        ok = r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0

        # The source MUST still exist with its original content (byte-identical).
        if ok:
            if not os.path.exists(src_file):
                ok = False
                print(f"      [{backend}] FAIL: source deleted despite skip! "
                      f"(data loss bug)")
            else:
                with open(src_file, "rb") as f:
                    if f.read() != payload:
                        ok = False
                        print(f"      [{backend}] FAIL: source content corrupted")
        else:
            print(f"      [{backend}] FAIL: completed={r.completed} alive={r.stayed_alive} "
                  f"oom={r.oom_killed} mem_err={r.mem_errors}")
        return ok
    finally:
        # Restore permissions so cleanup can remove the dest.
        try: os.chmod(dest_file, 0o644)
        except OSError: pass
        shutil.rmtree(src_dir, ignore_errors=True)
        shutil.rmtree(dest_dir, ignore_errors=True)


def run(backends=None, memcheck=H.NONE) -> bool:
    bks = backends or [H.ASYNC, H.IO_URING]
    avail = []
    for b in bks:
        try:
            H.binary_for(b, H.load_config(), asan=False)
            avail.append(b)
        except Exception:
            pass

    results = []
    for backend in avail:
        res = _one_run(backend)
        results.append((backend, res))
        print(f"    [{backend} move-dest-error] {'PASS' if res else 'FAIL'}")

    ok = all(r[1] for r in results)
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)