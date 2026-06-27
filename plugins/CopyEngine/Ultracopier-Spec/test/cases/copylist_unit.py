#!/usr/bin/env python3
"""Build + run the in-process UI<->copy-list RECONCILIATION + reorder unit test
(test/unit/listreorder_test.cpp). This is the FAST style (#18): it drives ListThread's
moveItemsOnTop/Up/Down/OnBottom/removeItems on a synthetic transfer list directly -- NO real
copy, NO disk, NO Qt event loop -- and includes a 2,000,000-entry SCALE pass that reorders +
removes in milliseconds. The whole test runs in ~0.2 s (vs ~40-100 s for a black-box copy case),
proving C++ processes a multi-million-entry list/reorder pipeline very quickly.

Backend-independent (the reorder logic is in the shared ListThread; the .pro links the async
TransferThreadImpl only to satisfy the monolithic link graph). So this case ignores the requested
backends and is NOT a GUI run -- it bypasses harness.run() and returns True iff the unit binary
builds and exits 0 (every internal check + the scale-time budget passed)."""
import sys, os, pathlib, subprocess, tempfile, shutil
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H   # noqa: F401  (symmetry with the other cases)

_UNIT_DIR = pathlib.Path(__file__).resolve().parents[1] / "unit"
_PRO = _UNIT_DIR / "listreorder_test.pro"


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def run(backends=None, memcheck=H.NONE) -> bool:
    bdir = pathlib.Path(tempfile.mkdtemp(prefix="uc-copylist-unit-"))
    try:
        q = subprocess.run([_qmake(), "-o", str(bdir / "Makefile"), str(_PRO),
                            "CONFIG+=release", "-spec", "linux-g++"],
                           capture_output=True, text=True)
        if q.returncode != 0:
            print(f"    qmake failed:\n{q.stderr[-2000:]}")
            return False
        m = subprocess.run(["make", "-C", str(bdir), f"-j{os.cpu_count()}"],
                           capture_output=True, text=True)
        binp = bdir / "listreorder_test"
        if m.returncode != 0 or not binp.exists():
            errs = "\n".join(l for l in m.stdout.splitlines() + m.stderr.splitlines()
                             if "error:" in l or "undefined reference" in l)
            print(f"    build failed:\n{errs[-2500:]}")
            return False
        r = subprocess.run([str(binp)], capture_output=True, text=True, timeout=120)
        out = (r.stdout + r.stderr).strip()
        for line in out.splitlines():
            print(f"    {line}")
        ok = (r.returncode == 0)
        print(f"    [copylist_unit] {'PASS' if ok else 'FAIL'}")
        return ok
    finally:
        shutil.rmtree(bdir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
