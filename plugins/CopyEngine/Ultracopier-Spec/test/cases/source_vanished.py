#!/usr/bin/env python3
"""Source file REMOVED after it was listed -> the engine must report "source not found"
for that one file, SKIP it, and copy every other file correctly (resilient backup, no
hang/crash). This is the classic scan-then-transfer race: the ListThread scans+lists the
file (its stat/lstat succeed, so it is counted into the transfer list with a size), but
by the time the TransferThread opens the source to read it, the file is gone -> open()
returns ENOENT.

Two mechanisms, by backend (the engines open the SOURCE differently):

  * async  -> DETERMINISTIC via the LD_PRELOAD shim verb `gone:VANISH`:
              stat/lstat still succeed (the scan LISTS the file) but open()/openat()
              of the matching path returns -1/ENOENT (the transfer cannot open it).
              The async ReadThread opens the source through libc open(), so the shim
              fires. No timing, no race -- the file is *always* listed-then-gone.

  * io_uring / IOCP -> the source is opened via io_uring_prep_openat (a ring SQE) /
              CreateFile, NOT libc open(), so the shim cannot fault it. There we do a
              REAL removal instead: a tiny tree (so the whole scan completes near-
              instantly) with one big first file; once the big file's transfer is
              visibly underway in the destination (-> the scan has long since listed
              every file, including the victim) we delete the victim source. When the
              transfer reaches the victim its real openat returns ENOENT. io_uring runs
              here; IOCP runs the same idea on the Windows box (see test/iocp_run.py /
              winlane). See `remove_after` in harness.run().

ASSERTS (every backend):
  * r.completed and r.stayed_alive          -- the removal did NOT hang or crash the job
  * the VICTIM file is ABSENT from the dest -- its source could not be opened (skipped)
  * every OTHER file is byte-perfect        -- one missing source must not lose siblings
  * not r.oom_killed and r.mem_errors == 0  -- the error path leaks/double-frees nothing
"""
import sys, pathlib, os, filecmp
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K

VICTIM_NAME = "invoice_VANISH_2026.dat"   # contains 'VANISH' -> matched by gone:VANISH
BIG_NAME = "aaa_big_first.dat"            # sorts first; keeps the transfer busy (real-removal lane)
BIG_SIZE = 192 * 1024 * 1024             # 192 MiB: long enough that the dest file is visibly growing


def _build_tree(root: str) -> str:
    """A small tree: a big first file, a few always-good files, and the victim.
    Deliberately small so the scan lists everything almost instantly."""
    root = os.path.abspath(root)
    os.makedirs(root, exist_ok=True)
    (pathlib.Path(root) / BIG_NAME).write_bytes(synthtree._seeded_bytes_fast(11, BIG_SIZE))
    for i in range(4):
        (pathlib.Path(root) / f"good_{i}.txt").write_bytes(
            synthtree._seeded_bytes_fast(100 + i, 4096 + i))
    (pathlib.Path(root) / VICTIM_NAME).write_bytes(synthtree._seeded_bytes_fast(42, 65536))
    return root


def _assert_outcome(r, src, copied) -> bool:
    """Resilient-skip outcome: alive+completed, victim absent, every sibling byte-perfect."""
    resilient = r.completed and r.stayed_alive
    victim_dst = os.path.join(copied, VICTIM_NAME)
    victim_absent = not os.path.exists(victim_dst)
    good_ok = True
    for dirpath, _dirs, files in os.walk(src):
        for fn in files:
            if fn != VICTIM_NAME:
                sp = os.path.join(dirpath, fn)
                dp = os.path.join(copied, os.path.relpath(sp, src))
                if not (os.path.exists(dp) and filecmp.cmp(sp, dp, shallow=False)):
                    good_ok = False
    ok = (resilient and victim_absent and good_ok
          and not r.oom_killed and r.mem_errors == 0)
    if not ok:
        print(f"      not ok: completed={r.completed} alive={r.stayed_alive} "
              f"victim_absent={victim_absent} good_ok={good_ok} "
              f"oom={r.oom_killed} mem_errors={r.mem_errors}")
    return ok


def _run_async_gone() -> bool:
    """async: deterministic listed-then-ENOENT via the `gone` shim (no removal, no race)."""
    src = _build_tree(K.fresh_src_root("vanish_async_src"))
    base = os.path.basename(src)
    dest = K.fresh_dest("vanish_async_dest")
    copied = os.path.join(dest, base)
    K.with_scenario("gone:VANISH")
    r = H.run(H.ASYNC, "cp", [src], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,        # source-not-found -> skip + keep going
              expect_dir=None,
              fs_preload=K.fs_so())
    K.with_scenario("")
    return _assert_outcome(r, src, copied)


def _run_real_removal(backend) -> bool:
    """io_uring (and async, as a real-removal cross-check): physically delete the victim
    once the big file's transfer is underway -> the scan listed it, the transfer opens a
    file that is really gone."""
    src = _build_tree(K.fresh_src_root(f"vanish_{backend}_src"))
    base = os.path.basename(src)
    dest = K.fresh_dest(f"vanish_{backend}_dest")
    copied = os.path.join(dest, base)
    victim_src = os.path.join(src, VICTIM_NAME)
    big_dst = os.path.join(copied, BIG_NAME)
    K.with_scenario("")
    r = H.run(backend, "cp", [src], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,
              expect_dir=None,
              # inodeThreads=1 -> strictly SEQUENTIAL transfer in list order. The big file
              # (sorts first) is transferred first; the victim (sorts last) cannot be
              # touched until the big file completes. With parallel threads a free thread
              # would grab the victim immediately and the race would be lost.
              inode_threads=1,
              # once the big dest file is clearly mid-transfer (>=8 MiB written) the scan
              # has long finished -> the victim is listed; delete it before its turn.
              remove_after=[{"when_dest_exists": big_dst, "min_size": 8 * 1024 * 1024,
                             "delete": [victim_src]}])
    return _assert_outcome(r, src, copied)


def run(backends=None, memcheck=H.NONE) -> bool:
    backends = backends or K.DEFAULT_BACKENDS
    ok = True
    if H.ASYNC in backends:
        res = _run_async_gone()
        print(f"    [async gone:VANISH] {'PASS' if res else 'FAIL'}")
        ok = ok and res
    if H.IO_URING in backends:
        res = _run_real_removal(H.IO_URING)
        print(f"    [io_uring real-removal] {'PASS' if res else 'FAIL'}")
        ok = ok and res
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
