#!/usr/bin/env python3
"""Symlink CYCLE in the source must NOT hang/recurse-infinitely the scan (audit #17, resilience).

Ultracopier copies a symlink AS a symlink (readlink+symlink) -- it must NOT follow a symlinked
*directory* and descend into it, or a cycle (a/to_b -> b, b/to_a -> a) would make the scan recurse
forever. Build exactly that cycle plus a real file on each side and copy the tree.

REQUIRED (all backends): the job COMPLETES (no hang/crash, pkill still does the killing), the two real
files are byte-perfect, and the two directory symlinks are copied AS symlinks with their targets intact
(NOT followed/expanded into copied subtrees). Memory stays under the cap, zero mem-errors."""
import os
import sys
import shutil
import pathlib

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K


def _verify(src: str, copied: str) -> tuple[bool, str]:
    msgs = []
    ok = True
    # real files
    for rel in ("a/real_a.txt", "b/real_b.txt"):
        s, d = os.path.join(src, rel), os.path.join(copied, rel)
        if not os.path.exists(d):
            ok = False; msgs.append(f"missing real file {rel}")
        elif pathlib.Path(s).read_bytes() != pathlib.Path(d).read_bytes():
            ok = False; msgs.append(f"real file differs {rel}")
    # directory symlinks: must be SYMLINKS at the dest (not followed into real dirs), target preserved
    for rel in ("a/to_b", "b/to_a"):
        d = os.path.join(copied, rel)
        if not os.path.islink(d):
            ok = False; msgs.append(f"{rel} is NOT a symlink at dest (cycle was followed/expanded)")
        elif os.readlink(d) != os.readlink(os.path.join(src, rel)):
            ok = False; msgs.append(f"{rel} target changed")
    return ok, "; ".join(msgs)


def run(backends=None, memcheck=H.NONE) -> bool:
    backends = backends or H.available_backends()
    src = pathlib.Path(K.fresh_src_root("cycle_src"))
    if src.exists():
        shutil.rmtree(src)
    (src / "a").mkdir(parents=True)
    (src / "b").mkdir(parents=True)
    (src / "a" / "real_a.txt").write_bytes(b"content-a\n" * 100)
    (src / "b" / "real_b.txt").write_bytes(b"content-b\n" * 100)
    os.symlink("../b", src / "a" / "to_b")   # a/to_b -> b
    os.symlink("../a", src / "b" / "to_a")   # b/to_a -> a  => cycle a/to_b/to_a/to_b/...

    all_ok = True
    for be in backends:
        dest = K.fresh_dest(f"cycle_dest_{be}")
        copied = os.path.join(dest, os.path.basename(str(src)))
        r = H.run(be, "cp", [str(src)], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                  expect_dir=None, memcheck=memcheck)
        content_ok, msg = _verify(str(src), copied)
        ok = (r.completed and r.stayed_alive and content_ok
              and not r.oom_killed and r.mem_errors == 0)
        print(f"      [{be}] completed={r.completed} alive={r.stayed_alive} content_ok={content_ok} "
              f"mem_err={r.mem_errors}")
        if msg:
            print(f"        {msg}")
        print(f"    [symlink_cycle:{be}] {'PASS' if ok else 'FAIL'}")
        all_ok = all_ok and ok
    return all_ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
