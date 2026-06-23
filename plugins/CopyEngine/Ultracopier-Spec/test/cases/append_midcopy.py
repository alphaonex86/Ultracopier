#!/usr/bin/env python3
"""Mid-copy APPEND (and, implicitly, delete-on-finish) on the live transfer list.

While a primary copy is in progress, a SECOND `ultracopier cp` is fired for an extra tree.
Ultracopier is single-instance (LocalListener / QLocalServer), so the second invocation does
NOT spawn a new process -- it forwards its sources to the already-running instance, which
appends them to the live transfer list (the engine's addToTransfer path, the same path that
interns each new file's source+destination into the PathTree). Both the primary tree and the
late-appended tree must end up copied byte-for-byte.

This is the runtime counterpart to the PathTree unit test's "append reuses the shared dir node"
check: here the append happens through the real binary, mid-flight, and is verified on disk.
"Delete (copy finish)" needs no separate trigger -- the list drains (RemoveItem) as each file
completes, which every passing copy case already exercises and the diff already proves; an item
that was wrongly dropped before transfer would show up as a missing file in the diff.

Asserts: the primary copy is r.ok (completed, stayed alive, primary subtree byte-identical, no
OOM, no mem-errors) AND the late-appended tree is present and byte-identical in the destination.
Runs on both backends -- both trees are plain regular files (no symlinks), so the append/list
path is exercised identically on async and io_uring."""
import sys, os, pathlib, subprocess
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K


def _diff_clean(src: str, copied: str) -> tuple[bool, str]:
    p = subprocess.run(["diff", "-rq", "--no-dereference", src, copied],
                       capture_output=True, text=True)
    return p.returncode == 0, (p.stdout + p.stderr)


def run(backends=None, memcheck=H.NONE) -> bool:
    # Primary tree: big enough that the copy is still running when the append fires (~a few
    # hundred MB of fixed-size chunks). Extra tree: a small distinctive 'default' tree.
    src_main = synthtree.make_tree(K.fresh_src_root("append_main"), "large", total_mb=240)
    src_extra = synthtree.make_tree(K.fresh_src_root("append_extra"), "default")
    extra_base = os.path.basename(src_extra)

    def one(backend):
        K.with_scenario("")
        dest = K.fresh_dest("append_dest")
        r = H.run(backend, "cp", [src_main], dest,
                  file_collision=H.FileCollision.ASK,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=src_main, memcheck=memcheck,
                  append_after=[{"delay": 2, "sources": [src_extra], "mode": "cp"}])

        # the late-appended tree must have landed byte-for-byte under dest/<basename(extra)>
        copied_extra = os.path.join(dest, extra_base)
        extra_ok, extra_diff = _diff_clean(src_extra, copied_extra)

        if not (r.ok and extra_ok):
            print(f"      not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"main_content={r.content_ok} appended_ok={extra_ok} "
                  f"oom={r.oom_killed} mem_errors={r.mem_errors}\n"
                  f"main_diff:\n{r.diff_text}\nappend_diff:\n{extra_diff}\n{r.notes}")
        return r.ok and extra_ok

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
