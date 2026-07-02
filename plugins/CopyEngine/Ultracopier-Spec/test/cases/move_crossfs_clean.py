#!/usr/bin/env python3
"""Clean CROSS-FILESYSTEM move of a full tree -- the copy+delete+bottom-up-rmdir pipeline that
NO other case runs cleanly (every clean move case is same-fs and takes the whole-tree rename()
fast path; the cross-fs cases are all fault-injection scenarios asserting survival, never the
nominal complete-move outcome).

Source: a synthetic 'default' tree (nested dirs, symlinks incl. a DANGLING one, unicode
names, a 0-byte file, a multi-chunk >blockSize file) on /tmp (tmpfs); dest on the scratch fs
(different st_dev -> isSameDrive()==false -> per-file copy + per-file delete + bottom-up
folder removal).

ASSERTS:
  * the job completes, stays alive, no OOM, no memcheck errors;
  * dest == a pristine pre-move reference: `diff -rq --no-dereference` == 0 (content AND
    symlink TARGETS -- the dangling symlink must arrive as the same dangling symlink);
  * the SOURCE is completely gone -- every file/symlink unlinked and every dir removed
    bottom-up, the moved root included (a clean, fully-moved tree leaves nothing behind;
    any leftover means a forgotten file or a folder-removal ordering bug).
"""
import sys, pathlib, os, shutil, tempfile
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K
from lib import synthtree


def run(backends=None, memcheck=H.NONE) -> bool:

    def one(backend):
        tmp_parent = tempfile.mkdtemp(prefix="uc-mxc-", dir="/tmp")
        try:
            src = synthtree.make_tree(os.path.join(tmp_parent, "mxc_src"), "default")
            dest = K.fresh_dest("mxc_dest")
            # Cross-fs premise guard.
            if os.stat(tmp_parent).st_dev == os.stat(dest).st_dev:
                print("      FAIL-premise: /tmp is not a distinct filesystem on this box")
                return False
            # Pristine reference BEFORE the move (symlinks preserved as symlinks).
            ref = os.path.join(tempfile.mkdtemp(prefix="uc-mxc-ref-", dir="/tmp"), "ref")
            shutil.copytree(src, ref, symlinks=True)

            r = H.run(backend, "mv", [src], dest,
                      file_collision=H.FileCollision.OVERWRITE,
                      folder_collision=H.FolderCollision.MERGE,
                      expect_dir=ref, memcheck=memcheck)

            # Source must be COMPLETELY gone (bottom-up rmdir incl. the root).
            leftover = []
            if os.path.lexists(src):
                if os.path.isdir(src):
                    for dp, dn, fn in os.walk(src):
                        leftover += [os.path.join(dp, f) for f in fn]
                        leftover += [os.path.join(dp, d) + "/" for d in dn]
                leftover.append(src + "/")
            sub_ok = (r.ok and not leftover)
            if not sub_ok:
                print(f"      FAIL: ok={r.ok} completed={r.completed} alive={r.stayed_alive} "
                      f"content={r.content_ok} mem_err={r.mem_errors} "
                      f"source_leftover={leftover[:8]}{'...' if len(leftover) > 8 else ''}"
                      f"\n{r.diff_text if not r.content_ok else ''}")
            shutil.rmtree(os.path.dirname(ref), ignore_errors=True)
            return sub_ok
        finally:
            shutil.rmtree(tmp_parent, ignore_errors=True)

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
