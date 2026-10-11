#!/usr/bin/env python3
"""io_uring: the speed limiter changes blockSize from the ListThread while a transfer thread's I/O
buffers are already allocated at the OLD size. Every read must be sized to the buffer actually
allocated (allocSize), never to the live blockSize: a block that GREW (the user removed a speed
limit mid-copy) otherwise makes the kernel write past the malloc'd buffer -> heap corruption and
wrong bytes in the destination. The IOCP backend already clamps to allocSize; io_uring did not.

Driver: unit/engine_api_test.cpp mode `speedchange` linked with the io_uring TransferThread
(engine_api_uring_test.pro): ONE inode thread, setSpeedLimitation(256 KB/s) before the copy (the
limiter shrinks the 64 KB block to 25 KB), then setSpeedLimitation(0) as soon as the first bytes landed
(block back to 64 KB while the 25 KB buffers are live). The source sits on a FUSE mount with a
5 ms per-read delay so the lift lands mid-file, deterministically. glibc's malloc debugging
(libc_malloc_debug + glibc.malloc.check=3) turns a heap overrun into an abort. Asserts: the
driver exits 0 with a clean Idle, the speed limit WAS lifted mid-copy, and the content is
byte-identical."""
import sys, os, re, pathlib, subprocess, shutil
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K
from cases import engine_api_uring as E

NFILES = 4
FSIZE = 12 * 1024 * 1024


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IO_URING not in backends:
        print("    [skip] iouring_speed_change is io_uring-only")
        return True
    if subprocess.run(["pkg-config", "--exists", "liburing"]).returncode != 0:
        print("    [skip] liburing unavailable")
        return True
    try:
        binp = E._build()
    except RuntimeError as e:
        print(f"    {e}")
        return False
    with K.FuseMount("speedchange", UC_FUSE_DELAY="SLOW:5") as m:
        if m is None:
            print("    [skip] libfuse3/fusermount3 unavailable or FUSE mount failed")
            return True
        (m.back / "src").mkdir()
        for i in range(NFILES):
            (m.back / "src" / f"f{i}_SLOW.dat").write_bytes(synthtree._seeded_bytes_fast(500 + i, FSIZE))
        dest = K.fresh_dest("speedchange_dest")
        env = dict(os.environ, HOME=K.fresh_dest("speedchange_home"), QT_QPA_PLATFORM="offscreen", DISPLAY="")
        malloc_dbg = "/usr/lib64/libc_malloc_debug.so"
        if os.path.exists(malloc_dbg):
            env.update(LD_PRELOAD=malloc_dbg, GLIBC_TUNABLES="glibc.malloc.check=3")
        rc, out = E._run(binp, str(m.mnt / "src"), dest, env, mode="speedchange")
        done = bool(re.search(r"DONE done=1", out))
        lifted = bool(re.search(r"SPEEDLIFT at destBytes=", out))
        content = subprocess.run(["diff", "-rq", "--no-dereference", str(m.back / "src"), os.path.join(dest, "src")],
                                 capture_output=True).returncode == 0
    ok = rc == 0 and done and lifted and content
    print(f"    [io_uring speed change] rc={rc} done={done} lifted_mid_copy={lifted} content_identical={content} -> {'PASS' if ok else 'FAIL'}")
    if not ok:
        for l in out.strip().splitlines()[-4:]:
            print(f"        {l}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
