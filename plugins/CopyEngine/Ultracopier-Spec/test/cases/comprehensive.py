#!/usr/bin/env python3
"""Comprehensive copy: EVERY kind and size of file (size matrix straddling sector/page/64K/1M
boundaries, a big multi-chunk file, sparse, hardlink, every symlink shape, deep nesting,
permission modes, hostile/unicode names). Asserts a byte-for-byte copy + key invariants, and
reports the ones ultracopier may legitimately not preserve (hardlink-sharing, sparseness)."""
import sys, pathlib, os
# Strip the cases/ dir from sys.path so the stdlib `copy` (imported by dataclasses) is never
# shadowed by cases/copy.py when this file is run directly.
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K


def run(backends=None, memcheck=H.NONE) -> bool:
    src = synthtree.make_tree(K.fresh_src_root("comp_src"), "comprehensive", big_mb=48)

    def one(backend):
        dest = K.fresh_dest("comp_dest")
        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.ASK,         # fresh dest -> no collision
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=src, memcheck=memcheck)
        copied = K.copied_root(dest, src)

        def _stat(*p):
            try: return os.stat(os.path.join(copied, *p))
            except OSError: return None
        a, b = _stat("hardlinks", "original.dat"), _stat("hardlinks", "hardlink_to_original.dat")
        hardlink_shared = bool(a and b and a.st_ino == b.st_ino)     # informational
        sp = _stat("kinds", "sparse_8mib.bin")
        sparse_kept = bool(sp and sp.st_blocks * 512 < sp.st_size)   # informational
        ex, ro = _stat("perms", "exec.sh"), _stat("perms", "readonly.dat")
        mode_ok = bool(ex and ro and (ex.st_mode & 0o777) == 0o755 and (ro.st_mode & 0o777) == 0o444)

        # PASS = the copy completed, stayed alive, content+symlinks byte-identical, modes kept,
        # under the computed memory cap. hardlink-sharing/sparseness are reported, not required.
        ok = r.ok and r.content_ok and mode_ok
        print(f"      content={r.content_ok} mode={mode_ok} peak_rss={r.peak_rss_mb}MB | "
              f"info: hardlink_shared={hardlink_shared} sparse_kept={sparse_kept}")
        if not ok:
            print(f"      not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"oom={r.oom_killed} mem_errors={r.mem_errors}\n{r.diff_text}\n{r.notes}")
        return ok

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
