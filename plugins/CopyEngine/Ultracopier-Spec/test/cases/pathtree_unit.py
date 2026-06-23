#!/usr/bin/env python3
"""Build + run the standalone PathTree C++ unit test (test/unit/pathtree_test.cpp), which
directly exercises the engine's tree-structured transfer-list storage: split-folder interning
with shared directory nodes, byte-exact resolve(), reorder permutation-invariance (move
up/down/top/bottom + delete-on-finish), append reusing the shared dir node, 255-byte and
>255-byte and >4096-byte length extremes, the embedded-NUL length-delimited invariant, and
Windows drive-letter roots.

It is compiled and run TWICE -- once as std::string (default INTERNALTYPEPATH) and once with
DEFINES+=WIDESTRING as std::wstring -- so both string types named in the request are covered on
Linux (the wstring path is additionally compile-verified by the Windows/MXE engine build).

PathTree is backend-independent (it lives in the shared ListThread), so this case does not
depend on async/io_uring and runs the same regardless of the requested backends. It is NOT a
GUI run, so it bypasses harness.run(): it returns True only if BOTH variants build and exit 0
(every internal check passed)."""
import sys, os, pathlib, subprocess, tempfile, shutil
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H   # noqa: F401  (kept for signature/symmetry with other cases)

_UNIT_DIR = pathlib.Path(__file__).resolve().parents[1] / "unit"
_PRO = _UNIT_DIR / "pathtree_test.pro"               # engine tree (INTERNALTYPEPATH)
_PRO_STR = _UNIT_DIR / "pathtreestr_test.pro"         # theme tree (std::string)


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def _build_and_run(label: str, widestring: bool, pro: pathlib.Path = _PRO,
                   binary: str = "pathtree_test") -> bool:
    """qmake+make the given unit .pro into a throwaway dir, then run the binary; return exit==0."""
    bdir = pathlib.Path(tempfile.mkdtemp(prefix=f"uc-pathtree-{label}-"))
    try:
        flags = ["CONFIG+=release", "-spec", "linux-g++"]
        if widestring:
            flags.append("DEFINES+=WIDESTRING")
        q = subprocess.run([_qmake(), "-o", str(bdir / "Makefile"), str(pro)] + flags,
                           cwd=bdir, capture_output=True, text=True)
        if q.returncode != 0:
            print(f"    [{label}] qmake FAILED:\n{q.stderr}")
            return False
        m = subprocess.run(["make", f"-j{os.cpu_count()}"], cwd=bdir, capture_output=True, text=True)
        if m.returncode != 0:
            print(f"    [{label}] build FAILED:\n{m.stdout[-2000:]}\n{m.stderr[-2000:]}")
            return False
        binp = bdir / binary
        if not binp.exists():
            print(f"    [{label}] binary not produced at {binp}")
            return False
        r = subprocess.run([str(binp)], capture_output=True, text=True, timeout=120)
        ok = r.returncode == 0
        # echo the test's own summary line(s) so a failure is legible in the suite log
        for line in r.stdout.splitlines():
            if "failure" in line or "FAIL" in line or line.startswith("=="):
                print(f"    [{label}] {line}")
        if not ok:
            print(f"    [{label}] unit test FAILED (exit {r.returncode})\n{r.stdout}\n{r.stderr}")
        return ok
    finally:
        shutil.rmtree(bdir, ignore_errors=True)


def run(backends=None, memcheck=H.NONE) -> bool:
    # engine PathTree: both INTERNALTYPEPATH instantiations (std::string + std::wstring)
    ok_str = _build_and_run("string", widestring=False)
    ok_wstr = _build_and_run("wstring", widestring=True)
    # theme PathTreeStr (std::string): the GUI-side twin used by the interface plugin's model
    ok_theme = _build_and_run("theme", widestring=False, pro=_PRO_STR, binary="pathtreestr_test")
    return ok_str and ok_wstr and ok_theme


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
