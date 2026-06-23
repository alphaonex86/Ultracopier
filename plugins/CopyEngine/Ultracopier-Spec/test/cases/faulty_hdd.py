#!/usr/bin/env python3
"""Failing-disk backup: the source is a DYING HDD. Some files sit on dead sectors and
fault on read(); the rest are perfectly readable. The expected, REQUIRED behaviour is
"try, work around, inform": copy every readable file in full, SKIP/defer the unreadable
ones (per the fileError policy), and NEVER abort the whole backup, hang, or crash.

We inject the source-read faults with the LD_PRELOAD shim (UC_FS_SCENARIO):
  * readfail:BADSECTOR        -> the whole BADSECTOR file is unreadable (dead from byte 0)
  * eio_after:BADSECTOR:65536 -> the BADSECTOR file reads its first 64 KiB then dies

and run two fileError policies that must both yield a resilient partial backup:
  * PUT_TO_END -> defer the bad file, keep going, eventually give up on it
  * SKIP       -> skip the bad file immediately

ASSERTS (per sub-run):
  * r.completed and r.stayed_alive      -- the backup did NOT hang or crash on the dead sectors
  * every GOOD file is byte-perfect      -- diff of the readable subset only == 0
  * the BAD file is NOT present with corrupt/partial-WRONG content -- it is either
    absent (skipped/deferred-then-given-up) or, at worst, a FAITHFUL PREFIX of the
    source (never trailing garbage, never the wrong bytes).

ASYNC ONLY: the async ReadThread reads the source via libc ::read/::pread, so the shim
faults those source reads. The io_uring data-plane reads go through io_uring_enter(2),
NOT libc, so they cannot be libc-faulted -- there is nothing to assert for io_uring here."""
import sys, pathlib, os, filecmp
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
# When run directly (python3 cases/faulty_hdd.py) Python prepends the script's own
# dir (cases/) to sys.path, where cases/copy.py would SHADOW the stdlib `copy` module
# (imported transitively by dataclasses inside lib.harness) and break the import.
# Drop the cases/ dir and add the test/ root so packages resolve as `lib.*`/`cases.*`.
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K


def _good_subset_ok(src_root: str, copied_root: str, good_paths) -> tuple[bool, str]:
    """Every GOOD source file must exist in the copied tree, byte-identical."""
    msgs = []
    ok = True
    for sp in good_paths:
        rel = os.path.relpath(sp, src_root)
        dp = os.path.join(copied_root, rel)
        if not os.path.exists(dp):
            ok = False
            msgs.append(f"missing good file: {rel}")
        elif not filecmp.cmp(sp, dp, shallow=False):
            ok = False
            msgs.append(f"good file differs: {rel}")
    return ok, "\n".join(msgs)


def _bad_not_corrupt(src_root: str, copied_root: str, bad_substr: str) -> tuple[bool, str]:
    """The bad file must be absent OR a faithful PREFIX of the source -- never wrong
    bytes, never trailing garbage, never the full size with corrupt tail."""
    msgs = []
    ok = True
    for dirpath, _dirs, files in os.walk(src_root):
        for fn in files:
            if bad_substr in fn:
                sp = os.path.join(dirpath, fn)
                rel = os.path.relpath(sp, src_root)
                dp = os.path.join(copied_root, rel)
                if os.path.exists(dp):
                    src_bytes = pathlib.Path(sp).read_bytes()
                    dst_bytes = pathlib.Path(dp).read_bytes()
                    # A faithful prefix: dst is shorter-or-equal AND matches the head
                    # of src exactly. Anything else is corrupt/partial-wrong content.
                    if len(dst_bytes) > len(src_bytes) or \
                       src_bytes[:len(dst_bytes)] != dst_bytes:
                        ok = False
                        msgs.append(f"bad file present with WRONG content: {rel} "
                                    f"(dst {len(dst_bytes)}B vs src {len(src_bytes)}B)")
    return ok, "\n".join(msgs)


def _one_run(scenario: str, file_error: int) -> bool:
    src, good_paths, bad_substr = synthtree.make_faulty_tree(K.fresh_src_root("faulty_src"))
    base = os.path.basename(src)
    dest = K.fresh_dest("faulty_dest")
    copied = os.path.join(dest, base)

    K.with_scenario(scenario)
    r = H.run(H.ASYNC, "cp", [src], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=file_error,
              expect_dir=None,                 # we diff the GOOD subset by hand
              fs_preload=K.fs_so())
    K.with_scenario("")                        # don't leak the scenario to later cases

    resilient = r.completed and r.stayed_alive
    good_ok, good_msg = _good_subset_ok(src, copied, good_paths)
    bad_ok, bad_msg = _bad_not_corrupt(src, copied, bad_substr)
    ok = resilient and good_ok and bad_ok and not r.oom_killed and r.mem_errors == 0
    if not ok:
        print(f"      not ok [{scenario} fe={file_error}]: completed={r.completed} "
              f"alive={r.stayed_alive} good_ok={good_ok} bad_ok={bad_ok} "
              f"oom={r.oom_killed} mem_errors={r.mem_errors}")
        if good_msg:
            print(f"        GOOD: {good_msg}")
        if bad_msg:
            print(f"        BAD:  {bad_msg}")
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    # ASYNC only (see module docstring); `backends`/`memcheck` accepted for the
    # common case-runner signature. memcheck is not threaded here because this case
    # already runs four sub-runs; the dedicated memory.py case covers valgrind/ASan.
    sub_runs = [
        ("readfail:BADSECTOR", H.FileError.PUT_TO_END),
        ("readfail:BADSECTOR", H.FileError.SKIP),
        ("eio_after:BADSECTOR:65536", H.FileError.PUT_TO_END),
        ("eio_after:BADSECTOR:65536", H.FileError.SKIP),
    ]
    ok = True
    for scenario, fe in sub_runs:
        res = _one_run(scenario, fe)
        print(f"    [async {scenario} fe={fe}] {'PASS' if res else 'FAIL'}")
        ok = ok and res
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
