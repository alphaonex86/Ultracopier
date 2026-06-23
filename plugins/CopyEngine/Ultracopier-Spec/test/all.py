#!/usr/bin/env python3
"""Discover and run every test case under cases/.

Usage:
  python3 all.py                       # run all cases on all available backends
  python3 all.py copy move skip        # run a subset (by module name)
  python3 all.py --backend async       # only the async backend
  python3 all.py --backend async,io_uring
  python3 all.py --backend async,io_uring,iocp   # also run IOCP via the Windows laptop
  python3 all.py --memcheck valgrind   # none | valgrind | sanitize

Exits nonzero if any case fails, so it can gate a git commit.
Each case is also runnable on its own: python3 cases/copy.py

IOCP note: cases/iocp_parity.py runs the Windows IOCP backend via SSH (winlane). It is SKIPPED
(clean PASS) when [windows] host is empty in config.ini, so CI without the laptop still passes.
Run `python3 all.py --backend iocp` to exercise ONLY the IOCP lane.
"""
import sys, os, pathlib, importlib, argparse, time

TEST_DIR = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(TEST_DIR))
from lib import harness as H

CASES_DIR = TEST_DIR / "cases"

_BACKENDS = {"async": H.ASYNC, "io_uring": H.IO_URING, "iocp": H.IOCP}
_MEMCHECK = {"none": H.NONE, "valgrind": H.VALGRIND, "sanitize": H.SANITIZE}


def discover():
    names = []
    for p in sorted(CASES_DIR.glob("*.py")):
        if p.stem != "__init__":
            names.append(p.stem)
    return names


def parse_args(argv):
    ap = argparse.ArgumentParser(description="Ultracopier-Spec test suite")
    ap.add_argument("cases", nargs="*", help="case module names to run (default: all)")
    ap.add_argument("--backend", default="async,io_uring",
                    help="comma list of backends: async,io_uring,iocp")
    ap.add_argument("--memcheck", default="none",
                    choices=list(_MEMCHECK), help="memory checker")
    return ap.parse_args(argv)


def main(argv):
    args = parse_args(argv)
    available = discover()
    selected = args.cases or available
    unknown = [c for c in selected if c not in available]
    if unknown:
        print(f"unknown case(s): {', '.join(unknown)}")
        print(f"available: {', '.join(available)}")
        return 2

    req_backends = [b.strip() for b in args.backend.split(",") if b.strip()]
    bad = [b for b in req_backends if b not in _BACKENDS]
    if bad:
        print(f"unknown backend(s): {', '.join(bad)} (valid: {', '.join(_BACKENDS)})")
        return 2
    backends = [_BACKENDS[b] for b in req_backends]
    memcheck = _MEMCHECK[args.memcheck]

    results = {}
    for name in selected:
        print(f"==> {name}  (backends={','.join(backends)}, memcheck={args.memcheck})")
        mod = importlib.import_module(f"cases.{name}")
        t0 = time.time()
        try:
            ok = bool(mod.run(backends=backends, memcheck=memcheck))
        except Exception as e:
            print(f"    EXCEPTION: {e!r}")
            ok = False
        results[name] = (ok, time.time() - t0)

    # PASS/FAIL table
    width = max((len(n) for n in results), default=4)
    print("\n" + "=" * (width + 22))
    print(f"{'CASE'.ljust(width)}  {'RESULT':6}  {'TIME':>8}")
    print("-" * (width + 22))
    n_fail = 0
    for name, (ok, dt) in results.items():
        if not ok:
            n_fail += 1
        print(f"{name.ljust(width)}  {'PASS' if ok else 'FAIL':6}  {dt:7.1f}s")
    print("=" * (width + 22))
    total = len(results)
    print(f"{total - n_fail}/{total} passed, {n_fail} failed")
    return 1 if n_fail else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
