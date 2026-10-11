#!/usr/bin/env python3
"""io_uring: the speed limit must actually throttle the pipelined backend.

The async reference waits one speed-clock tick every multiForBigSpeed blocks (WriteThread's
waitNewClockForSpeed); the pipelined backends had the clock plumbing (setMultiForBigSpeed,
timeOfTheBlockCopyFinished) but never WAITED on it -- setSpeedLimitation only shrank the block size,
so the UI's "limit speed" did nothing on io_uring (and IOCP): a 1 MB/s limit still saturated the disk.

Driver: unit/engine_api_test.cpp mode `speedlimit` with the io_uring TransferThread: ONE inode
thread, setSpeedLimitation(512 KB/s) for the whole copy of 2 x 1 MiB. At 512 KB/s that is >= 4 s;
the gate is HALF of it (2 s), a LOWER bound -- load can only make the copy slower, never faster, so
the assertion is load-immune. The unthrottled engine finished in well under a second. Content must be
byte-identical and the engine must reach Idle (rc 0)."""
import sys, os, re, pathlib, subprocess, shutil
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import casekit as K
from cases import engine_api_uring as E

NFILES = 2
FSIZE = 1024 * 1024
LIMIT_KBPS = 512
MIN_SECONDS = (NFILES * FSIZE) / (LIMIT_KBPS * 1024) / 2.0


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IO_URING not in backends:
        print("    [skip] iouring_speed_limit is io_uring-only")
        return True
    if subprocess.run(["pkg-config", "--exists", "liburing"]).returncode != 0:
        print("    [skip] liburing unavailable")
        return True
    try:
        binp = E._build()
    except RuntimeError as e:
        print(f"    {e}")
        return False
    src = E._mk_src("speedlimit_src", NFILES, FSIZE)
    dest = K.fresh_dest("speedlimit_dest")
    env = dict(os.environ, HOME=K.fresh_dest("speedlimit_home"), QT_QPA_PLATFORM="offscreen", DISPLAY="")
    rc, out = E._run(binp, src, dest, env, mode="speedlimit")
    m = re.search(r"DONE done=(\d) .* elapsedMs=(\d+)", out)
    done = bool(m and m.group(1) == "1")
    elapsed = int(m.group(2)) / 1000.0 if m else -1.0
    content = E._diff_ok(src, dest)
    throttled = elapsed >= MIN_SECONDS
    ok = rc == 0 and done and content and throttled
    print(f"    [io_uring speed limit] rc={rc} done={done} content_identical={content} "
          f"elapsed={elapsed:.2f}s (min {MIN_SECONDS:.1f}s at {LIMIT_KBPS} KB/s) throttled={throttled} -> {'PASS' if ok else 'FAIL'}")
    if not ok:
        for l in out.strip().splitlines()[-3:]:
            print(f"        {l}")
    shutil.rmtree(src, ignore_errors=True)
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
