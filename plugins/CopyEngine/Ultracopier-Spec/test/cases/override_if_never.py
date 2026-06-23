#!/usr/bin/env python3
"""'Override only if never identical' (OVERWRITE_IF_NOT_SAME_SIZE_AND_DATE).

Pre-populate dest with:
  (a) a byte-identical file with a matching mtime -> must NOT be rewritten
      (its mtime must stay unchanged; the engine skips identical files).
  (b) a different-size file                       -> must BE overwritten.
"""
import sys, pathlib, os, time
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K

IDENTICAL_NAME = "tiny.txt"          # default profile: b"hello ultracopier\n"
IDENTICAL_DATA = b"hello ultracopier\n"
DIFFSIZE_NAME = "few_kb.dat"         # default profile: 4 KiB


def run(backends=None, memcheck=H.NONE) -> bool:
    src = synthtree.make_tree(K.fresh_src_root("oin_src"), "default")
    base = os.path.basename(src)

    def one(backend):
        dest = K.fresh_dest("oin_dest")
        ident = os.path.join(dest, base, IDENTICAL_NAME)
        diffsz = os.path.join(dest, base, DIFFSIZE_NAME)

        # (a) byte-identical file; align its mtime with the source so "same size and
        #     date" holds and the engine must NOT touch it.
        K.write_file(ident, IDENTICAL_DATA)
        src_mtime = os.stat(os.path.join(src, IDENTICAL_NAME)).st_mtime
        os.utime(ident, (src_mtime, src_mtime))
        pre_stat = os.stat(ident)

        # (b) a file of a DIFFERENT size -> must be overwritten.
        K.write_file(diffsz, b"x")   # 1 byte, source is 4 KiB
        time.sleep(1.1)              # ensure any rewrite changes mtime detectably

        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.OVERWRITE_IF_NOT_SAME_SIZE_AND_DATE,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=src, memcheck=memcheck)

        post_stat = os.stat(ident)
        identical_untouched = (post_stat.st_mtime == pre_stat.st_mtime and
                               K.read_file(ident) == IDENTICAL_DATA)
        diff_overwritten = (os.path.exists(diffsz) and
                            K.read_file(diffsz) == K.read_file(os.path.join(src, DIFFSIZE_NAME)))
        if not (r.ok and r.content_ok and identical_untouched and diff_overwritten):
            print(f"      not ok: completed={r.completed} content={r.content_ok} "
                  f"identical_untouched={identical_untouched} "
                  f"diff_overwritten={diff_overwritten}\n{r.diff_text}")
        return r.ok and r.content_ok and identical_untouched and diff_overwritten

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
