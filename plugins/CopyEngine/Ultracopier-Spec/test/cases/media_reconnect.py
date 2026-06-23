#!/usr/bin/env python3
"""Media disconnect / reconnect mid-transfer: a USB / network volume drops out partway
through a file and comes back. The engine must NOT lose data or corrupt the destination --
it recovers and the copied file is byte-perfect.

We inject the drop with the LD_PRELOAD shim's recoverable MID-FILE verbs (UC_FS_SCENARIO):
  * disconnect:<substr>:<off>:<n>   -> source reads succeed to <off> bytes, then fail EIO
                                       for <n> attempts (the unplug), then succeed (replug)
  * wdisconnect:<substr>:<off>:<n>  -> destination writes succeed to <off>, then fail EIO
                                       for <n> attempts, then succeed

with fileError = PUT_TO_END so the faulted file is deferred and retried until the volume is
back. The async backend currently RESTARTS the file from 0 on each retry (resume-at-offset is
a pipelined-backend feature; async is the stable v3.0 reference and deliberately restarts) --
either way the REQUIRED guarantee is identical and is what we assert: the final copied file is
byte-for-byte the source.

ASSERTS (per sub-run):
  * r.completed and r.stayed_alive       -- no hang, no crash across the drop/recover
  * the faulted file is EVENTUALLY byte-perfect in the destination (data fully recovered)
  * every always-good sibling is byte-perfect too
  * not r.oom_killed and r.mem_errors == 0 -- the error/retry storm leaks/double-frees nothing

ASYNC ONLY: disconnect/wdisconnect fault libc read()/write(), which only the async backend's
Read/WriteThread use. io_uring submits its data I/O through io_uring_enter(2) and IOCP through
overlapped ReadFile/WriteFile -- neither goes through libc, so the shim cannot fault their data
plane. The pipelined backends' media-reconnect RESUME is verified separately (real fault device
/ the Windows box), not here. (Same backend-scope limitation as faulty_hdd.py / transient_sector.py.)
"""
import sys, pathlib, os, filecmp
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K

SRC_DROP_NAME = "movie_SRCDROP.dat"     # source volume drops mid-read
DST_DROP_NAME = "backup_DSTDROP.dat"    # dest volume drops mid-write
BIG = 4 * 1024 * 1024                   # 4 MiB: big enough to fault partway in
OFF = 1024 * 1024                       # drop at 1 MiB
RETRIES = 3                             # comes back after 3 failed attempts


def _build_tree(root: str, victim_name: str) -> str:
    """A 'default' tree plus one >1 MiB victim file that will see the media drop."""
    root = os.path.abspath(root)
    synthtree.make_tree(root, "default")
    (pathlib.Path(root) / victim_name).write_bytes(synthtree._seeded_bytes_fast(31, BIG))
    return root


def _assert_recovered(r, src, copied, victim_name) -> bool:
    resilient = r.completed and r.stayed_alive
    vs = os.path.join(src, victim_name)
    vd = os.path.join(copied, victim_name)
    victim_ok = os.path.exists(vd) and filecmp.cmp(vs, vd, shallow=False)   # fully recovered
    good_ok = True
    for dirpath, _dirs, files in os.walk(src):
        for fn in files:
            if fn != victim_name:
                sp = os.path.join(dirpath, fn)
                dp = os.path.join(copied, os.path.relpath(sp, src))
                if not (os.path.exists(dp) and filecmp.cmp(sp, dp, shallow=False)):
                    good_ok = False
    ok = (resilient and victim_ok and good_ok and not r.oom_killed and r.mem_errors == 0)
    if not ok:
        print(f"      not ok: completed={r.completed} alive={r.stayed_alive} "
              f"victim_recovered={victim_ok} good_ok={good_ok} "
              f"oom={r.oom_killed} mem_errors={r.mem_errors}")
    return ok


def _run(scenario: str, victim_name: str, tag: str) -> bool:
    src = _build_tree(K.fresh_src_root(f"reconnect_{tag}_src"), victim_name)
    base = os.path.basename(src)
    dest = K.fresh_dest(f"reconnect_{tag}_dest")
    copied = os.path.join(dest, base)
    K.with_scenario(scenario)
    r = H.run(H.ASYNC, "cp", [src], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.PUT_TO_END,    # defer + retry until the volume returns
              expect_dir=None,
              fs_preload=K.fs_so())
    K.with_scenario("")
    return _assert_recovered(r, src, copied, victim_name)


def run(backends=None, memcheck=H.NONE) -> bool:
    backends = backends or K.DEFAULT_BACKENDS
    if H.ASYNC not in backends:
        print("    [skip] media_reconnect faults libc I/O -> async backend only")
        return True
    ok = True
    res = _run(f"disconnect:SRCDROP:{OFF}:{RETRIES}", SRC_DROP_NAME, "src")
    print(f"    [async source disconnect/reconnect] {'PASS' if res else 'FAIL'}")
    ok = ok and res
    res = _run(f"wdisconnect:DSTDROP:{OFF}:{RETRIES}", DST_DROP_NAME, "dst")
    print(f"    [async dest disconnect/reconnect] {'PASS' if res else 'FAIL'}")
    ok = ok and res
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
