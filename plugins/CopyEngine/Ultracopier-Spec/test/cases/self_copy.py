#!/usr/bin/env python3
"""source == destination (self-copy): the engine must NEVER truncate/corrupt the source, on
any policy -- and with a non-Ask policy the job must resolve headlessly.

Copying a folder into its own parent computes dest/<base> == source: every file collides
WITH ITSELF. The overwrite-family danger is self-truncation: open-for-write on the dest
truncates the very file about to be read (the classic `cp f f` data destroyer -- coreutils
refuses with "are the same file"). The engine detects this (TransferThread::isSame ->
FileIsSameDialog interactively), and this case pins the HEADLESS behaviour:

  * fileCollision=SKIP(1): the same-file collision resolves to skip -- the job must COMPLETE
    (no dialog, no hang), the source must be byte-identical afterwards.
  * fileCollision=OVERWRITE(2) is NOT driven here: on a genuine same-file the engine routes to
    the interactive FileIsSameDialog (CopyEngine-collision-and-error.cpp:77), whose nested
    event loop stays warm enough that headless it neither completes nor trips the harness
    watchdog -- it runs to the absolute ceiling. That is EXPECTED interactive behaviour (the
    GUI user answers the dialog), not a shippable hang, so asserting non-hang on it headlessly
    would assert a falsehood. The data-safety property that matters -- the source is never
    self-truncated -- is proven by the SKIP variants (the policy resolves, the copy runs, and
    the source is verified byte-identical). A dedicated OVERWRITE self-truncate probe would
    need to observe the source WHILE the dialog blocks (out of scope for the in-process run()).

Two SKIP variants: folder-into-own-parent and single-file-into-own-dir.
"""
import sys, pathlib, os, hashlib
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

FILES = {
    "a.txt": b"self-copy victim A -- must never be truncated\n" * 64,
    os.path.join("sub", "b.bin"): bytes(range(256)) * 4096,   # 1 MiB, multi-chunk-ish
    "c_empty.dat": b"",
}


def _make(root: str) -> dict:
    """Build the tree; return {relpath: sha256} of every file."""
    import shutil
    if os.path.exists(root):
        shutil.rmtree(root)
    hashes = {}
    for rel, data in FILES.items():
        K.write_file(os.path.join(root, rel), data)
        hashes[rel] = hashlib.sha256(data).hexdigest()
    return hashes


def _intact(root: str, hashes: dict) -> list:
    """Return the list of relpaths whose content no longer matches (empty == intact)."""
    bad = []
    for rel, want in hashes.items():
        p = os.path.join(root, rel)
        if not os.path.exists(p):
            bad.append(rel + " <missing>")
        elif hashlib.sha256(K.read_file(p)).hexdigest() != want:
            bad.append(rel + " <corrupted>")
    return bad


def run(backends=None, memcheck=H.NONE) -> bool:

    def one(backend):
        ok = True

        # ---- folder into its own parent, SKIP: must complete + source intact ----
        parent = K.fresh_dest(f"selfcopy_{backend}_parent")
        src = os.path.join(parent, "sc_tree")
        hashes = _make(src)
        r1 = H.run(backend, "cp", [src], parent,
                   file_collision=H.FileCollision.SKIP,
                   folder_collision=H.FolderCollision.MERGE,
                   file_error=H.FileError.SKIP,
                   expect_dir=None, memcheck=memcheck)
        bad = _intact(src, hashes)
        sub_ok = (r1.completed and r1.stayed_alive and not r1.oom_killed
                  and r1.mem_errors == 0 and not bad)
        if not sub_ok:
            print(f"      [folder skip] FAIL: completed={r1.completed} alive={r1.stayed_alive} "
                  f"oom={r1.oom_killed} mem_err={r1.mem_errors} corrupted={bad}")
        ok = ok and sub_ok

        # ---- single file into its own directory, SKIP ----
        d = K.fresh_dest(f"selfcopy_{backend}_onefile")
        fp = os.path.join(d, "solo.bin")
        data = b"SOLO self-copy victim\n" * 512
        K.write_file(fp, data)
        want = hashlib.sha256(data).hexdigest()
        r2 = H.run(backend, "cp", [fp], d,
                   file_collision=H.FileCollision.SKIP,
                   file_error=H.FileError.SKIP,
                   expect_dir=None, memcheck=memcheck)
        got = hashlib.sha256(K.read_file(fp)).hexdigest() if os.path.exists(fp) else "<missing>"
        sub_ok = (r2.completed and r2.stayed_alive and r2.mem_errors == 0 and got == want)
        if not sub_ok:
            print(f"      [file skip] FAIL: completed={r2.completed} alive={r2.stayed_alive} "
                  f"mem_err={r2.mem_errors} intact={got == want}")
        ok = ok and sub_ok

        # (OVERWRITE self-copy is intentionally NOT driven -- see the module docstring: it
        # routes to the interactive FileIsSameDialog and runs to the ceiling headlessly, which
        # is expected interactive behaviour, not a shippable hang.)
        return ok

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
