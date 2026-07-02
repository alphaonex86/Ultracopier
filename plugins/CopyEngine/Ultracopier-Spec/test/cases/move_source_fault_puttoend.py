#!/usr/bin/env python3
"""MOVE + an unreadable source file + fileError=PUT_TO_END: the abandoned file's source (and
its folder) must survive, the rest must move -- and the job must END (bounded give-up).

move_folder_partial covers mv + source-read-fault with SKIP on async only. PUT_TO_END is a
DIFFERENT terminal path: the failing file is re-queued and retried, and the final give-up
(finalSkipNoRetry after the retry budget) must set the same per-file failed state -- if the
second pass confuses the source-removal bookkeeping (the entry now sits at a different queue
position), the source of the never-copied file is deleted: data loss on the dying-disk
salvage MOVE, the flagship use case. A put-to-end loop that never gives up = the job never
ends (hang class) -- the no-progress watchdog turns that into a loud fail.

Two fault flavours, one per backend (both are real cross-fs moves, source on /tmp):
  * async   -- shim `readfail:BADSECTOR`: open OK, every READ fails EIO (bad sectors);
  * io_uring -- the victim is chmod 000: OPEN fails EACCES (a real fault, no shim -- reaches
    the ring backend regardless of how it opens).
ASSERTS per backend: job completes + alive + memory clean; victim SOURCE byte-identical and
its parent folder still present; every good file MOVED (dest correct, source gone).
"""
import os, sys, shutil, tempfile, hashlib, pathlib
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

GOOD = {"good_a.dat": bytes((i * 11) & 0xFF for i in range(2 * 1024 * 1024)),  # multi-chunk
        os.path.join("sub", "good_b.txt"): b"good nested sibling\n" * 32}


def _one(backend: str, memcheck) -> bool:
    src_parent = tempfile.mkdtemp(prefix="uc-mvpte-", dir="/tmp")
    src = os.path.join(src_parent, "mvpte_tree")
    victim_rel = os.path.join("baddir", "BADSECTOR_victim.bin")
    v_data = b"unreadable victim -- source must survive put-to-end give-up\n" * 64
    K.write_file(os.path.join(src, victim_rel), v_data)
    for rel, data in GOOD.items():
        K.write_file(os.path.join(src, rel), data)
    dest = K.fresh_dest(f"mvpte_{backend}_dest")
    vp = os.path.join(src, victim_rel)
    try:
        if backend == H.ASYNC:
            K.with_scenario("readfail:BADSECTOR")     # read-path fault
            preload = K.fs_so()
        else:
            K.with_scenario("")
            os.chmod(vp, 0)                            # open-path fault (real EACCES)
            preload = None
        r = H.run(backend, "mv", [src], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.PUT_TO_END,
                  folder_error=H.FolderError.SKIP,
                  expect_dir=None, fs_preload=preload, memcheck=memcheck)
        K.with_scenario("")
        problems = []
        if not (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0):
            problems.append(f"run: completed={r.completed} alive={r.stayed_alive} "
                            f"oom={r.oom_killed} mem_err={r.mem_errors}"
                            + ("  <-- put-to-end never gave up (hang)" if not r.completed else ""))
        if not os.path.exists(vp):
            problems.append("victim SOURCE deleted after put-to-end give-up -- DATA LOSS")
        else:
            os.chmod(vp, 0o644)
            if K.read_file(vp) != v_data:
                problems.append("victim SOURCE corrupted")
        if not os.path.isdir(os.path.dirname(vp)):
            problems.append("victim's source FOLDER removed while non-empty")
        copied = os.path.join(dest, os.path.basename(src))
        for rel, data in GOOD.items():
            dp = os.path.join(copied, rel)
            if not os.path.exists(dp) or hashlib.sha256(K.read_file(dp)).digest() != hashlib.sha256(data).digest():
                problems.append(f"good {rel} not moved correctly")
            if os.path.exists(os.path.join(src, rel)):
                problems.append(f"good {rel} source not removed")
        if problems:
            print(f"      [{backend}] FAIL: " + "; ".join(problems))
        return not problems
    finally:
        try:
            os.chmod(vp, 0o644)
        except OSError:
            pass
        shutil.rmtree(src_parent, ignore_errors=True)


def run(backends=None, memcheck=H.NONE) -> bool:
    def one(backend):
        return _one(backend, memcheck)
    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
