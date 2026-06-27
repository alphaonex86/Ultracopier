#!/usr/bin/env python3
"""Verify the Oxygen theme closes / keeps the copy WINDOW open at end-of-copy exactly as the
comboBox_copyEnd option documents (interface.cpp Themes::actionInProgess(Idle)):

  index 0  "Don't close if errors are found":  success -> close,  error -> stay open
  index 1  "Never close":                      success -> stay,   error -> stay open
  index 2  "Always close":                      success -> close,  error -> close

This is the FAST in-process style (#18): it builds + runs test/unit/window_close_test.cpp, which
instantiates the REAL Themes widget, drives it through "copy started -> (maybe error) -> idle",
and asserts whether the theme emits cancel() (== close the window) for each of the six
(index x error) combinations -- the whole thing in milliseconds, no real copy, no disk.

Why in-process and not black-box window observation: under the headless test mode (offscreen
QPA, CLI `cp`) the copy window is never shown as a visible top-level widget -- verified, only the
engine's option sub-dialogs ever appear -- so "did the window open/close" cannot be observed from
outside the process. The decision comboBox_copyEnd governs CAN be exercised directly here, and
end-to-end copy correctness is already covered by the black-box copy/faulty_hdd cases.

Backend-independent (the close decision lives in the Oxygen theme; the .pro links the theme as a
console app). It ignores the requested backends and returns True iff the unit binary builds and
exits 0 (all six decision checks passed)."""
import sys, os, pathlib, subprocess, tempfile, shutil
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H   # noqa: F401  (symmetry with the other cases)

_UNIT_DIR = pathlib.Path(__file__).resolve().parents[1] / "unit"
_PRO = _UNIT_DIR / "window_close_test.pro"


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def run(backends=None, memcheck=H.NONE) -> bool:
    bdir = pathlib.Path(tempfile.mkdtemp(prefix="uc-window-close-unit-"))
    try:
        q = subprocess.run([_qmake(), "-o", str(bdir / "Makefile"), str(_PRO),
                            "CONFIG+=release", "-spec", "linux-g++"],
                           capture_output=True, text=True)
        if q.returncode != 0:
            print(f"    qmake failed:\n{q.stderr[-2000:]}")
            return False
        m = subprocess.run(["make", "-C", str(bdir), f"-j{os.cpu_count()}"],
                           capture_output=True, text=True)
        binp = bdir / "window_close_test"
        if m.returncode != 0 or not binp.exists():
            errs = "\n".join(l for l in m.stdout.splitlines() + m.stderr.splitlines()
                             if "error:" in l or "undefined reference" in l)
            print(f"    build failed:\n{errs[-2500:]}")
            return False
        # offscreen QPA + blank DISPLAY: the Themes widget constructs without ever reaching a
        # real X display (no dialog can pop onto the operator's screen).
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen", DISPLAY="")
        r = subprocess.run([str(binp)], capture_output=True, text=True, timeout=120, env=env)
        out = (r.stdout + r.stderr).strip()
        for line in out.splitlines():
            if "propagateSizeHints" not in line:   # harmless offscreen QPA chatter
                print(f"    {line}")
        ok = (r.returncode == 0)
        print(f"    [window_close] {'PASS' if ok else 'FAIL'}")
        return ok
    finally:
        shutil.rmtree(bdir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
