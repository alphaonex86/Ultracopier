#!/usr/bin/env python3
"""Build + run the standalone operation-decomposition scheduler unit test (test/unit/opscheduler_test.cpp).

This exercises the pipelining scheduler CORE (pipeline/OpScheduler.h) — STAGE 1a of the tiered-parallelism
rewrite (pipeline/PIPELINING_DESIGN.md). It proves, at the plan level (no I/O, no Qt), the SAME invariants
ops_integrity.py / ops_dependency.py assert against the live engine: NO duplicate / NO forgotten op, and
every hard FS dependency edge (mkdir<open<data<close<file-meta; mkdir top-down; and the transitive FOLDER-
date-after-EVERY-contained-file-closed rule) holds under ANY order the caller completes ready ops in — plus
the tiered-pool (Inode / DataSmall / DataMedium / DataLarge) size classification.

Backend-independent (pure logic), so it ignores the requested backends. NOT a GUI run -> bypasses
harness.run(): True iff the test builds and exits 0."""
import sys, pathlib, subprocess, tempfile, shutil
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H   # noqa: F401  (kept for signature symmetry with other cases)

_PRO = pathlib.Path(__file__).resolve().parents[1] / "unit" / "opscheduler_test.pro"


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def run(backends=None, memcheck=H.NONE) -> bool:
    bdir = pathlib.Path(tempfile.mkdtemp(prefix="uc-opsched-"))
    try:
        q = subprocess.run([_qmake(), "-o", str(bdir / "Makefile"), str(_PRO),
                            "CONFIG+=release", "-spec", "linux-g++"],
                           cwd=bdir, capture_output=True, text=True)
        if q.returncode != 0:
            print("    [opscheduler] qmake FAILED\n" + q.stderr[-1500:]); return False
        m = subprocess.run(["make", "-j2"], cwd=bdir, capture_output=True, text=True)
        if m.returncode != 0:
            print("    [opscheduler] build FAILED\n" + (m.stderr or m.stdout)[-1500:]); return False
        binp = bdir / "opscheduler_test"
        r = subprocess.run([str(binp)], capture_output=True, text=True, timeout=60)
        ok = r.returncode == 0
        print(f"    [opscheduler] {r.stdout.strip()}")
        if not ok:
            print(r.stdout[-1500:])
        return ok
    finally:
        shutil.rmtree(bdir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
