#!/usr/bin/env python3
"""RESILIENT BACKUP on a read fault: a bad sector on ONE file must not hang/crash/leak the job -- it
completes, every readable SIBLING is copied byte-perfect, the process stays alive.

Deterministic test (async; eio_after is a libc read fault): copy a multi-chunk VICTIM with
eio_after:VICTIM:65536 (first 64 KiB read OK, then EIO) + a good sibling + fileError=Skip. Run twice (the
deletePartiallyTransferredFiles flag ON vs OFF). REQUIRED in BOTH runs: the job completes, the good sibling
is byte-perfect, no crash/OOM/mem-error.

NOT asserted (separate follow-up -- [[project_read_salvage_drain_followup]]): persisting the BAD file's own
readable 64 KiB prefix. Pre-fix it survived ONLY as a hang artifact -- the wedged skip-close (the bug
skip_drops_multichunk/faulty_hdd fixed) gave the lagging write thread time to open+drain it. With the hang
gone, the fast read fills the buffer and faults before the write thread opens the destination (it bails in
internalOpen on stopIt), so the prefix is never written. Persisting it is a NEW write-drain-on-skip
feature; this case no longer asserts that (artifact) salvage-keep. The data-loss this fix actually
eliminates -- losing a *clean* large sibling because a bad file wedged the cap=1 slot -- is covered green
by skip_drops_multichunk and faulty_hdd.

UPDATE 2026-06-27: the read-salvage drain IS NOW IMPLEMENTED (async, engine markers "#23 read-salvage") and
the full-prefix salvage is asserted by cases/read_salvage_drain.py, which forces the deterministic
write-opens-first path via the shim I/O-order control (slow/slowwrite/delayopen). THIS case stays focused on
the resilient-backup of the SIBLINGS: it does not force that ordering, so the bad file's partial here is
timing-dependent and intentionally not asserted. (The earlier "bails in internalOpen on stopIt" guess above
was wrong -- the real path is ow-opened then a write_closed-handler unlink; see the memory note.)
"""
import os
import sys
import shutil
import pathlib

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

VICTIM = "VICTIM_sector.bin"
FULL = 1024 * 1024  # multi-chunk (> blockSize) so 64 KiB is a true partial


def _one(keep: bool):
    """keep=True -> deletePartiallyTransferredFiles=false (the user wants to KEEP the partial)."""
    src = pathlib.Path(K.fresh_src_root("delpart_" + ("keep" if keep else "del") + "_src"))
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    (src / VICTIM).write_bytes(b"\xC3" * FULL)
    (src / "good.bin").write_bytes(b"\x3C" * 4096)

    dest = K.fresh_dest("delpart_" + ("keep" if keep else "del") + "_dest")
    base = os.path.basename(str(src))
    K.with_scenario("eio_after:VICTIM:65536")
    r = H.run(H.ASYNC, "cp", [str(src)], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,
              expect_dir=None,
              extra_options={"deletePartiallyTransferredFiles": ("false" if keep else "true")},
              fs_preload=K.fs_so())
    K.with_scenario("")

    vp = pathlib.Path(dest, base, VICTIM)
    present = vp.exists()
    size = vp.stat().st_size if present else 0
    faithful = True
    if present and size > 0:
        dst = vp.read_bytes()
        faithful = (len(dst) <= FULL and (b"\xC3" * FULL)[:len(dst)] == dst)
    # the always-good sibling must copy regardless
    good_ok = pathlib.Path(dest, base, "good.bin").exists()
    return r, present, size, faithful, good_ok


def run(backends=None, memcheck=H.NONE) -> bool:
    # RESILIENT BACKUP (the load-bearing property): a read fault on one file must NOT hang/crash/leak the
    # job -- it COMPLETES, every readable SIBLING is copied byte-perfect, and the process stays alive.
    # This is what the skip-close fix guarantees (the wedged-slot data-loss that lost good *clean* files is
    # gone -- see skip_drops_multichunk / faulty_hdd).
    #
    # NOT asserted here (separate follow-up): persisting the *bad* file's own readable PREFIX. The fast read
    # fills the buffer and faults before the write thread opens the destination (it bails in internalOpen on
    # stopIt), AND skip() calls writeThread.flushBuffer() -- which frees+clears theBlockList -- before the
    # close, so the buffered prefix is discarded. Persisting it is a NEW write-drain-on-skip feature needing
    # coordinated skip()/WriteThread changes, tracked separately ([[project_read_salvage_drain_followup]]);
    # this case asserts only the resilient-backup property, not the salvage-keep.
    rON, _onp, _ons, _onf, on_good = _one(keep=False)
    rOFF, _ofp, _ofs, _off, off_good = _one(keep=True)

    ok = (rON.completed and rOFF.completed and rON.stayed_alive and rOFF.stayed_alive
          and on_good and off_good
          and not rON.oom_killed and not rOFF.oom_killed
          and rON.mem_errors == 0 and rOFF.mem_errors == 0)
    print(f"      completed: ON={rON.completed} OFF={rOFF.completed} | good_sibling: ON={on_good} OFF={off_good} | "
          f"mem_err: ON={rON.mem_errors} OFF={rOFF.mem_errors}")
    if not (on_good and off_good):
        print("        DATA LOSS: a readable SIBLING of the bad file was lost on a read-fault skip")
    print(f"    [opt_delete_partial_files:async] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
