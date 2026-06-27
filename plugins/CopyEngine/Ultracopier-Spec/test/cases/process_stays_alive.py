#!/usr/bin/env python3
"""Process-stays-alive-after-copy: after a copy completes, the ultracopier process must NOT
exit. It is a tray-resident app and is killed externally (by pkill or by the harness after
`stay_alive_seconds`). A clean exit, or a crash within the stay-alive window, is a failure.

This guards against a class of bugs where the engine's teardown code (after the last file
is transferred) calls `QCoreApplication::exit()` or crashes during cleanup, causing the
process to disappear before the user can interact with the result. The user sees "the
process not exist any more after a copy" — exactly this scenario.

The test runs the async backend (the stable reference) with a small synthetic tree,
waits for completion, then waits the full `stay_alive_seconds` window and checks that the
process is still alive. It runs on both async and io_uring on Linux, AND via winlane on
the Windows laptop (mandatory for IOCP, per CLAUDE.md).
"""
import sys, os, time, signal, subprocess, pathlib, shutil, tempfile

_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K


def _one_run(backend: str, stay_seconds: int = 30) -> bool:
    """Run a copy and verify the process is still alive `stay_seconds` after completion."""
    src = synthtree.make_tree(K.fresh_src_root("alive_src"), profile="default")
    dest = K.fresh_dest("alive_dest")

    # Use a longer stay_alive_seconds to catch late crashes (the harness default is 10).
    r = H.run(backend, "cp", [src], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,
              expect_dir=src,
              stay_alive_seconds=stay_seconds,
              memcheck=H.NONE)

    ok = r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0
    if not ok:
        print(f"      [{backend}] FAIL: completed={r.completed} alive={r.stayed_alive} "
              f"oom={r.oom_killed} mem_err={r.mem_errors} exit={r.exit_code}")
    else:
        print(f"      [{backend}] PASS  stayed_alive={stay_seconds}s")
    shutil.rmtree(K._scratch_root(), ignore_errors=True)
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    bks = backends or [H.ASYNC, H.IO_URING]
    avail = []
    for b in bks:
        try:
            H.binary_for(b, H.load_config(), asan=False)
            avail.append(b)
        except Exception:
            pass

    results = []
    for backend in avail:
        res = _one_run(backend)
        results.append((backend, res))

    ok = all(r[1] for r in results)
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)