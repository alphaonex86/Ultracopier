#!/usr/bin/env python3
"""REAL io_uring DATA-plane WRITE-CORRUPTION + checksum-verify, via a writable FUSE.

The LD_PRELOAD shim can't fault io_uring's transferred bytes (io_uring_enter(2) bypasses libc). So we
mount a *writable passthrough* FUSE (`fsoverride/linux/fuse_fault.c`, `UC_FUSE_WFAULT=<substr>:<off>`)
as the DESTINATION: a write to the victim that covers byte <off> is SILENTLY bit-flipped on its way to
the backing (the write still returns the full count, so the engine believes it succeeded), and a re-read
returns the corrupt byte. io_uring's ring writes/reads are serviced by this daemon regardless, so the
fault reaches the ring data plane for real.

We run a 1-thread io_uring copy (see CLAUDE.md "force FEW inode threads": io_uring's concurrency is from
async submission, not CPU; through FUSE the round-trip latency dominates) into the FUSE dest, TWICE:

  * checksum=true  + checksumOnlyOnError=false -> the engine MUST re-read the destination to verify it
    (xxh3-64), so it DETECTS the silent corruption. Proven objectively by the FUSE read-log: the corrupt
    victim's dest bytes are re-read (>= its size).
  * checksum=false -> NO verify re-read of the dest (read-log == 0): the divergence proves the verify is
    what does the work, and that without it io_uring would deliver the corrupt bytes undetected.

REQUIRED (both runs): the process does not crash, every readable SIBLING is byte-perfect (one file's
corruption must not break others), AND the verify-re-read divergence holds (ON re-reads the dest, OFF
does not). With checksum ON, io_uring DETECTS the corruption (re-read mismatch) AND REMOVES the corrupt
destination (the #25 cleanup, ported to the pipelined via the #9 destinationPreExisted guard, captured in
preOperation on the first leg only); with OFF it is not verified and the corrupt victim is LEFT.

Environment-gated: needs an unprivileged user+mount namespace AND FUSE (fuse3 + /dev/fuse + fusermount3).
Where unavailable it is reported SKIPPED (capability gate, like iouring_dest_full / the Windows lane).
"""
import os
import sys
import shutil
import tempfile
import pathlib
import subprocess

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

VICTIM = "victim_multi.dat"
VSIZE = 1024 * 1024
OFF = 524288   # mid-file: a true multi-chunk write covers it

# Runs INSIDE the user+mount namespace: mount the writable-corruption FUSE at the dest, run the io_uring
# copy, then report the victim's fate + the dest re-read byte count. Exit 0 always (the case asserts).
_WRAPPER = r'''
import os, sys, time, subprocess
binp, src, backing, mnt, home, fuse, victim, off, rlog = sys.argv[1:10]
fp = subprocess.Popen([fuse, mnt, "-f", "-s"],
                      env=dict(os.environ, UC_FUSE_BACKING=backing, UC_FUSE_WFAULT=victim+":"+off,
                               UC_FUSE_READLOG_MATCH=victim, UC_FUSE_READLOG_PATH=rlog),
                      stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(1.5)
env = dict(os.environ, HOME=home, QT_QPA_PLATFORM="offscreen", DISPLAY="",
           XDG_CONFIG_HOME=os.path.join(home, ".config"))
def db():
    t = 0
    for dp, _, fs in os.walk(mnt):
        for f in fs:
            try: t += os.path.getsize(os.path.join(dp, f))
            except OSError: pass
    return t
p = subprocess.Popen([binp, "cp", src, mnt], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
last = -1; stable = 0; crashed = False
for _ in range(90):
    time.sleep(1)
    rc = p.poll()
    if rc is not None and rc != 0: crashed = True; break
    cur = db(); stable = stable + 1 if cur == last else 0; last = cur
    if stable >= 6: break
time.sleep(1); p.kill(); time.sleep(0.5)
base = os.path.basename(src); cp = os.path.join(backing, base)
def good_ok():
    d = os.path.join(cp, "good.dat"); s = os.path.join(src, "good.dat")
    if not os.path.exists(d): return False
    return subprocess.run(["cmp", "-s", s, d]).returncode == 0
try: reread = int(open(rlog).read().strip() or "0")
except Exception: reread = 0
victim_present = os.path.exists(os.path.join(cp, "victim_multi.dat"))
print("RESULT crashed=%s good_ok=%s dest_reread=%d victim_present=%s" % (crashed, good_ok(), reread, victim_present))
subprocess.run(["fusermount3", "-u", mnt], stderr=subprocess.DEVNULL)
'''


