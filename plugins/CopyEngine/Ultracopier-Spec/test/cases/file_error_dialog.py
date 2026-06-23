#!/usr/bin/env python3
"""FileErrorDialog actions, exercised headlessly through the VIRTUAL dialog seam.

With fileError=Ask the engine creates the (otherwise interactive, offscreen-invisible) FileErrorDialog
through FileErrorDialog::createInstance. The TEST binary links test/hooks/FileErrorDialogHook.cpp -- a
FileErrorDialog subclass installed via FileErrorDialog::overrideFactory at static-init that overrides
exec() (never shows) and getAction() (returns the env ULTRACOPIER_TEST_FILE_ERROR_ACTION choice). The
SHIPPING engine has NO test code, #ifdef or env read: overrideFactory is nullptr there and the real GUI
dialog runs. This is the clean "virtual/override the .ui class" approach.

Each dialog button is driven against a faulted source (LD_PRELOAD shim faulting libc reads, async only):
  * Skip     on a dead (readfail) file -> that file is skipped; every good file still copied
  * PutToEnd on a dead file            -> deferred + bounded-retried + skipped; rest copied
  * Cancel   on a dead file            -> the job aborts CLEANLY (stays alive, no crash/leak)

The Retry button is covered separately by reconnect_resume_uring.py (io_uring), where the Retry path is
DETERMINISTIC. It is deliberately NOT asserted on async here: a real modal FileErrorDialog GATES
successive retries by staying up until the human reacts (seconds), so the next Retry can only fire after
the previous attempt has fully failed and the dialog re-appeared. A headless test accepts instantly and
cannot reproduce that gating; async's frozen restart-from-0 recovers a SINGLE fast retry but hangs on an
overlapping second one (verified: flaky:1 recovers in 0.4s, flaky:2+ never does) -- a fast-automated-retry
artifact, not a shipping bug (the GUI is gated). io_uring's resume path is robust to it, so Retry lives
there. (Asserting async Retry here would mean shipping a flaky test, which the suite forbids.)

ASYNC ONLY: the shim faults libc source reads (async ReadThread). The dialog + action dispatch live in
the SHARED CopyEngine, so async coverage validates the dialog logic for every backend."""
import sys, pathlib, os, filecmp
# Running this file directly puts cases/ on sys.path, where cases/copy.py would shadow the stdlib
# `copy` that dataclasses imports -> drop cases/ (and "") before importing the lib package.
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K

DEAD = "disk_BADREAD_block.bin"       # permanently unreadable -> Skip/PutToEnd/Cancel territory


def _tree_with_victim(root: str, victim: str, size: int) -> str:
    """A 'default' synthetic tree plus one >blockSize victim file with the fault marker in its name."""
    root = os.path.abspath(root)
    synthtree.make_tree(root, "default")
    pathlib.Path(root, victim).write_bytes(synthtree._seeded_bytes_fast(7777, size))
    return root


def _good_files_ok(src: str, copied: str, exclude: set) -> bool:
    """Every source file except `exclude` must be present and byte-identical in the copy."""
    for dirpath, _dirs, files in os.walk(src):
        for fn in files:
            if fn in exclude:
                pass
            else:
                sp = os.path.join(dirpath, fn)
                dp = os.path.join(copied, os.path.relpath(sp, src))
                if not (os.path.exists(dp) and filecmp.cmp(sp, dp, shallow=False)):
                    return False
    return True


def _run(action: str, scenario: str, file_error: int):
    """Build a fresh faulted tree, run async cp with fileError=Ask + the scripted dialog action."""
    src = _tree_with_victim(K.fresh_src_root(f"fed_{action}_src"), DEAD, 800 * 1024 + 7)
    dest = K.fresh_dest(f"fed_{action}_dest")
    copied = os.path.join(dest, os.path.basename(src))
    os.environ["ULTRACOPIER_TEST_FILE_ERROR_ACTION"] = action
    K.with_scenario(scenario)
    try:
        r = H.run(H.ASYNC, "cp", [src], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=file_error,            # Ask -> the dialog override injects `action`
                  expect_dir=None,
                  fs_preload=K.fs_so())
    finally:
        K.with_scenario("")
        os.environ.pop("ULTRACOPIER_TEST_FILE_ERROR_ACTION", None)
    return src, copied, r


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.ASYNC not in backends:
        print("    [skip] file_error_dialog is async-only (shim faults libc reads)")
        return True

    results = []

    # SKIP on a dead file -> victim ABSENT, every good file still byte-perfect, no hang.
    src, copied, r = _run("skip", "readfail:BADREAD", H.FileError.ASK)
    skipped = not os.path.exists(os.path.join(copied, DEAD))
    good = _good_files_ok(src, copied, {DEAD})
    ok = (r.completed and r.stayed_alive and skipped and good and not r.oom_killed and r.mem_errors == 0)
    print(f"    [Skip   dead ] completed={r.completed} alive={r.stayed_alive} victim_absent={skipped} "
          f"good_ok={good} mem_err={r.mem_errors} -> {'PASS' if ok else 'FAIL'}")
    results.append(ok)

    # PUT-TO-END on a dead file -> deferred, bounded-retried, finally skipped; rest copied.
    src, copied, r = _run("puttoend", "readfail:BADREAD", H.FileError.ASK)
    skipped = not os.path.exists(os.path.join(copied, DEAD))
    good = _good_files_ok(src, copied, {DEAD})
    ok = (r.completed and r.stayed_alive and skipped and good and not r.oom_killed and r.mem_errors == 0)
    print(f"    [PutEnd dead ] completed={r.completed} alive={r.stayed_alive} victim_absent={skipped} "
          f"good_ok={good} mem_err={r.mem_errors} -> {'PASS' if ok else 'FAIL'}")
    results.append(ok)

    # CANCEL on a dead file -> the whole job aborts, but CLEANLY: process survives, no crash/leak.
    #    (The destination is intentionally partial; we assert the engine handled Cancel without a
    #    crash, OOM or mem-error -- the safety invariant for the dialog's Cancel button.)
    src, copied, r = _run("cancel", "readfail:BADREAD", H.FileError.ASK)
    ok = (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0)
    print(f"    [Cancel dead ] completed={r.completed} alive={r.stayed_alive} "
          f"mem_err={r.mem_errors} oom={r.oom_killed} -> {'PASS' if ok else 'FAIL'}")
    results.append(ok)

    return all(results)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
