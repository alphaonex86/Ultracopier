#!/usr/bin/env python3
"""Async CONTENT-corruption detect + corrupt-dest CLEANUP (#25), via the LD_PRELOAD shim.

The shim's `wflip:<substr>:<off>` SILENTLY bit-flips one byte of a matching DEST write but returns the
FULL count -- so the write "succeeds" and the file looks complete, exactly like a torn write / bad RAM /
a lying disk. With the checksum-verify ON (`checksum=true,checksumOnlyOnError=false`) the engine re-reads
the finished destination, computes xxh3, finds it != the source, and must:
  * DETECT it (fileContentError), and
  * REMOVE the corrupt destination so a full-size WRONG file is NEVER left looking like a complete copy
    (the #25 fix; gated by destinationIsOursToRemove() so a user's pre-existing dest is never deleted).

REQUIRED: job completes (no hang/crash), the good sibling is byte-perfect, AND the corrupt victim is
ABSENT from the destination (detected + removed -- not delivered). Memory under cap, zero mem-errors.

ASYNC only: `wflip` is a libc-level write fault, so it reaches the async data plane (io_uring's ring
writes bypass libc -- that path is covered by cases/iouring_write_corruption.py via the writable FUSE)."""
import os
import sys
import shutil
import pathlib
import subprocess

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

VICTIM = "victim_corrupt.dat"
VSIZE = 1024 * 1024
OFF = 524288
GOODS = {"good_a.dat": 300000, "good_b.dat": 64000}


def _mktree(root: str) -> str:
    r = pathlib.Path(root)
    if r.exists():
        shutil.rmtree(r)
    r.mkdir(parents=True)
    (r / VICTIM).write_bytes(bytes((i * 7) & 0xFF for i in range(VSIZE)))
    for name, n in GOODS.items():
        (r / name).write_bytes(bytes((i * 11) & 0xFF for i in range(n)))
    return str(r)


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.ASYNC not in backends:
        print("    [skip] write_corruption_async is async-only (libc wflip)")
        return True
    src = _mktree(K.fresh_src_root("wcorr_src"))
    dest = K.fresh_dest("wcorr_dest")
    copied = os.path.join(dest, os.path.basename(src))
    K.with_scenario(f"wflip:{VICTIM}:{OFF}")
    r = H.run(H.ASYNC, "cp", [src], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
              expect_dir=None, fs_preload=K.fs_so(), memcheck=memcheck,
              extra_options={"checksum": "true", "checksumOnlyOnError": "false"})
    K.with_scenario("")

    good_ok = True
    for name in GOODS:
        d = subprocess.run(["cmp", "-s", os.path.join(src, name), os.path.join(copied, name)],
                           capture_output=True)
        if d.returncode != 0:
            good_ok = False
            print(f"      good sibling corrupt/missing: {name}")
    victim_path = os.path.join(copied, VICTIM)
    victim_present = os.path.exists(victim_path)
    victim_removed = not victim_present   # #25: corrupt dest must be removed, not delivered

    ok = (r.completed and r.stayed_alive and good_ok and victim_removed
          and not r.oom_killed and r.mem_errors == 0)
    print(f"      completed={r.completed} alive={r.stayed_alive} good_ok={good_ok} "
          f"victim_present={victim_present} (corrupt_removed={victim_removed}) mem_err={r.mem_errors}")
    if victim_present:
        sz = os.path.getsize(victim_path)
        print(f"        FAIL #25: corrupt victim LEFT at full size ({sz}/{VSIZE}) -- delivered as complete")
    print(f"    [write_corruption_async] {'PASS' if ok else 'FAIL'}  "
          f"(checksum-verify detects the silent byte-flip and REMOVES the corrupt destination)")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