def _userns_fuse_available() -> bool:
    if not pathlib.Path("/dev/fuse").exists() or not shutil.which("fusermount3"):
        return False
    try:
        r = subprocess.run(
            ["unshare", "-Umr", "bash", "-c",
             'd=$(mktemp -d); mount -t tmpfs -o size=1m tmpfs "$d" && umount "$d" && rmdir "$d"'],
            capture_output=True, timeout=20)
        return r.returncode == 0
    except (OSError, subprocess.TimeoutExpired):
        return False


def _run_once(binp: str, fuse: str, checksum: bool):
    src = pathlib.Path(tempfile.mkdtemp(prefix="wc-src-")) / "src"
    src.mkdir(parents=True)
    (src / VICTIM).write_bytes(bytes((i * 7) & 0xFF for i in range(VSIZE)))
    (src / "good.dat").write_bytes(b"good-sibling-content\n" * 30000)
    backing = tempfile.mkdtemp(prefix="wc-bk-")
    mnt = tempfile.mkdtemp(prefix="wc-mnt-")
    home = pathlib.Path(tempfile.mkdtemp(prefix="wc-home-"))
    rlog = tempfile.mkstemp(prefix="wc-rlog-")[1]
    H.write_config(home, file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.MERGE,
                   file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                   inode_threads=1,
                   extra_options={"checksum": ("true" if checksum else "false"),
                                  "checksumOnlyOnError": "false"})
    wf = pathlib.Path(tempfile.mkstemp(prefix="wc-wrap-", suffix=".py")[1])
    wf.write_text(_WRAPPER)
    try:
        r = subprocess.run(
            ["unshare", "-Umr", "python3", str(wf),
             binp, str(src), backing, mnt, str(home), fuse, "victim_multi", str(OFF), rlog],
            capture_output=True, text=True, timeout=180)
    finally:
        wf.unlink(missing_ok=True)
        subprocess.run(["pkill", "-9", "-x", "ultracopier"], capture_output=True)
        shutil.rmtree(home, ignore_errors=True)
    crashed, good, reread, victim_present = True, False, 0, True
    for line in (r.stdout + r.stderr).splitlines():
        if line.startswith("RESULT"):
            for tok in line.split()[1:]:
                k, v = tok.split("=")
                if k == "crashed": crashed = (v == "True")
                elif k == "good_ok": good = (v == "True")
                elif k == "dest_reread": reread = int(v)
                elif k == "victim_present": victim_present = (v == "True")
    return crashed, good, reread, victim_present


def run(backends=None, memcheck=H.NONE) -> bool:
    if not _userns_fuse_available():
        print("    [iouring_write_corruption] SKIPPED: unprivileged userns + FUSE (fuse3/"
              "/dev/fuse/fusermount3) not available (capability gate, not a failure)")
        return True
    fuse = str(pathlib.Path(__file__).resolve().parents[1] / "fsoverride" / "build" / "fuse_fault")
    if not os.path.exists(fuse):
        subprocess.run(["make", "fuse_fault"],
                       cwd=str(pathlib.Path(__file__).resolve().parents[1] / "fsoverride"),
                       capture_output=True)
    if not os.path.exists(fuse):
        print("    [iouring_write_corruption] SKIPPED: fuse_fault not built (no fuse3 dev headers)")
        return True
    binp = H.binary_for(H.IO_URING, H.load_config(), asan=False)

    on_crash, on_good, on_reread, on_victim = _run_once(binp, fuse, checksum=True)
    off_crash, off_good, off_reread, off_victim = _run_once(binp, fuse, checksum=False)

    # checksum ON re-reads the corrupt dest (>= its size), detects the mismatch, AND REMOVES the corrupt
    # destination (the #25 cleanup, now ported to the pipelined via the #9 destinationPreExisted guard).
    # checksum OFF: no verify re-read -> the corrupt victim is LEFT (the divergence proves the verify is
    # what catches it). The good sibling must survive both; neither run may crash.
    verify_ran = on_reread >= VSIZE
    no_verify_off = off_reread == 0
    on_removed = not on_victim       # ON: corrupt dest detected + removed (not delivered)
    off_left = off_victim            # OFF: corrupt dest left (undetected without checksum)
    ok = (not on_crash and not off_crash and on_good and off_good
          and verify_ran and no_verify_off and on_removed and off_left)
    print(f"      checksum ON : crash={on_crash} good_ok={on_good} dest_reread={on_reread} "
          f"victim_present={on_victim} (verify_ran={verify_ran} corrupt_removed={on_removed})")
    print(f"      checksum OFF: crash={off_crash} good_ok={off_good} dest_reread={off_reread} "
          f"victim_present={off_victim} (no_verify={no_verify_off} corrupt_left={off_left})")
    if not ok:
        print("        FAIL: io_uring write-corruption detect+cleanup (#25) not as expected")
    print(f"    [iouring_write_corruption] {'PASS' if ok else 'FAIL'}  "
          f"(REAL io_uring write-corruption: checksum-verify re-reads, detects, and REMOVES the corrupt dest)")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
