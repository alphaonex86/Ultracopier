#!/usr/bin/env python3
"""Async read/write fault resilience, injected with the LD_PRELOAD shim (UC_FS_SCENARIO):

  * shortread:<f>:<n>  -- read() on the matched file returns at most <n> bytes, forcing the
                          engine's short-read REFILL path (a chunk is assembled from several
                          partial reads). The copy must still round-trip BYTE-IDENTICAL.
  * enospc:<f>:<bytes> -- write() to the matched DEST file succeeds until <bytes> cumulative,
                          then fails ENOSPC (the destination fills up partway through one file).
                          With fileError=Skip the engine must: stay alive, finish the whole job,
                          copy every OTHER file correctly, and NOT pass the truncated victim off
                          as a complete copy (resilient partial backup, never lose good data).

ASYNC ONLY, by design: the io_uring data plane issues its reads/writes through io_uring_enter(2),
NOT libc, so an LD_PRELOAD shim cannot fault it -- exactly the reason cases/faulty_hdd.py is also
async-only. Faulting the io_uring / IOCP write path (real ENOSPC, short/torn writes) needs a
genuine constrained filesystem (a small loopback fs or an unprivileged FUSE mount) and is tracked
as separate work; it is NOT coverable by this libc-level shim. This case therefore exercises the
shared/async read-assembly and write-error handling, which the io_uring/IOCP backends mirror.
"""
import os
import sys
import shutil
import pathlib
import subprocess

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

BIG = "big_multi.dat"
BIG_SIZE = 3 * 1024 * 1024            # multi-chunk: bigger than the engine block size
SMALLS = {"small_a.dat": 1000, "small_c.dat": 2000, "small_d.dat": 4096}


def _mktree(root: str) -> str:
    r = pathlib.Path(root)
    if r.exists():
        shutil.rmtree(r)
    r.mkdir(parents=True)
    # Deterministic, verifiable content (a fixed byte pattern, NOT random, so a re-run matches).
    (r / BIG).write_bytes(bytes((i * 7) & 0xFF for i in range(BIG_SIZE)))
    for name, n in SMALLS.items():
        (r / name).write_bytes((name.encode() * ((n // len(name)) + 1))[:n])
    return str(r)


def _size(p: str) -> int:
    try:
        return os.path.getsize(p)
    except OSError:
        return -1


def _shortread_ok() -> bool:
    """A copy where every read on the big file is clamped to 64 KiB must still be byte-identical."""
    src = _mktree(K.fresh_src_root("wf_sr_src"))
    dest = K.fresh_dest("wf_sr_dest")
    copied = os.path.join(dest, os.path.basename(src))
    K.with_scenario(f"shortread:{BIG}:65536")
    r = H.run(H.ASYNC, "cp", [src], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,
              expect_dir=None,
              fs_preload=K.fs_so())
    K.with_scenario("")
    diff = subprocess.run(["diff", "-rq", "--no-dereference", src, copied],
                          capture_output=True, text=True)
    content_ok = diff.returncode == 0
    ok = r.completed and r.stayed_alive and content_ok and not r.oom_killed and r.mem_errors == 0
    if not ok:
        print(f"      [shortread] FAIL completed={r.completed} alive={r.stayed_alive} "
              f"content_ok={content_ok} oom={r.oom_killed} mem_err={r.mem_errors}")
        if not content_ok:
            print(f"        DIFF: {diff.stdout[:300]}")
    else:
        print("      [shortread] PASS  big file reassembled byte-identical from 64KiB short reads")
    return ok


def _enospc_ok() -> bool:
    """Dest fills up (ENOSPC) 1 MiB into the big file; Skip. The job must finish, every OTHER
    file must be correct, and the victim must NOT be a full/correct copy (truncated, not claimed
    complete) -- the resilient partial-backup contract."""
    src = _mktree(K.fresh_src_root("wf_es_src"))
    dest = K.fresh_dest("wf_es_dest")
    copied = os.path.join(dest, os.path.basename(src))
    K.with_scenario(f"enospc:{BIG}:1048576")
    r = H.run(H.ASYNC, "cp", [src], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,
              expect_dir=None,
              fs_preload=K.fs_so())
    K.with_scenario("")
    # Every NON-victim file must be byte-correct.
    good_ok = True
    for name, n in SMALLS.items():
        sp, dp = os.path.join(src, name), os.path.join(copied, name)
        d = subprocess.run(["cmp", "-s", sp, dp], capture_output=True)
        if d.returncode != 0:
            good_ok = False
            print(f"      [enospc] good file corrupt/missing: {name} (size={_size(dp)} want {n})")
    # The victim must NOT be passed off as a complete copy: absent, or smaller than the source.
    victim = _size(os.path.join(copied, BIG))
    victim_not_complete = (victim < BIG_SIZE)   # -1 (absent) or truncated both satisfy this
    ok = (r.completed and r.stayed_alive and good_ok and victim_not_complete
          and not r.oom_killed and r.mem_errors == 0)
    if not ok:
        print(f"      [enospc] FAIL completed={r.completed} alive={r.stayed_alive} good_ok={good_ok} "
              f"victim_size={victim}(src={BIG_SIZE}) victim_not_complete={victim_not_complete} "
              f"oom={r.oom_killed} mem_err={r.mem_errors}")
    else:
        print(f"      [enospc] PASS  job finished, good files correct, victim truncated "
              f"({victim}/{BIG_SIZE}) not claimed complete, no leaks")
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    # ASYNC only (see docstring); accept the common signature. The shim cannot fault io_uring's
    # io_uring_enter(2) data plane, so there is nothing for the other backends to assert here.
    print("[write_faults] async read/write fault resilience")
    results = [("shortread", _shortread_ok()), ("enospc", _enospc_ok())]
    ok = all(v for _, v in results)
    print(f"    [write_faults] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
