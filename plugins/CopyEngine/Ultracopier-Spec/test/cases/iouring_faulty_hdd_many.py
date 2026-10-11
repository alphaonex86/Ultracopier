#!/usr/bin/env python3
"""io_uring dying-disk salvage with MANY unreadable files on the ring DATA plane (faulty_hdd is
async-only: the libc shim cannot fault io_uring reads; the single-victim FUSE cases never show a
per-file cost that only hurts at scale).

Every failing file used to cost the engine a 2 s cancel-drain timeout AND up to 8 pipeline buffers:
when a read CQE carried the error the batch loop broke out, so the completions already dequeued in
the same batch were never accounted (readsInFlight stayed high, their buffers stayed 'Reading'); the
drain then waited the full deadline for completions that had already been consumed and QUARANTINED
every such buffer (never reused, freed only when the worker dies). A dying HDD with a few thousand
bad files = hours of stall + gigabytes of leaked buffers -> OOM, the exact backup this engine exists
for. IOCP has the same loop shape.

Rig: a FUSE source where every *BADSECTOR* file faults EIO at offset 0 for every attempt, 20 such
files each > 8 blocks (so 8 reads are in flight when the first one fails), plus good siblings;
blockSize 2 MiB so a leak is measurable; 1 inode thread so legit RSS is flat. Two big GOOD files
sort FIRST so every pipeline buffer has been filled (resident pages) before the dead files arrive:
a quarantined buffer that was never written is only virtual memory and would hide the leak from
RSS. Two runs on the same mount (goods only, then goods+bad): the peak-RSS delta between them is
the leak (a correct engine leaks nothing; the buggy one tens to hundreds of MB), and the wall-time
delta per dead file exposes the 2 s drain stall (bound 1 s/file: a correct engine costs ms)."""
import sys, os, pathlib, filecmp, time
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K

N_BAD = 20
N_GOOD = 10
BLOCK_KB = 2048
BAD_SIZE = 8 * BLOCK_KB * 1024 + 64 * 1024     # > 8 blocks: the whole buffer pool is in flight
LEAK_LIMIT_MB = 64
STALL_LIMIT_S_PER_FILE = 1.0


def _one(mnt: pathlib.Path, back: pathlib.Path, sub: str, tag: str):
    dest = K.fresh_dest(f"faultymany_dest_{tag}")
    t0 = time.monotonic()
    r = H.run(H.IO_URING, "cp", [str(mnt / sub)], dest, file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE, file_error=H.FileError.SKIP, expect_dir=None,
              inode_threads=1, extra_options={"blockSize": str(BLOCK_KB)}, mem_limit_mb=1024)
    elapsed = time.monotonic() - t0
    copied = pathlib.Path(dest) / sub
    goods_bad = [f for f in sorted(os.listdir(back / sub)) if "BADSECTOR" not in f
                 and not ((copied / f).exists() and filecmp.cmp(str(back / sub / f), str(copied / f), shallow=False))]
    victims_wrong = []
    for f in sorted(os.listdir(back / sub)):
        if "BADSECTOR" in f and (copied / f).exists():
            got = (copied / f).read_bytes()
            if got != (back / sub / f).read_bytes()[:len(got)]:
                victims_wrong.append(f)
    return r, elapsed, goods_bad, victims_wrong


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IO_URING not in backends:
        print("    [skip] iouring_faulty_hdd_many is io_uring-only")
        return True
    with K.FuseMount("faultymany", UC_FUSE_FAULT="BADSECTOR:0:1000000000") as m:
        if m is None:
            print("    [skip] libfuse3/fusermount3 unavailable or FUSE mount failed")
            return True
        for sub in ("goods", "mixed"):
            (m.back / sub).mkdir()
            for i in range(2):   # sort first; fill all 8 buffers so a later quarantine leaks RESIDENT pages
                (m.back / sub / f"a_big_{i}.bin").write_bytes(synthtree._seeded_bytes_fast(150 + i, BAD_SIZE))
            for i in range(N_GOOD):
                (m.back / sub / f"good_{i:02d}.bin").write_bytes(synthtree._seeded_bytes_fast(200 + i, 64 * 1024 * (1 + i % 3) + i))
        for i in range(N_BAD):
            (m.back / "mixed" / f"dead_{i:02d}_BADSECTOR.bin").write_bytes(synthtree._seeded_bytes_fast(300 + i, BAD_SIZE))
        rA, tA, badA, _ = _one(m.mnt, m.back, "goods", "goods")
        rB, tB, badB, wrongB = _one(m.mnt, m.back, "mixed", "mixed")
    leak = rB.peak_rss_mb - rA.peak_rss_mb
    stall = max(0.0, tB - tA) / N_BAD
    ok = (rA.ok and rB.completed and rB.stayed_alive and not badA and not badB and not wrongB
          and not rB.oom_killed and rB.mem_errors == 0 and leak < LEAK_LIMIT_MB
          and stall < STALL_LIMIT_S_PER_FILE)
    print(f"    [io_uring many bad sectors] goods-only: ok={rA.ok} rss={rA.peak_rss_mb}MB {tA:.0f}s | "
          f"+{N_BAD} dead: completed={rB.completed} alive={rB.stayed_alive} goods_missing={badB[:3]} "
          f"victims_wrong={wrongB[:3]} oom={rB.oom_killed} rss={rB.peak_rss_mb}MB {tB:.0f}s | "
          f"leak={leak}MB (limit {LEAK_LIMIT_MB}) stall={stall:.2f}s/file (limit {STALL_LIMIT_S_PER_FILE}) "
          f"-> {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
