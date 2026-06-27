#!/usr/bin/env python3
"""Cross-FS MOVE + collision RENAME + a write error must NOT delete the source (audit finding #4,
data-loss). On a RENAME collision the engine retargets the incoming file to a renamed path
('data - copy.bin') and the destination variable becomes that renamed path; the source-removal on a move
must still be gated on copyFailed (which includes writeError) -- NOT merely on exists(<renamed dest>) --
or a failed renamed-dest write would unlink the user's only copy.

Deterministic write error (async): pre-seed the destination 'data.bin' (forces the RENAME collision) and
make the destination directory READ-ONLY so the renamed incoming 'data - copy.bin' cannot be created
(open O_WRONLY|O_CREAT -> EACCES -> writeError). Source is on /tmp to force a cross-FS move (copy+delete,
realMove=false) so the delete path is exercised.

REQUIRED: the SOURCE 'data.bin' is still byte-identical (never unlink'd despite writeError); the job
completes; no crash/leak.
"""
import os
import sys
import shutil
import tempfile
import pathlib

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

ORIG = b"SOURCE-bytes-that-MUST-survive-a-renamed-destination-write-error\n" * 4096  # multi-chunk


def run(backends=None, memcheck=H.NONE) -> bool:
    # source on /tmp -> different fs than the scratch dest -> cross-fs move (copy + delete)
    src = pathlib.Path(tempfile.mkdtemp(prefix="mvre-src-", dir="/tmp")) / "mvre_src"
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    (src / "data.bin").write_bytes(ORIG)

    dest = K.fresh_dest("mvre_dest")
    base = os.path.basename(str(src))
    copied = pathlib.Path(dest, base)
    copied.mkdir(parents=True, exist_ok=True)
    (copied / "data.bin").write_bytes(b"OLD-dest-forces-the-RENAME-collision\n" * 16)
    os.chmod(copied, 0o555)  # read-only dir: the renamed 'data - copy.bin' cannot be created -> writeError
    try:
        r = H.run(H.ASYNC, "mv", [str(src)], dest,
                  file_collision=H.FileCollision.RENAME,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.SKIP,
                  folder_error=H.FolderError.SKIP,
                  expect_dir=None)
    finally:
        os.chmod(copied, 0o755)  # restore so the harness can clean up

    sp = src / "data.bin"
    src_ok = sp.exists() and sp.read_bytes() == ORIG
    ok = (r.completed and r.stayed_alive and src_ok
          and not r.oom_killed and r.mem_errors == 0)
    print(f"      source_preserved={src_ok} completed={r.completed} "
          f"alive={r.stayed_alive} mem_err={r.mem_errors}")
    if not src_ok:
        print("        DATA LOSS: move source deleted despite a renamed-destination write error")
    print(f"    [move_collision_rename_error:async] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
