#!/usr/bin/env python3
"""Memory: copy ~400 MB under a hard memory cap. The pipeline must stay bounded:
not OOM-killed, peak RSS below the cap, no memcheck errors, copy completes and the
process survives teardown."""
import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K

TOTAL_MB = 400
MEM_CAP_MB = 1024     # matches config default; the copy must fit well under this


def run(backends=None, memcheck=H.NONE) -> bool:
    src = synthtree.make_tree(K.fresh_src_root("mem_src"), "large", total_mb=TOTAL_MB)

    def one(backend):
        dest = K.fresh_dest("mem_dest")
        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=src, mem_limit_mb=MEM_CAP_MB, memcheck=memcheck)

        bounded = (not r.oom_killed and r.peak_rss_mb < MEM_CAP_MB)
        # When systemd-run is present, we expect the hard cgroup cap to be used.
        # If hard_capped is False but systemd-run exists, that's a warning (not a failure)
        # because valgrind mode disables the cap. We'll just log it.
        if r.hard_capped:
            print(f"      hard cgroup cap active (systemd-run)")
        else:
            import shutil
            if shutil.which("systemd-run") and memcheck != H.VALGRIND:
                print(f"      WARNING: systemd-run present but hard_capped=False (maybe valgrind?)")
        good = (r.completed and r.stayed_alive and r.content_ok and
                r.mem_errors == 0 and bounded)
        if not good:
            print(f"      not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"content={r.content_ok} oom={r.oom_killed} "
                  f"peak_rss_mb={r.peak_rss_mb} cap={MEM_CAP_MB} "
                  f"mem_errors={r.mem_errors}\n{r.diff_text}")
        return good

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
