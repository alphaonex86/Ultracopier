#!/usr/bin/env python3
"""Build + run the standalone uc_foldCompletedWrite() unit test (test/unit/watermark_test.cpp).

This directly exercises the contiguous-from-0 low-water mark (pipeline/ContiguousWatermark.h)
that makes media-reconnect RESUME safe on the io_uring/IOCP backends: because those backends
complete writes OUT OF ORDER, the only offset safe to resume from is the largest N with every
byte in [0,N) written -- NOT transferProgression (which can sit above a hole). The test drives
out-of-order completion, short-write partial+remainder, multi-extent absorption, zero-length,
and a scrambled 16-chunk permutation, asserting the mark equals the true contiguous prefix.

Backend-independent (the algorithm is shared base code), so it ignores the requested backends.
It is NOT a GUI run -> it bypasses harness.run(): True iff the test builds and exits 0."""
import sys, pathlib, subprocess, tempfile, shutil
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H   # noqa: F401  (kept for signature symmetry with other cases)

_PRO = pathlib.Path(__file__).resolve().parents[1] / "unit" / "watermark_test.pro"


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def run(backends=None, memcheck=H.NONE) -> bool:
    bdir = pathlib.Path(tempfile.mkdtemp(prefix="uc-watermark-"))
    try:
        q = subprocess.run([_qmake(), "-o", str(bdir / "Makefile"), str(_PRO),
                            "CONFIG+=release", "-spec", "linux-g++"],
                           cwd=bdir, capture_output=True, text=True)
        if q.returncode != 0:
            print("    [watermark] qmake FAILED\n" + q.stderr[-1500:]); return False
        m = subprocess.run(["make", "-j2"], cwd=bdir, capture_output=True, text=True)
        if m.returncode != 0:
            print("    [watermark] build FAILED\n" + (m.stderr or m.stdout)[-1500:]); return False
        binp = bdir / "watermark_test"
        r = subprocess.run([str(binp)], capture_output=True, text=True, timeout=60)
        ok = r.returncode == 0
        print(f"    [watermark] {r.stdout.strip()}")
        if not ok:
            print(r.stdout[-1500:])
        return ok
    finally:
        shutil.rmtree(bdir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
