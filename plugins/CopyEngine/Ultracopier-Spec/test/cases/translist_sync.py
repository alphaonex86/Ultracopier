#!/usr/bin/env python3
"""Verify every theme empties its transfer list when the engine says a file is done.

The engine drops a finished entry with an O(1) TOMBSTONE and only compacts
actionToDoListTransfer every 512 of them, so the `position` it ships with a RemoveItem is a
PHYSICAL slot of a sparse list: it keeps growing while the theme's dense row list shrinks. A
theme that bounds-checks that position against its own row count drops the removal, the row
stays displayed, the list never empties and the end-of-copy action (close the window) never
runs -- the "blocked at 99%" reports on 3.1.0.5, whose debug log shows exactly
"id: 3, position is wrong: 1" followed by a window that stays open with one leftover row.

Removal is therefore BY ID, never by position. This case builds + runs
test/unit/translist_sync_test.cpp against BOTH shipped models (Oxygen -- also used by the
Supercopier theme, which includes Oxygen's interfaceInclude.pri -- and Oxygen2), driving the
real synchronizeItems() with the action streams the engine really produces: one completion per
UI tick, a whole batch at once, out-of-order completions, and a batch that mixes a newly
scanned file with completions (the path Oxygen's pure-removal fast path does not cover).

Backend-independent: the models are theme code, so the requested backends are ignored."""
import sys, os, pathlib, subprocess, tempfile, shutil
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H   # noqa: F401  (symmetry with the other cases)

_UNIT_DIR = pathlib.Path(__file__).resolve().parents[1] / "unit"
_THEMES = ("oxygen", "oxygen2")


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def _run_one(theme: str) -> bool:
    pro = _UNIT_DIR / f"translist_sync_{theme}_test.pro"
    bdir = pathlib.Path(tempfile.mkdtemp(prefix=f"uc-translist-{theme}-"))
    try:
        q = subprocess.run([_qmake(), "-o", str(bdir / "Makefile"), str(pro),
                            "CONFIG+=release", "-spec", "linux-g++"],
                           capture_output=True, text=True)
        if q.returncode != 0:
            print(f"    [{theme}] qmake failed:\n{q.stderr[-2000:]}")
            return False
        m = subprocess.run(["make", "-C", str(bdir), f"-j{os.cpu_count()}"],
                           capture_output=True, text=True)
        binp = bdir / f"translist_sync_{theme}_test"
        if m.returncode != 0 or not binp.exists():
            errs = "\n".join(l for l in m.stdout.splitlines() + m.stderr.splitlines()
                             if "error:" in l or "undefined reference" in l)
            print(f"    [{theme}] build failed:\n{errs[-2500:]}")
            return False
        # offscreen QPA + blank DISPLAY: the model constructs its QIcons without ever reaching
        # a real X display.
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen", DISPLAY="")
        r = subprocess.run([str(binp)], capture_output=True, text=True, timeout=120, env=env)
        for line in (r.stdout + r.stderr).strip().splitlines():
            if "propagateSizeHints" not in line:   # harmless offscreen QPA chatter
                print(f"    [{theme}] {line}")
        ok = (r.returncode == 0)
        print(f"    [{theme}] {'PASS' if ok else 'FAIL'}")
        return ok
    finally:
        shutil.rmtree(bdir, ignore_errors=True)


def run(backends=None, memcheck=H.NONE) -> bool:
    ok = True
    for theme in _THEMES:
        if not _run_one(theme):
            ok = False
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
