#!/usr/bin/env python3
"""Media DEST reconnect on the io_uring data plane, END-TO-END (coverage-audit gap 17).

The mirror of reconnect_resume_uring but on the WRITE side: the DESTINATION is a FUSE mount
whose writes fail EIO for a few attempts past a deep offset (USB target unplugged mid-file),
then succeed (replugged). On each write fault the engine takes Retry -> the pipelined resume
re-opens/seeks the dest and continues; the file must end BYTE-PERFECT.

media_reconnect covers this on ASYNC via the LD_PRELOAD shim's wdisconnect verb, but io_uring
submits its writes through the ring (not libc write()), so the shim can't fault them -- a
FUSE-backed write IS serviced by our daemon regardless, so the fault reaches the ring. This is
the new UC_FUSE_WFAIL=<substr>:<off>:<n> mode (LOUD EIO write failure, the exact write-side
analog of UC_FUSE_FAULT).

Retry is driven headlessly by the same virtual FileErrorDialogHook (env
ULTRACOPIER_TEST_FILE_ERROR_ACTION=retry + fileError=Ask) -- no test code in the engine.

ASSERTS: completed + stayed_alive (recovered, no hang/crash); the faulted file byte-perfect;
every always-good sibling byte-perfect; no OOM, zero mem-errors. io_uring ONLY (IOCP dest
reconnect is verified on the Windows box); env-gated on libfuse3 + fusermount3.
"""
import sys, os, pathlib, shutil, subprocess, time, filecmp
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K

VICTIM = "movie_DSTDROP.dat"
SIZE = 32 * 1024 * 1024
OFF = 20 * 1024 * 1024      # fault deep so a large prefix is already written before the drop
RETRIES = 3
_FSO = _TEST_DIR / "fsoverride"


def _fuse_bin():
    if not (pathlib.Path("/dev/fuse").exists() and shutil.which("fusermount3")):
        return None
    if subprocess.run(["pkg-config", "--exists", "fuse3"]).returncode != 0:
        return None
    out = _FSO / "build" / "fuse_fault"
    subprocess.run(["make", "-C", str(_FSO), "fuse_fault"], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    return out if out.exists() else None


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IO_URING not in backends:
        print("    [skip] reconnect_dest_uring is io_uring-only")
        return True
    fuse = _fuse_bin()
    if fuse is None:
        print("    [skip] libfuse3/fusermount3 unavailable -> cannot fault io_uring data writes")
        return True

    # Source is a normal dir; the DESTINATION is the FUSE mount (writes faulted).
    src = pathlib.Path("/tmp/uc-dstrec-src")
    back = pathlib.Path("/tmp/uc-dstrec-back")     # real bytes behind the mount
    mnt = pathlib.Path("/tmp/uc-dstrec-mnt")       # the faulting dest
    subprocess.run(["fusermount3", "-u", str(mnt)], stderr=subprocess.DEVNULL)
    for d in (src, back, mnt):
        shutil.rmtree(d, ignore_errors=True); d.mkdir(parents=True)
    (src / VICTIM).write_bytes(synthtree._seeded_bytes_fast(7, SIZE))
    for i in range(3):
        (src / f"ok_{i}.txt").write_bytes(synthtree._seeded_bytes_fast(50 + i, 6000 + i))

    fenv = dict(os.environ, UC_FUSE_BACKING=str(back),
                UC_FUSE_WFAIL=f"DSTDROP:{OFF}:{RETRIES}")
    subprocess.run([str(fuse), str(mnt), "-s"], env=fenv, check=True)
    time.sleep(1)
    if subprocess.run(["mountpoint", "-q", str(mnt)]).returncode != 0:
        print("    [skip] FUSE mount failed (sandbox restriction)")
        return True

    os.environ["ULTRACOPIER_TEST_FILE_ERROR_ACTION"] = "retry"
    try:
        # dest is the mount itself; ultracopier lands the source folder at <mnt>/<srcname>.
        r = H.run(H.IO_URING, "cp", [str(src)], str(mnt),
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.ASK,     # Ask -> hook injects Retry on each write fault
                  expect_dir=None, inode_threads=1)  # data-plane fault: 1 inode thread (see CLAUDE.md)
        copied = os.path.join(str(back), src.name)   # verify via the BACKING (post-unmount safe)
        vd = os.path.join(copied, VICTIM)
        victim_ok = os.path.exists(vd) and filecmp.cmp(str(src / VICTIM), vd, shallow=False)
        good_ok = all(os.path.exists(os.path.join(copied, f"ok_{i}.txt"))
                      and filecmp.cmp(str(src / f"ok_{i}.txt"),
                                      os.path.join(copied, f"ok_{i}.txt"), shallow=False)
                      for i in range(3))
        ok = (r.completed and r.stayed_alive and victim_ok and good_ok
              and not r.oom_killed and r.mem_errors == 0)
        print(f"    [io_uring dest-reconnect] completed={r.completed} alive={r.stayed_alive} "
              f"victim_ok={victim_ok} good_ok={good_ok} mem_err={r.mem_errors}")
        if not ok:
            print(f"      notes: {(r.notes or '').splitlines()[0] if r.notes else ''}")
        return ok
    finally:
        os.environ.pop("ULTRACOPIER_TEST_FILE_ERROR_ACTION", None)
        subprocess.run(["fusermount3", "-u", str(mnt)], stderr=subprocess.DEVNULL)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
