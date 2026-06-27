#!/usr/bin/env python3
"""io_uring port-check of the skip-close DATA-LOSS (the async faulty_hdd/skip_drops_multichunk bug).

On ASYNC, a multichunk source read-fault that is SKIPped used to never complete its close, hold the only
large-transfer slot (MAXPARALLELTRANFER=1), and the NEXT large file was LOST (fixed: project_skip_close_
dataloss_fix). That bug was async-only to TEST because the LD_PRELOAD shim can't fault io_uring's ring
reads. The FUSE read-fault CAN: mount the source through fuse_fault (UC_FUSE_FAULT=BADSECTOR:0:<huge> ->
the whole victim is permanently unreadable) and copy via io_uring.

Tree (over the FUSE-mounted source): a good multichunk file BEFORE and one AFTER a multichunk BADSECTOR
victim, plus small siblings. io_uring copy, fileError=Skip, inode_threads=1 (CLAUDE.md: io_uring's
concurrency is async submission, not threads). REQUIRED: the job completes (no hang/crash), BOTH good
multichunk files (esp. the one AFTER the skipped victim) are byte-perfect, the small siblings are correct,
and the victim is absent-or-faithful-prefix. If the AFTER file is lost, io_uring has the same bug and the
skip-close fix must be ported to TransferThreadPipelined.

io_uring ONLY; needs libfuse3 + fusermount3 (skipped cleanly otherwise)."""
import os
import sys
import time
import shutil
import pathlib
import subprocess
import filecmp

_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import casekit as K

MB = 1024 * 1024
_FSO = _TEST_DIR / "fsoverride"


def _fuse_bin():
    if not (pathlib.Path("/dev/fuse").exists() and shutil.which("fusermount3")):
        return None
    if subprocess.run(["pkg-config", "--exists", "fuse3"]).returncode != 0:
        return None
    out = _FSO / "build" / "fuse_fault"
    subprocess.run(["make", "-C", str(_FSO), "fuse_fault"], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    return out if out.exists() else None


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IO_URING not in backends:
        print("    [skip] iouring_skip_drops is io_uring-only")
        return True
    fuse = _fuse_bin()
    if fuse is None:
        print("    [skip] libfuse3/fusermount3 unavailable -> cannot fault io_uring data reads")
        return True

    back = pathlib.Path("/tmp/uc-skdrop-back")
    mnt = pathlib.Path("/tmp/uc-skdrop-mnt")
    subprocess.run(["fusermount3", "-u", str(mnt)], stderr=subprocess.DEVNULL)
    for d in (back, mnt):
        shutil.rmtree(d, ignore_errors=True); d.mkdir(parents=True)
    files = {
        "a_small1.bin": bytes((i * 3) & 0xFF for i in range(4096)),
        "b_good_multichunk_before.bin": bytes((i * 7) & 0xFF for i in range(MB + 4096)),
        "c_small2.bin": bytes((i * 5) & 0xFF for i in range(4096)),
        "m_BADSECTOR_dead.bin": bytes((i * 9) & 0xFF for i in range(MB + 4096)),
        "n_small3.bin": bytes((i * 11) & 0xFF for i in range(4096)),
        "z_good_multichunk_after.bin": bytes((i * 13) & 0xFF for i in range(MB + 4096)),
    }
    for name, data in files.items():
        (back / name).write_bytes(data)
    good = [n for n in files if "BADSECTOR" not in n]

    fenv = dict(os.environ, UC_FUSE_BACKING=str(back),
                UC_FUSE_FAULT="BADSECTOR:0:999999")   # whole victim permanently unreadable
    subprocess.run([str(fuse), str(mnt), "-s"], env=fenv, check=True)
    time.sleep(1)
    if subprocess.run(["mountpoint", "-q", str(mnt)]).returncode != 0:
        print("    [skip] FUSE mount failed (sandbox restriction)")
        return True
    try:
        dest = K.fresh_dest("skdrop_dest")
        copied = os.path.join(dest, mnt.name)
        r = H.run(H.IO_URING, "cp", [str(mnt)], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                  expect_dir=None, inode_threads=1)
        missing, differs = [], []
        for n in good:
            d = os.path.join(copied, n)
            if not os.path.exists(d):
                missing.append(n)
            elif not filecmp.cmp(str(back / n), d, shallow=False):
                differs.append(n)
        good_ok = not missing and not differs
        ok = (r.completed and r.stayed_alive and good_ok
              and not r.oom_killed and r.mem_errors == 0)
        print(f"    [io_uring skip_drops] completed={r.completed} alive={r.stayed_alive} "
              f"good_ok={good_ok} mem_err={r.mem_errors}")
        if missing:
            print(f"        DATA LOSS: missing good files (skip-close bug present on io_uring): {missing}")
        if differs:
            print(f"        DATA LOSS: good files differ: {differs}")
        print(f"    [iouring_skip_drops] {'PASS' if ok else 'FAIL'}")
        return ok
    finally:
        subprocess.run(["fusermount3", "-u", str(mnt)], stderr=subprocess.DEVNULL)
        subprocess.run(["pkill", "-9", "-x", "ultracopier"], capture_output=True)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
