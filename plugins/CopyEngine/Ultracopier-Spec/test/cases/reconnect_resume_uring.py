#!/usr/bin/env python3
"""Media-reconnect RESUME-at-offset, io_uring, END-TO-END. A source file is read from a FUSE
mount that drops out mid-file (EIO for a few attempts past a deep offset, then reconnects). On
each read fault the engine takes the Retry action -> TransferThreadPipelined::resumeAfterErrorAndSeek
re-stats the source (size + sub-second mtime + ctime) and, finding it byte-identical, RESUMES the
transfer at the contiguous-written low-water mark instead of restarting from 0 -- keeping the
destination prefix and re-reading only the tail.

Why FUSE (not the LD_PRELOAD shim): io_uring submits its data reads through the ring, NOT libc
read(), so the shim can't fault them; a FUSE-backed read IS serviced by our daemon regardless, so
the fault reaches the ring. The daemon also COUNTS source bytes read, which is how we PROVE resume
(total ~= file size) vs restart-from-0 (total ~= file size + fault_offset*retries).

Driving the otherwise-interactive Retry headlessly: FileErrorDialog is created through the virtual
FileErrorDialog::createInstance, and the TEST binary links test/hooks/FileErrorDialogHook.cpp which
subclasses it and returns the action from env ULTRACOPIER_TEST_FILE_ERROR_ACTION without showing the
GUI (production overrideFactory is nullptr -> the real dialog). With it = "retry" and fileError=Ask,
every read fault takes the Retry path -> the resume. No test code lives in the engine itself.

ASSERTS:
  * r.completed and r.stayed_alive               -- recovered, no hang/crash
  * the faulted file is byte-perfect             -- diff of source vs copied
  * every always-good sibling is byte-perfect
  * source bytes read ~= file size (RESUME)      -- NOT file size + offset*retries (restart)
  * not r.oom_killed and r.mem_errors == 0

io_uring ONLY (FUSE fault reaches the ring; IOCP resume is verified on the Windows box). Requires
libfuse3 + fusermount3 (skipped cleanly if unavailable, like other environment-gated cases)."""
import sys, os, pathlib, shutil, subprocess, time, filecmp
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K

VICTIM = "movie_SRCDROP.dat"
SIZE = 32 * 1024 * 1024     # big enough that the deep fault leaves a large, unambiguous prefix
OFF = 20 * 1024 * 1024      # drop deep in the file so the contiguous prefix at fault time is large
RETRIES = 3
_FSO = _TEST_DIR / "fsoverride"


def _fuse_bin() -> "pathlib.Path | None":
    """Build (if needed) and return the FUSE fault injector, or None if libfuse3 is unavailable."""
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
        print("    [skip] reconnect_resume_uring is io_uring-only")
        return True
    fuse = _fuse_bin()
    if fuse is None:
        print("    [skip] libfuse3/fusermount3 unavailable -> cannot fault io_uring data reads")
        return True

    back = pathlib.Path("/tmp/uc-resume-back")
    mnt = pathlib.Path("/tmp/uc-resume-mnt")
    readlog = "/tmp/uc-resume-readlog.txt"
    subprocess.run(["fusermount3", "-u", str(mnt)], stderr=subprocess.DEVNULL)
    for d in (back, mnt):
        shutil.rmtree(d, ignore_errors=True); d.mkdir(parents=True)
    (back / VICTIM).write_bytes(synthtree._seeded_bytes_fast(5, SIZE))
    for i in range(3):
        (back / f"ok_{i}.txt").write_bytes(synthtree._seeded_bytes_fast(40 + i, 5000 + i))
    try:
        os.remove(readlog)
    except OSError:
        pass

    fenv = dict(os.environ, UC_FUSE_BACKING=str(back),
                UC_FUSE_FAULT=f"SRCDROP:{OFF}:{RETRIES}",
                UC_FUSE_READLOG_MATCH="SRCDROP", UC_FUSE_READLOG_PATH=readlog)
    subprocess.run([str(fuse), str(mnt), "-s"], env=fenv, check=True)  # FUSE auto-daemonizes
    time.sleep(1)
    if subprocess.run(["mountpoint", "-q", str(mnt)]).returncode != 0:
        print("    [skip] FUSE mount failed (sandbox restriction)")
        return True

    os.environ["ULTRACOPIER_TEST_FILE_ERROR_ACTION"] = "retry"   # virtual hook -> Retry -> resume
    try:
        dest = K.fresh_dest("resume_uring_dest")
        copied = os.path.join(dest, mnt.name)
        r = H.run(H.IO_URING, "cp", [str(mnt)], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.ASK,     # Ask -> the hook injects Retry on each fault
                  expect_dir=None)
        vd = os.path.join(copied, VICTIM)
        victim_ok = os.path.exists(vd) and filecmp.cmp(str(back / VICTIM), vd, shallow=False)
        good_ok = all(os.path.exists(os.path.join(copied, f"ok_{i}.txt"))
                      and filecmp.cmp(str(back / f"ok_{i}.txt"), os.path.join(copied, f"ok_{i}.txt"), shallow=False)
                      for i in range(3))
        try:
            total = int(pathlib.Path(readlog).read_text().strip())
        except (OSError, ValueError):
            total = -1
        # RESUME reads each source byte ~once (prefix once + tail once); restart-from-0 would re-read
        # the prefix on every one of the RETRIES failed attempts (>= SIZE + RETRIES*OFF).
        resumed = 0 < total < SIZE + OFF
        ok = (r.completed and r.stayed_alive and victim_ok and good_ok and resumed
              and not r.oom_killed and r.mem_errors == 0)
        mb = lambda x: f"{x // 1024 // 1024}MB" if isinstance(x, int) and x >= 0 else "?"
        print(f"    [io_uring resume] completed={r.completed} alive={r.stayed_alive} "
              f"victim_ok={victim_ok} good_ok={good_ok} mem_err={r.mem_errors} | "
              f"bytes_read={mb(total)} (resume~={mb(SIZE)}, restart~={mb(SIZE + RETRIES * OFF)}) resumed={resumed}")
        return ok
    finally:
        os.environ.pop("ULTRACOPIER_TEST_FILE_ERROR_ACTION", None)
        subprocess.run(["fusermount3", "-u", str(mnt)], stderr=subprocess.DEVNULL)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
