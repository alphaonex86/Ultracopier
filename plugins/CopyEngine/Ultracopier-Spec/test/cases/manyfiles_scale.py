#!/usr/bin/env python3
"""Many-SMALL-files scale gate: 60k tiny files under the COMPUTED memory cap + an fd/handle
ceiling -- the per-file-cost regression tripwire.

memory.py covers BYTE volume (~400 MB in ~50 chunky files, FIXED 1024 MB cap) but per-FILE
memory cost is ungated: the documented #1 scalability blocker OOMed at ~904 MB on a 70k-file
tree, and a 1 KB/file leak is invisible at 50 files yet 60 MB here and 50 GB at the 50M-file
target. This case copies 60k tiny files (nested dirs + one 20k-entry FLAT dir -- directory-
width stress) under the harness's COMPUTED cap (mem_limit_mb=None: scales with #files, far
below the old 904-MB behaviour -> a per-file-struct regression trips the cgroup).

A sampler thread polls /proc/<pid>/fd (nothing else in the suite observes fd counts): the
io_uring fire-and-forget close and the per-thread rings keep a bounded working set -- a leak
of one fd per file would hit 60k. ASSERT peak fds < 2048 (default ulimit 1024 would fail
mid-copy anyway; the margin covers rings+eventfds+Qt).

ASSERTS: completes + alive + under the computed cap + memcheck clean; file COUNT at dest ==
60k and spot-hash of a deterministic 1% sample byte-identical (a full 60k-file diff walk is
minutes of pure test overhead; count + spot-hash catches forgotten/corrupt files); peak fd
bounded. Slow-tail case (excluded from --quick automatically by its recorded time).
"""
import os, sys, threading, time, hashlib, pathlib
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

N_NESTED = 40000          # in nested dirs, ~100/dir
N_FLAT = 20000            # one flat dir (directory-width stress)
TOTAL = N_NESTED + N_FLAT
FD_CEILING = 2048


def _content(i: int) -> bytes:
    # tiny, unique, deterministic -- hashable without storing anything per-file
    return (b"f%08d:" % i) + hashlib.sha256(str(i).encode()).hexdigest().encode() + b"\n"


def _make_src(name: str) -> str:
    src = K.fresh_src_root(name)
    for i in range(N_NESTED):
        d = os.path.join(src, "n%03d" % (i // 100))
        if i % 100 == 0:
            os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, "f%08d.txt" % i), "wb") as f:
            f.write(_content(i))
    flat = os.path.join(src, "flat")
    os.makedirs(flat, exist_ok=True)
    for i in range(N_NESTED, TOTAL):
        with open(os.path.join(flat, "f%08d.txt" % i), "wb") as f:
            f.write(_content(i))
    return src


def run(backends=None, memcheck=H.NONE) -> bool:
    src = _make_src("many_src")

    def one(backend):
        dest = K.fresh_dest(f"many_{backend}_dest")
        peak_fds = [0]
        stop = threading.Event()

        def sampler():
            while not stop.is_set():
                for pid in H._test_ultracopier_pids():
                    try:
                        n = len(os.listdir(f"/proc/{pid}/fd"))
                        if n > peak_fds[0]:
                            peak_fds[0] = n
                    except OSError:
                        pass
                time.sleep(0.5)

        th = threading.Thread(target=sampler, daemon=True)
        th.start()
        try:
            r = H.run(backend, "cp", [src], dest,
                      file_collision=H.FileCollision.OVERWRITE,
                      folder_collision=H.FolderCollision.MERGE,
                      expect_dir=None, memcheck=memcheck)   # count+spot-hash below
        finally:
            stop.set()
            th.join(timeout=3)

        copied = os.path.join(dest, os.path.basename(src))
        problems = []
        if not (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0):
            problems.append(f"run: completed={r.completed} alive={r.stayed_alive} "
                            f"oom={r.oom_killed} (per-file memory cost?) mem_err={r.mem_errors}")
        # COUNT: every file arrived (no forgotten file).
        got = sum(len(fs) for _dp, _dn, fs in os.walk(copied))
        if got != TOTAL:
            problems.append(f"dest file count {got} != {TOTAL}")
        # SPOT-HASH: deterministic 1% sample byte-identical.
        for i in range(0, TOTAL, 100):
            rel = (os.path.join("n%03d" % (i // 100), "f%08d.txt" % i) if i < N_NESTED
                   else os.path.join("flat", "f%08d.txt" % i))
            p = os.path.join(copied, rel)
            if not os.path.exists(p) or K.read_file(p) != _content(i):
                problems.append(f"sample {rel} missing/corrupt")
                break
        if peak_fds[0] >= FD_CEILING:
            problems.append(f"peak open fds {peak_fds[0]} >= {FD_CEILING} -- fd leak at scale")
        print(f"      [{backend}] files={got}/{TOTAL} peak_rss={r.peak_rss_mb}MB "
              f"peak_fds={peak_fds[0]}")
        if problems:
            print(f"      [{backend}] FAIL: " + "; ".join(problems))
        return not problems

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
