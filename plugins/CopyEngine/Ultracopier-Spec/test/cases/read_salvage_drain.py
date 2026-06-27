#!/usr/bin/env python3
"""#23 read-salvage drain: on a PURE read-fault SKIP of a dying-disk file whose partials are kept, the
salvaged partial must contain ALL readable bytes (the read-ahead buffered before the fault), not just the
lagging write position. The async read thread reads AHEAD into a bounded buffer (theBlockList); on a fault
that buffer was historically DISCARDED, so the salvage stopped at the lagging write. #23 keeps it and drains
it in WriteThread::internalClose() (gated to OUR fresh dest), and keeps the partial through teardown
(TransferThreadAsync's read-fault-skip unlinks are gated !finalSkipNoRetry). See engine markers
"#23 read-salvage" and memory project_read_salvage_drain_followup.

This ALSO is the deterministic test for the new I/O-ORDER CONTROL verbs in the LD_PRELOAD shim
(fsoverride/linux/fs_preload.c): the salvage only happens on the "write opens + enters its loop before the
fault" path (Case A). A fast/cached source races the write's open (Case B) and there is nothing to salvage,
so the test must FORCE Case A deterministically. Two independent orderings are exercised (both must salvage
the full prefix), proving the I/O-order control works:
  * slow:<ms>     -- slows read+write so the fault is late (write opens+loops first) + slowwrite lags the
                     write so the read-ahead buffer is non-empty at the fault.
  * delayopen:r:<ms> -- "first write open, after <ms> the read open": delays the SOURCE (read) open so the
                     dest write opens first; slowwrite then fills the buffer.

ASYNC ONLY (the drain lives in the async WriteThread; io_uring/IOCP data plane bypasses libc)."""
import sys, os, pathlib
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import casekit as K

O = 1024 * 1024            # 1 MiB readable, then the source sector dies
TOTAL = 2 * 1024 * 1024
_PATTERN = (bytes(range(256)) * (TOTAL // 256))   # position-dependent: a misaligned salvage is caught
GOOD = b"good-sibling-payload\n" * 512
_TRACE = os.environ.get("UC_RSD_TRACE_PATH")

# Two I/O-order-control scenarios that each force Case A (write opens+loops before the fault). Both must
# salvage the full readable prefix. label -> UC_FS_SCENARIO.
SCENARIOS = [
    ("slow+slowwrite",       f"slow:4,slowwrite:12,eio_after:SALVAGEME:{O}"),
    ("delayopen-read",       f"delayopen:r:40,slowwrite:10,eio_after:SALVAGEME:{O}"),
]


def _evict(path):
    fd = os.open(path, os.O_RDONLY)
    try:
        os.fsync(fd)
        if hasattr(os, "posix_fadvise"):
            os.posix_fadvise(fd, 0, 0, os.POSIX_FADV_DONTNEED)
    finally:
        os.close(fd)


def _make_src():
    src = pathlib.Path(K.fresh_src_root("salvage_src"))
    src.mkdir(parents=True, exist_ok=True)
    sm = src / "SALVAGEME.dat"
    sm.write_bytes(_PATTERN)
    (src / "good.dat").write_bytes(GOOD)
    _evict(str(sm))
    return str(src)


def _one(label, scenario):
    if _TRACE:
        open(_TRACE, "w").close()
    src = _make_src()
    base = os.path.basename(src)
    dest = K.fresh_dest("salvage_dest")
    K.with_scenario(scenario)
    r = H.run(H.ASYNC, "cp", [src], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,
              extra_options={"deletePartiallyTransferredFiles": "false"},  # keep partials = the salvage use case
              expect_dir=None,
              fs_preload=K.fs_so())
    K.with_scenario("")

    dst_salvage = os.path.join(dest, base, "SALVAGEME.dat")
    dst_good = os.path.join(dest, base, "good.dat")
    salvaged = os.path.getsize(dst_salvage) if os.path.exists(dst_salvage) else -1
    prefix_ok = (salvaged >= 0 and salvaged <= TOTAL
                 and pathlib.Path(dst_salvage).read_bytes() == _PATTERN[:salvaged])
    drain_ok = (salvaged == O)
    good_ok = (os.path.exists(dst_good) and pathlib.Path(dst_good).read_bytes() == GOOD)
    resilient = r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0
    ok = resilient and prefix_ok and drain_ok and good_ok

    if _TRACE and not ok and os.path.exists(_TRACE):
        for l in [l for l in open(_TRACE).read().splitlines() if "SALVAGEME" in l][:50]:
            print("          " + l)
    print(f"      [{label}] salvaged={salvaged}B (want O={O}) prefix_ok={prefix_ok} good_ok={good_ok} "
          f"| completed={r.completed} alive={r.stayed_alive} mem_errors={r.mem_errors} -> {'PASS' if ok else 'FAIL'}")
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.ASYNC not in backends:
        print("    [skip] read_salvage_drain is async-only")
        return True
    ok = True
    for label, scenario in SCENARIOS:
        ok = _one(label, scenario) and ok
    print(f"    [read_salvage_drain] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
