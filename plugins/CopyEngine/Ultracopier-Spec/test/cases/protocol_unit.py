#!/usr/bin/env python3
"""Build + run the standalone extractProtocol() unit test (test/unit/protocol_test.cpp).

extractProtocol() classifies every source and destination string Ultracopier is given as either a
LOCAL path or a protocol URL, and Core REFUSES a transfer whose protocols no loaded copy engine
supports. Both directions of a wrong answer are damaging, which is why the classifier is pinned
here rather than only through a running app:

  * a protocol read as "file" is the bug this change fixes -- "sftp://root@host/x" as a source
    silently transferred nothing, and as a destination made the engine create a junk LOCAL folder
    named after the URL ("./sftp:/root@host/XXX") and copy into it;
  * a local path read as a protocol would refuse a perfectly good transfer -- which is exactly what
    a naive "letters before ':'" rule does to the Windows drive letter "C:/Users/...".

The C++ side checks both, plus case-insensitivity, "file://" URIs, and the length-delimited
invariant (an embedded NUL must neither truncate the scan nor let a scheme after it be picked up).

Backend-independent (it is pure string handling in cpp11addition.cpp), so it runs once regardless
of the requested backends. Bypasses harness.run(): it passes only if the test binary builds and
exits 0."""
import sys, os, pathlib, subprocess, tempfile, shutil

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H   # noqa: F401  (signature symmetry with the other cases)

_PRO = pathlib.Path(__file__).resolve().parents[1] / "unit" / "protocol_test.pro"


def run(backends=None, memcheck=H.NONE) -> bool:
    qmake = shutil.which("qmake6") or shutil.which("qmake") or "qmake6"
    bdir = pathlib.Path(tempfile.mkdtemp(prefix="uc-protocol-unit-"))
    try:
        q = subprocess.run([qmake, "-o", str(bdir / "Makefile"), str(_PRO),
                            "CONFIG+=release", "-spec", "linux-g++"],
                           cwd=bdir, capture_output=True, text=True)
        if q.returncode != 0:
            print(f"    [protocol_unit] qmake FAILED:\n{q.stderr}")
            return False
        m = subprocess.run(["make", f"-j{os.cpu_count()}"], cwd=bdir, capture_output=True, text=True)
        if m.returncode != 0:
            print(f"    [protocol_unit] build FAILED:\n{m.stdout[-2000:]}\n{m.stderr[-2000:]}")
            return False
        binp = bdir / "protocol_test"
        if not binp.exists():
            print(f"    [protocol_unit] binary not produced at {binp}")
            return False
        r = subprocess.run([str(binp)], capture_output=True, text=True, timeout=120)
        for line in r.stdout.splitlines():
            if line.startswith("==") or line.startswith("FAIL"):
                print(f"    [protocol_unit] {line}")
        if r.returncode != 0:
            print(f"    [protocol_unit] FAIL (exit {r.returncode})")
            return False
        return True
    finally:
        shutil.rmtree(bdir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
