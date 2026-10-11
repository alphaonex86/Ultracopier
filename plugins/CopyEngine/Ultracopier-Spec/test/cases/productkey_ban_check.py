#!/usr/bin/env python3
"""Build + run the ProductKey ban-list unit test (test/unit/productkey_test.cpp).

Regression for the tray menu losing "Register product key" ~15 s after start (seen under wine):
ProductKey::httpFinished matched an EMPTY lastKey against the downloaded ban list
(QString::contains("") is true), so every free user was "banned" and changeToUltimate() was emitted,
which SystrayIcon::changeToUltimate turns into removeAction(actionProductKey). The test compiles the
REAL ProductKey.cpp with only the ban URL redirected to an embedded qrc: file (no internet), fetches
it, and asserts neither changeToUltimate nor bannedKey fires without a valid key.

Core code, backend-independent: ignores the requested backends. True iff it builds and exits 0."""
import sys, os, pathlib, subprocess, tempfile, shutil
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H   # noqa: F401  (kept for signature symmetry with other cases)

_PRO = pathlib.Path(__file__).resolve().parents[1] / "unit" / "productkey_test.pro"


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def run(backends=None, memcheck=H.NONE) -> bool:
    bdir = pathlib.Path(tempfile.mkdtemp(prefix="uc-productkey-"))
    try:
        q = subprocess.run([_qmake(), "-o", str(bdir / "Makefile"), str(_PRO),
                            "CONFIG+=release", "-spec", "linux-g++"],
                           cwd=bdir, capture_output=True, text=True)
        if q.returncode != 0:
            print("    [productkey] qmake FAILED\n" + q.stderr[-1500:]); return False
        m = subprocess.run(["make", "-j2"], cwd=bdir, capture_output=True, text=True)
        if m.returncode != 0:
            print("    [productkey] build FAILED\n" + (m.stderr or m.stdout)[-1500:]); return False
        binp = bdir / "productkey_test"
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen", DISPLAY="")
        r = subprocess.run([str(binp)], capture_output=True, text=True, timeout=60, env=env)
        ok = r.returncode == 0
        print(f"    [productkey] {r.stdout.strip()}")
        if not ok:
            print((r.stdout + r.stderr)[-1500:])
        return ok
    finally:
        shutil.rmtree(bdir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
