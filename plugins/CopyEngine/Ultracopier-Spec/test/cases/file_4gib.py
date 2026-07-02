#!/usr/bin/env python3
"""Single file ACROSS the 4 GiB boundary: the 32-bit offset/size-wrap class.

The suite's largest file elsewhere is 192 MiB -- nothing ever crosses 2^32. A missed
OffsetHigh in an IOCP OVERLAPPED, an int/long truncation in offset math, a size stored in
32 bits: all wrap at exactly 4 GiB, writing block N onto offset N mod 4GiB -- silent
corruption (or a never-terminating length loop) on every file bigger than 4 GiB. Movies and
disk images are exactly the files Ultracopier is pointed at.

Source: ONE sparse ~4.5 GiB file (unaligned tail: +12345 bytes) carrying 16-byte offset
stamps every 64 MiB plus a dense cluster around the 4 GiB boundary (4GiB-8/4GiB/4GiB+8): a
wrapped write lands a late stamp onto an early offset, so ANY aliasing mismatches. Sparse
creation is instant; the copy itself streams real bytes.
ASSERTS per Linux backend: job completes under the harness (memory cap included -- the file
must STREAM, never be held in RAM); dest size exactly equal; dest byte-identical (full cmp).
SKIPs (capability gate, loudly) when the scratch fs lacks ~10 GiB free.
(IOCP: the same class matters most on Windows -- OVERLAPPED Offset/OffsetHigh -- but staging
4.5 GiB over the test SSH lane is impractical; the on-box variant belongs to the manual
Windows bench flow. This case pins the shared+io_uring offset math.)
"""
import os, sys, struct, shutil, subprocess, pathlib
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

GIB = 1024 ** 3
MIB = 1024 ** 2
# Just PAST 4 GiB (minimal boundary cross keeps the copy+cmp I/O bounded): stamps land on both
# sides of 2^32 so any 32-bit offset wrap still aliases. On 64-bit Linux the wrap risk is low
# (io_uring __u64 offsets, async off_t with _FILE_OFFSET_BITS=64) -- this is a boundary smoke
# test whose PRIMARY target is IOCP's 32-bit OVERLAPPED Offset/OffsetHigh (deferred: staging
# 4 GiB over the SSH lane is impractical; belongs to the manual Windows bench flow).
SIZE = 4 * GIB + 8 * MIB + 12345
VICTIM = "over4gib.bin"


def _stamp_offsets():
    offs = list(range(0, SIZE - 16, 64 * MIB))
    offs += [4 * GIB - 8, 4 * GIB, 4 * GIB + 8, SIZE - 16]   # the wrap hot-zone + tail
    return sorted(set(o for o in offs if 0 <= o <= SIZE - 16))


def _make_sparse(path: str):
    with open(path, "wb") as f:
        for off in _stamp_offsets():
            f.seek(off)
            f.write(struct.pack("<QQ", off, off ^ 0xA5A5A5A5A5A5A5A5))
        f.truncate(SIZE)


def run(backends=None, memcheck=H.NONE) -> bool:
    # Capability gate: the dest needs ~4.5 GiB (dest is REAL bytes even for a sparse source
    # unless the fs punches holes) + the source's sparse blocks. Require 10 GiB headroom.
    scratch = K._scratch_root()
    st = os.statvfs(scratch)
    free = st.f_bavail * st.f_frsize
    if free < 10 * GIB:
        print(f"    [file_4gib] SKIPPED: only {free // GIB} GiB free on the scratch fs "
              f"(needs ~10 GiB) -- capability gate, not a pass on the content")
        return True

    def one(backend):
        src = K.fresh_src_root(f"g4_{backend}_src")
        shutil.rmtree(src, ignore_errors=True)           # fresh_src_root returns a path, does not create/wipe
        os.makedirs(src, exist_ok=True)
        sp = os.path.join(src, VICTIM)
        _make_sparse(sp)
        dest = K.fresh_dest("g4_dest")
        try:
            r = H.run(backend, "cp", [src], dest,
                      file_collision=H.FileCollision.OVERWRITE,
                      folder_collision=H.FolderCollision.MERGE,
                      expect_dir=None, memcheck=memcheck)   # cmp below is the content gate
            dp = os.path.join(dest, os.path.basename(src), VICTIM)
            problems = []
            if not (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0):
                problems.append(f"run: completed={r.completed} alive={r.stayed_alive} "
                                f"oom={r.oom_killed} mem_err={r.mem_errors}")
            if not os.path.exists(dp):
                problems.append("dest missing")
            else:
                ds = os.path.getsize(dp)
                if ds != SIZE:
                    problems.append(f"dest size {ds} != {SIZE} "
                                    + ("(== SIZE mod 4GiB? 32-bit length wrap)" if ds == SIZE % (4 * GIB) else ""))
                elif subprocess.run(["cmp", "-s", sp, dp]).returncode != 0:
                    problems.append("dest NOT byte-identical -- offset aliasing across 4 GiB")
            if problems:
                print(f"      [{backend}] FAIL: " + "; ".join(problems))
            return not problems
        finally:
            # 9 GiB of scratch per backend would accumulate -- clean both trees right away.
            shutil.rmtree(src, ignore_errors=True)
            shutil.rmtree(dest, ignore_errors=True)

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
