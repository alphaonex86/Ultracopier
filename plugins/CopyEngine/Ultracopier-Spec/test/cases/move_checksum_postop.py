#!/usr/bin/env python3
"""MOVE + keepDate (+ checksum) where the destination DATE-SET fails -> the source must be PRESERVED
(coverage-audit finding "move_checksum_postop").

A cross-filesystem MOVE is copy + delete. The engine copies the content, then in
checkIfAllIsClosedAndDoOperations() it used to unlink the SOURCE *before* doFilePostOperation() set the
destination's date/permissions. With keepDate=true a failed date-set is escalated to an error
(doFilePostOperation returns false), but by then the source was already gone -> the move "failed" yet
the user's only copy was deleted and the destination has the wrong date. The fix runs the post-op
FIRST and keeps the source if a critical post-op step fails (treats it like copyFailed).

We inject the date-set failure with the LD_PRELOAD shim verb `datefail:<substr>` (utimensat -> EPERM;
the data write already succeeded). Two runs:
  * datefail ON  -> REQUIRED: source PRESERVED (byte-identical), dest content correct, job completes.
  * datefail OFF -> REQUIRED: clean move, source REMOVED (regression guard so the fix didn't break the
    normal move).

The DATEFAIL sub-run is **ASYNC-only** by necessity: the io_uring backend opens the dest fd
via io_uring_prep_openat (through the ring -- TransferThreadUring::openDestFile), so the
LD_PRELOAD shim never records that fd's path (uc_fd_path -> NULL) and its futimens datefail
guard cannot fire -- the on-handle date-set genuinely SUCCEEDS, the source is correctly
removed, and asserting preservation there would be a FALSE alarm (the shim fundamentally
cannot fault a ring-opened fd -- the same limitation as the io_uring data plane; a REAL
io_uring/IOCP on-handle date-fault needs a FUSE utimens-EPERM mount / fs_hook verb, tracked
separately). io_uring therefore runs only the CLEAN sub-run here (proving the move +
on-handle date-set + source-removal path works end to end). IOCP: laptop lane, separate.
"""
import os
import sys
import shutil
import tempfile
import pathlib

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

PAYLOAD = (b"move-checksum-postop-payload-bytes\n" * 64)   # real multi-line content for the checksum


def _one(backend: str, datefail: bool) -> bool:
    # Source on /tmp -> different filesystem than the scratch dest -> realMove=false (copy+delete).
    src_dir = pathlib.Path(tempfile.mkdtemp(prefix="uc-mvchk-src-", dir="/tmp"))
    src_file = src_dir / "ledger.bin"
    src_file.write_bytes(PAYLOAD)
    dest = K.fresh_dest("mvchk_dest")
    dest_file = pathlib.Path(dest, "ledger.bin")

    if datefail:
        K.with_scenario("datefail:ledger.bin")   # the DEST date-set (utimensat/futimens) fails EPERM
    else:
        K.with_scenario("")
    r = H.run(backend, "mv", [str(src_file)], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,
              keep_date=True,
              expect_dir=None,
              extra_options={"checksum": "true"},
              fs_preload=K.fs_so())
    K.with_scenario("")

    dest_ok = dest_file.exists() and dest_file.read_bytes() == PAYLOAD
    src_exists = src_file.exists()
    if datefail:
        # REQUIRED: a critical (keepDate) post-op failure keeps the source.
        ok = (r.completed and r.stayed_alive and dest_ok and src_exists
              and src_file.read_bytes() == PAYLOAD and not r.oom_killed and r.mem_errors == 0)
        print(f"      [{backend} datefail] source_preserved={src_exists} dest_ok={dest_ok} "
              f"completed={r.completed} mem_err={r.mem_errors}")
        if not src_exists:
            print("        DATA LOSS: source deleted despite a keepDate date-set failure on the MOVE")
    else:
        # REQUIRED: a clean move removes the source (the fix must NOT keep the source on success).
        ok = (r.completed and r.stayed_alive and dest_ok and not src_exists
              and not r.oom_killed and r.mem_errors == 0)
        print(f"      [{backend} clean]    source_removed={not src_exists} dest_ok={dest_ok} "
              f"completed={r.completed} mem_err={r.mem_errors}")
        if src_exists:
            print("        REGRESSION: clean move did NOT remove the source")
    shutil.rmtree(src_dir, ignore_errors=True)
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    def one(backend):
        # datefail is meaningful only where the shim can fault the date-set: on async the
        # dest is opened via libc (uc_fd_path knows it) so futimens/utimensat datefail fires;
        # on io_uring the dest fd is ring-opened (invisible to the shim) so the fault can't
        # engage -- run only the clean sub-run there (see the module docstring).
        clean = _one(backend, datefail=False)
        if backend == H.ASYNC:
            return _one(backend, datefail=True) and clean
        return clean
    ok = K.for_backends(backends, one)
    print(f"    [move_checksum_postop] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
