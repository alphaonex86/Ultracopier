#!/usr/bin/env python3
"""Filename robustness: copy a tree of files whose NAMES contain hostile-but-valid
bytes (control chars, invalid UTF-8, embedded space/tab/newline, trailing dot/space,
leading '-', shell metachars, a >200-char name). On ext4 only '/' and NUL are
forbidden, so every one of these is a legal name that Ultracopier must copy AS-IS,
byte-for-byte -- it must never sanitise, re-encode, or normalise a filename.

Asserts r.ok and r.content_ok (a clean diff -rq --no-dereference of the copied tree
against the source proves names AND content round-tripped exactly). Runs on BOTH
backends: weird names are a metadata/scan concern shared by async and io_uring, so
the io_uring data-plane limitation does not apply here."""
import sys, pathlib, os
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
# When run directly (python3 cases/weird_names.py) Python prepends the script's own
# dir (cases/) to sys.path, where cases/copy.py would SHADOW the stdlib `copy` module
# (imported transitively by dataclasses inside lib.harness) and break the import.
# Drop the cases/ dir and add the test/ root so packages resolve as `lib.*`/`cases.*`.
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K


def run(backends=None, memcheck=H.NONE) -> bool:
    src = synthtree.make_tree(K.fresh_src_root("weird_src"), "weird_names")
    base = os.path.basename(src)

    def one(backend):
        # No filesystem fault here: clear any scenario a previous case left set.
        K.with_scenario("")
        dest = K.fresh_dest("weird_dest")
        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.ASK,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=src, memcheck=memcheck)

        # Spot-check at the bytes level that a representative hostile name actually
        # landed verbatim in the destination (diff covers all of them; this makes a
        # failure legible). The newline-in-name file is the most likely to be mangled.
        copied = os.path.join(dest, base)
        entries = os.listdir(os.fsencode(copied))
        has_newline = any(b"line1\nline2.dat" == e for e in entries)
        has_trailing_space = any(e == b"trailing_space " for e in entries)
        names_ok = has_newline and has_trailing_space

        if not (r.ok and r.content_ok and names_ok):
            print(f"      not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"content={r.content_ok} names_ok={names_ok} "
                  f"mem_errors={r.mem_errors}\n{r.diff_text}")
        return r.ok and r.content_ok and names_ok

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
