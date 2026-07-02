#!/usr/bin/env python3
"""folderError policy vs a permission-denied SOURCE directory (a real, common salvage
condition: root-owned/0700 dirs inside the tree being backed up).

opt_mkpath's off-direction already exercises folderError=Skip against a DEST mkdir failure;
the SOURCE side -- a directory the scan cannot enter (opendir EACCES) -- has zero coverage.
The scan runs in its own thread; an unhandled opendir failure can abort the whole listing,
loop, or park on the folder-error dialog (headless wedge).

Source: healthy files + a chmod-000 subdir ('noentry/', containing a file written before the
chmod) + more healthy files AFTER it in sort order. folderError=SKIP, fileError=SKIP.
ASSERTS: the job COMPLETES and stays alive (no hang on the unreadable dir), memory clean;
every readable file is copied byte-perfect -- including the ones sorting AFTER the blocked
dir (a positional abort would strand them); the blocked dir's content did not land (it was
unreadable); the source dir's permission is restored afterwards (test hygiene only).
"""
import os, sys, hashlib, pathlib, shutil, stat
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

READABLE = {
    "a_first.txt": b"readable file before the blocked dir\n" * 8,
    "z_after.dat": bytes((i * 29) & 0xFF for i in range(512 * 1024)),
    os.path.join("sub_ok", "nested.txt"): b"nested readable\n" * 16,
}
BLOCKED_INNER = b"content inside the unreadable dir -- must NOT land (unreadable)\n"


def run(backends=None, memcheck=H.NONE) -> bool:

    def one(backend):
        # Per-backend + wipe (fresh_src_root does not wipe).
        src = K.fresh_src_root(f"fesp_{backend}_src")
        shutil.rmtree(src, ignore_errors=True)
        hashes = {}
        for rel, data in READABLE.items():
            K.write_file(os.path.join(src, rel), data)
            hashes[rel] = hashlib.sha256(data).hexdigest()
        blocked = os.path.join(src, "noentry")
        K.write_file(os.path.join(blocked, "secret.txt"), BLOCKED_INNER)
        os.chmod(blocked, 0)
        dest = K.fresh_dest(f"fesp_{backend}_dest")
        try:
            r = H.run(backend, "cp", [src], dest,
                      file_collision=H.FileCollision.OVERWRITE,
                      folder_collision=H.FolderCollision.MERGE,
                      file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                      expect_dir=None, memcheck=memcheck)
        finally:
            os.chmod(blocked, 0o755)
            # The engine may copy the 000 source dir into dest (perms preserved) -> make every
            # dest dir traversable so the next fresh_dest can rmtree it (test hygiene only).
            for dp, dn, _fn in os.walk(dest):
                for d in dn:
                    try: os.chmod(os.path.join(dp, d), 0o755)
                    except OSError: pass

        copied = os.path.join(dest, os.path.basename(src))
        problems = []
        if not (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0):
            problems.append(f"run: completed={r.completed} alive={r.stayed_alive} "
                            f"oom={r.oom_killed} mem_err={r.mem_errors}"
                            + ("  <-- hang/abort on unreadable source dir" if not r.completed else ""))
        for rel, want in hashes.items():
            p = os.path.join(copied, rel)
            if not os.path.exists(p):
                problems.append(f"readable {rel} missing (stranded by the blocked dir?)")
            elif hashlib.sha256(K.read_file(p)).hexdigest() != want:
                problems.append(f"readable {rel} corrupt")
        leaked = os.path.join(copied, "noentry", "secret.txt")
        if os.path.exists(leaked):
            problems.append("unreadable dir content LANDED at dest (impossible read?)")
        if problems:
            print(f"      [{backend}] FAIL: " + "; ".join(problems))
        return not problems

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
