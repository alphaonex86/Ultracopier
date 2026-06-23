#!/usr/bin/env python3
"""Shared helpers for the test cases under ../cases/.

Every case:
  * builds a synthetic source via synthtree.make_tree()
  * gets a fresh, wiped scratch destination from the harness config
    ([paths] DESTINATIONLINUX), optionally pre-populated for collision cases
  * iterates the requested backends and asserts on the harness Result.

These helpers keep each case file small and readable.
"""
from __future__ import annotations
import os, shutil, subprocess, pathlib
from . import harness as H

DEFAULT_BACKENDS = [H.ASYNC, H.IO_URING]

_FSOVERRIDE = pathlib.Path(__file__).resolve().parents[1] / "fsoverride"


def fs_so() -> str:
    """Build (if needed) the LD_PRELOAD filesystem-condition shim and return the
    absolute path to fsoverride/build/fs_preload.so.

    A case that wants to inject a filesystem condition does:

        os.environ["UC_FS_SCENARIO"] = "readfail:BADSECTOR"
        r = H.run(H.ASYNC, "cp", [src], dest,
                  file_error=H.FileError.PUT_TO_END,
                  fs_preload=K.fs_so(), ...)

    The harness sets LD_PRELOAD=<so> and forwards UC_FS_SCENARIO from os.environ,
    so the running ultracopier sees the injected source/dest I/O faults. Remember
    to clear/reset UC_FS_SCENARIO between sub-runs (it persists in os.environ)."""
    out = _FSOVERRIDE / "build" / "fs_preload.so"
    # Always run make: it is a no-op when the .so is already up to date, and it
    # rebuilds after an edit to linux/fs_preload.c.
    subprocess.run(["make", "-C", str(_FSOVERRIDE)], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    if not out.exists():
        raise RuntimeError(f"fs_preload.so was not built at {out}")
    return str(out)


def with_scenario(scenario: str) -> None:
    """Set UC_FS_SCENARIO in os.environ (the harness forwards it to the binary).
    Pass '' to disable. Convenience so cases don't poke os.environ directly."""
    os.environ["UC_FS_SCENARIO"] = scenario


def _scratch_root() -> pathlib.Path:
    cfg = H.load_config()
    root = cfg.get("paths", "DESTINATIONLINUX", fallback="/tmp/ultracopier-spec-test")
    p = pathlib.Path(os.path.abspath(root))
    assert p.is_absolute()
    return p


def fresh_dest(name: str) -> str:
    """Return a wiped, freshly-created scratch destination directory (absolute)."""
    dest = _scratch_root() / name
    if dest.exists() or dest.is_symlink():
        if dest.is_dir() and not dest.is_symlink():
            shutil.rmtree(dest)
        else:
            dest.unlink()
    dest.mkdir(parents=True, exist_ok=True)
    return str(dest)


def fresh_src_root(name: str) -> str:
    """Absolute path under the scratch root to hold a generated source tree."""
    return str(_scratch_root() / name)


def copied_root(dest: str, src: str) -> str:
    """ultracopier copies the source FOLDER into dest -> dest/<basename(src)>."""
    base = os.path.basename(os.path.normpath(src))
    cand = os.path.join(dest, base)
    return cand if os.path.isdir(cand) else dest


def write_file(path: str, data: bytes) -> None:
    p = pathlib.Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_bytes(data)


def read_file(path: str) -> bytes:
    return pathlib.Path(path).read_bytes()


def for_backends(backends, fn) -> bool:
    """Run fn(backend) for each backend; return True only if all return True.
    Prints a per-backend line so individual cases are debuggable on their own.

    IOCP is SKIPPED here (printed as SKIP, counts as PASS) because the async/io_uring cases
    drive the in-process harness.run(), which only supports async/io_uring. IOCP is covered
    end-to-end by cases/iocp_parity.py via the Windows SSH lane (winlane.run_windows)."""
    backends = backends or DEFAULT_BACKENDS
    ok = True
    for b in backends:
        if b == H.IOCP:
            print(f"    [{b}] SKIP (IOCP is covered by iocp_parity.py via the Windows lane)")
            continue
        try:
            res = fn(b)
        except Exception as e:           # a thrown assertion/exception == failure
            print(f"    [{b}] EXCEPTION: {e!r}")
            ok = False
        else:
            print(f"    [{b}] {'PASS' if res else 'FAIL'}")
            ok = ok and bool(res)
    return ok
