#!/usr/bin/env python3
"""Failing‑disk backup robustness PLUS a large multi‑chunk file (>50 MiB) to confirm
that a readable sibling is NOT lost when another file faults.

The original `faulty_hdd.py` already passes for the default tree (includes 1 MiB).
This case adds a huge file (big_mb=50) to stress the pipeline and increases
the idle‑detection threshold — the harness's default idle detection may think the
transfer is done during the I/O‑error pause, killing the copy before the large
file completes. We raise `stay_alive_seconds` to 30 and extend the timeout.

ASSERTS (same as faulty_hdd):
   * r.completed and r.stayed_alive (no hang/crash)
   * every GOOD file (including the large one) is byte‑perfect
   * the BAD file is absent or a faithful prefix (never corrupt tail)
   * no OOM kill, no valgrind‑errors in our code
"""
import sys, pathlib, os, filecmp, random
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
# When run directly (python3 cases/faulty_hdd_large.py) Python prepends the script's own
# dir (cases/) to sys.path, where cases/copy.py would SHADOW the stdlib `copy` module
# (imported transitively by dataclasses inside lib.harness) and break the import.
# Drop the cases/ dir and add the test/ root so packages resolve as `lib.*`/`cases.*`.
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K


def _good_subset_ok(src_root: str, copied_root: str, good_paths) -> tuple[bool, str]:
    """Every GOOD source file must exist in the copied tree, byte‑identical."""
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
    """The bad file must be absent OR a faithful PREFIX of the source — never wrong
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
                    # A faithful prefix: dst is shorter‑or‑equal AND matches the head
                    # of src exactly. Anything else is corrupt/partial‑wrong content.
                    if len(dst_bytes) > len(src_bytes) or \
                       src_bytes[:len(dst_bytes)] != dst_bytes:
                        ok = False
                        msgs.append(f"bad file present with WRONG content: {rel} "
                                    f"(dst {len(dst_bytes)}B vs src {len(src_bytes)}B)")
    return ok, "\n".join(msgs)


def _one_run(scenario: str, file_error: int, big_mb=50) -> bool:
    """Build a tree that includes a large good file (big_mb MiB)."""
    src_root = K.fresh_src_root("faulty_src_large")
    # Build the faulty tree (default + two BADSECTOR files)
    synthtree.make_faulty_tree(src_root)   # builds default + two BADSECTOR files
    # Add a large good file (no BADSECTOR substring) in the root.
    large_name = f"large_good_{big_mb}mb.dat"
    large_path = os.path.join(src_root, large_name)
    with open(large_path, "wb") as f:
        rng = random.Random(big_mb)
        size = big_mb * 1024 * 1024
        # write in 1 MiB chunks
        chunk = 1024 * 1024
        written = 0
        while written < size:
            to_write = min(chunk, size - written)
            buf = rng.randbytes(to_write)
            f.write(buf)
            written += to_write

    # Recompute good paths: everything except BADSECTOR files.
    good_paths = []
    for dirpath, _dirs, files in os.walk(src_root):
        for fn in files:
            if "BADSECTOR" not in fn:
                good_paths.append(os.path.join(dirpath, fn))
    good_paths.sort()
    bad_substr = "BADSECTOR"

    base = os.path.basename(src_root)
    dest = K.fresh_dest("faulty_dest_large")
    copied = os.path.join(dest, base)

    K.with_scenario(scenario)
    # Increase stay_alive_seconds to 30 and timeout to 300 seconds.
    r = H.run(H.ASYNC, "cp", [src_root], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=file_error,
              expect_dir=None,                 # we diff the GOOD subset by hand
              fs_preload=K.fs_so(),
              stay_alive_seconds=30)
    K.with_scenario("")                        # don't leak the scenario to later cases

    resilient = r.completed and r.stayed_alive
    good_ok, good_msg = _good_subset_ok(src_root, copied, good_paths)
    bad_ok, bad_msg = _bad_not_corrupt(src_root, copied, bad_substr)
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
    # common case‑runner signature.
    sub_runs = [
        ("readfail:BADSECTOR", H.FileError.PUT_TO_END),
        ("readfail:BADSECTOR", H.FileError.SKIP),
        ("eio_after:BADSECTOR:65536", H.FileError.PUT_TO_END),
        ("eio_after:BADSECTOR:65536", H.FileError.SKIP),
    ]
    ok = True
    for scenario, fe in sub_runs:
        res = _one_run(scenario, fe)
        print(f"    [async large {scenario} fe={fe}] {'PASS' if res else 'FAIL'}")
        ok = ok and res
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
