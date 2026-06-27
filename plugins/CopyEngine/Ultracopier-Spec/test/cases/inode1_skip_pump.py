#!/usr/bin/env python3
"""Regression: inodeThreads=1 + a collision SKIP must NOT drop the later files.

The bug (see project_inode1_skip_pump_stall): with a single transfer thread, when an early
file resolves to SKIP, the async backend's lone thread emitted postOperationStopped via the
SHARED TransferThread::destinationExists() skip path WITHOUT setting the async-only
`sended_state_postOperationStopped` flag, so async readyForReuse() stayed false, the scheduler
never re-pumped the idle thread, and EVERY later file in the job was silently dropped (data
loss). io_uring/IOCP readyForReuse() is always true (immune); >=2 threads hid it (a fresh
thread picked up the next file). Fixed by setting the flag reactively on ANY postOperationStopped
emission (async/TransferThreadAsync.cpp run()).

Setup: a source whose ALPHABETICALLY-FIRST file collides with a pre-seeded destination file and
is SKIPPED (fileCollision=Skip); all later files are brand new and MUST be copied. inodeThreads=1
forces the single-thread scheduler. Pre-fix async drops the later files; the fix copies them all.

Asserts (all backends, inodeThreads=1):
  * the skipped colliding file keeps its pre-existing (OLD) content;
  * EVERY later new file is present with its correct (NEW) content -- none dropped.
"""
import sys, pathlib, os, shutil
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

OLD = b"OLD pre-existing destination content (must survive the skip)\n"
# alphabetical order matters: the colliding/skip file must be processed FIRST so, if the pump
# stalls after its skip, the later NEW files are the ones that would be dropped.
COLLIDE = "aaa_collide.dat"
NEW_FILES = ["bbb_new.dat", "ccc_new.dat", "mmm_new.dat", "zzz_new.dat"]
def _newdata(name): return b"NEW source content for " + name.encode() + b"\n"


def _build_src(root):
    shutil.rmtree(root, ignore_errors=True)
    os.makedirs(root)
    K.write_file(os.path.join(root, COLLIDE), _newdata(COLLIDE))  # collides -> skipped
    for n in NEW_FILES:
        K.write_file(os.path.join(root, n), _newdata(n))           # brand new -> must copy
    return root


def run(backends=None, memcheck=H.NONE) -> bool:
    src = _build_src(K.fresh_src_root("inode1_src"))
    base = os.path.basename(src)

    def _check_dest(read_collide, read_new, label):
        ok = True
        # the skipped colliding file keeps the pre-existing OLD content
        if read_collide(COLLIDE) != OLD:
            print(f"      [{label}] colliding file not kept (got {read_collide(COLLIDE)!r})")
            ok = False
        # every later NEW file must be present + correct -- the pump-stall dropped these
        for n in NEW_FILES:
            got = read_new(n)
            if got != _newdata(n):
                print(f"      [{label}] later file DROPPED/wrong: {n} -> {got!r}")
                ok = False
        return ok

    def linux_one(backend):
        dest = K.fresh_dest("inode1_dest")
        K.write_file(os.path.join(dest, base, COLLIDE), OLD)   # pre-seed only the colliding file
        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.SKIP,
                  folder_collision=H.FolderCollision.MERGE,
                  inode_threads=1, expect_dir=None, memcheck=memcheck)
        copied = K.copied_root(dest, src)
        def rc(n):
            p = os.path.join(copied, n); return K.read_file(p) if os.path.exists(p) else b"<MISSING>"
        if not (r.completed and r.stayed_alive and r.mem_errors == 0):
            print(f"      run not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"mem_errors={r.mem_errors}\n{r.notes}")
            return False
        return _check_dest(rc, rc, backend)

    def iocp_one():
        from lib import winlane
        if not K.windows_host_configured():
            print("    [iocp] SKIP (no Windows host configured)")
            return True
        seed = K.fresh_src_root("inode1_seed")
        shutil.rmtree(seed, ignore_errors=True); os.makedirs(seed)
        K.write_file(os.path.join(seed, COLLIDE), OLD)

        def pv(box, dest, srcs):
            copied = winlane.win_join(dest, base)
            problems = []
            # colliding file kept (still present); later files must all exist
            for n in [COLLIDE] + NEW_FILES:
                p = winlane.win_join(copied, n)
                out = box.ps(f"if(Test-Path -LiteralPath '{p}'){{'Y'}}else{{'N'}}")
                if "Y" not in out.stdout:
                    problems.append(f"{n} MISSING on box")
            if problems:
                return False, "; ".join(problems)
            return True, "all later files present (no pump stall)"

        r = winlane.run_windows("cp", [src],
                                file_collision=H.FileCollision.SKIP,
                                folder_collision=H.FolderCollision.MERGE,
                                extra_options={"inodeThreads": "1"},
                                dest_pre_seed=seed, post_verify=pv)
        if not r.ok:
            print(f"      [iocp] FAIL: completed={r.completed} alive={r.stayed_alive} "
                  f"content={r.content_ok} crashes={r.mem_errors} notes={r.notes}")
        return r.ok

    return K.for_option_backends(backends, linux_one, iocp_one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
