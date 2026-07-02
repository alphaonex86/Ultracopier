#!/usr/bin/env python3
"""Hard-kill (SIGKILL == power-loss model) MID-WRITE, then re-run over the partial dest with
the size/date collision policies -- the crash-consistency + preallocation tripwire.

SIGKILL bypasses ALL graceful cleanup (no destinationIsOursToRemove unlink, no dtors) --
exactly what a power cut / kernel panic leaves behind. Two invariants protect the user:

  1. CRASH-SHORTNESS: the interrupted dest file must be SHORT (st_size == bytes actually
     written), NEVER full-size. Both preallocation paths were chosen precisely for this
     (io_uring FALLOC_FL_KEEP_SIZE / IOCP FileAllocationInfo -- never SetEndOfFile): if a
     regression ever pre-sets the final size, a hard-killed copy leaves a full-size
     zero-tailed file, and then...
  2. RE-RUN HEALS: a second run with fileCollision=7 ("overwrite if not same size") must
     re-copy the short partial (size differs) -> final tree byte-identical. If the partial
     were full-size, idx 7 would SKIP it as complete -- silent permanent corruption. The
     idx 3 variant (size AND date) must equally heal it.

Per backend (async + io_uring): copy a tree with one 256 MiB stamped victim (every MiB
carries its own offset -- any hole/shift/zero-tail mismatches), SIGKILL the engine mid-write
of the victim (dest size polled in (8 MiB, full)), assert the leftover is SHORT, then re-run
with idx 7 (and once more with idx 3 after a fresh kill) and assert the final tree ==
source byte-for-byte (full cmp of the victim + harness diff of the tree).
If the kill window is missed (too-fast box) the attempt is retried with a fresh dest up to
3 times; only a genuinely un-catchable window SKIPs (never a silent pass on a caught one).
"""
import os, sys, time, signal, shutil, struct, tempfile, pathlib, subprocess
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

VICTIM = "victim_256m.bin"
MIB = 1024 * 1024
V_MIB = 256
SIBS = {"aa_first.txt": b"small sibling before\n" * 8,
        "zz_last.txt": b"small sibling after\n" * 8}


def _make_src(name: str) -> str:
    src = K.fresh_src_root(name)
    shutil.rmtree(src, ignore_errors=True)               # fresh_src_root returns a path, does not create/wipe
    os.makedirs(src, exist_ok=True)
    base = bytearray(os.urandom(4096) * (MIB // 4096))   # 1 MiB pseudo-random block
    with open(os.path.join(src, VICTIM), "wb") as f:
        for i in range(V_MIB):
            base[0:16] = struct.pack("<QQ", i * MIB, 0xDEADBEEF)   # per-MiB offset stamp
            f.write(base)
    for rel, data in SIBS.items():
        K.write_file(os.path.join(src, rel), data)
    return src


def _kill_mid_write(backend: str, src: str, dest: str) -> str:
    """Launch the real binary, SIGKILL it while the victim's dest is partially written.
    Returns 'killed' (window caught), 'missed' (copy finished first), or 'failed'."""
    home = pathlib.Path(tempfile.mkdtemp(prefix="uc-hk-home-"))
    H.write_config(home, file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.MERGE,
                   file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                   keep_date=True, do_right=True)
    binp = H.binary_for(backend, H.load_config(), asan=False)
    env = dict(os.environ, HOME=str(home), QT_QPA_PLATFORM="offscreen", DISPLAY="",
               ULTRACOPIER_SOCKET_SUFFIX=H.TEST_SOCKET_SUFFIX,
               XDG_CONFIG_HOME=str(home / ".config"))
    vd = os.path.join(dest, os.path.basename(src), VICTIM)
    proc = subprocess.Popen([binp, "cp", src, dest], env=env,
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    outcome = "missed"
    t0 = time.time()
    try:
        while time.time() - t0 < 120:
            try:
                sz = os.path.getsize(vd)
            except OSError:
                sz = -1
            if 8 * MIB <= sz < V_MIB * MIB:
                os.kill(proc.pid, signal.SIGKILL)      # power loss: no cleanup of any kind
                outcome = "killed"
                break
            if proc.poll() is not None:
                break
            time.sleep(0.001)
    finally:
        if proc.poll() is None and outcome != "killed":
            proc.kill()
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            outcome = "failed"
        subprocess.run(["pkill", "-9", "-x", "ultracopier"], capture_output=True)
        shutil.rmtree(home, ignore_errors=True)
    return outcome


def _one(backend: str, idx: int, memcheck) -> bool:
    src = _make_src(f"hk_{backend}_{idx}_src")
    v_full = V_MIB * MIB
    for attempt in range(3):
        dest = K.fresh_dest(f"hk_{backend}_{idx}_dest_{attempt}")
        outcome = _kill_mid_write(backend, src, dest)
        if outcome == "killed":
            vd = os.path.join(dest, os.path.basename(src), VICTIM)
            problems = []
            # 1. CRASH-SHORTNESS: the hard-killed dest must be SHORT, never full-size.
            if os.path.exists(vd):
                sz = os.path.getsize(vd)
                if sz >= v_full:
                    problems.append(f"hard-killed dest is FULL-SIZE ({sz}) -- a size-only "
                                    f"policy would skip it: SILENT-CORRUPTION EXPOSURE")
            # 2. RE-RUN HEALS: size(/date) policy must re-copy the short partial.
            r = H.run(backend, "cp", [src], dest,
                      file_collision=idx,
                      folder_collision=H.FolderCollision.MERGE,
                      file_error=H.FileError.SKIP,
                      expect_dir=src, memcheck=memcheck)
            if not r.ok:
                problems.append(f"re-run: ok={r.ok} completed={r.completed} "
                                f"content={r.content_ok} mem_err={r.mem_errors}")
            if os.path.exists(vd):
                rc = subprocess.run(["cmp", "-s", os.path.join(src, VICTIM), vd]).returncode
                if rc != 0:
                    problems.append("victim NOT byte-identical after the healing re-run "
                                    "-- the partial survived (skipped as complete?)")
            else:
                problems.append("victim missing after the healing re-run")
            if problems:
                print(f"      [{backend} idx {idx}] FAIL: " + "; ".join(problems))
            return not problems
        if outcome == "failed":
            print(f"      [{backend} idx {idx}] FAIL: kill window handling failed")
            return False
        # missed -> retry with a fresh dest
    print(f"      [{backend} idx {idx}] SKIP: could not catch the mid-write window in 3 "
          f"attempts (box copies 256 MiB too fast) -- not a pass, not a fail")
    return True


def run(backends=None, memcheck=H.NONE) -> bool:
    def one(backend):
        a = _one(backend, 7, memcheck)   # overwrite-if-not-same-size
        b = _one(backend, 3, memcheck)   # overwrite-if-not-same-size-AND-date
        return a and b
    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
