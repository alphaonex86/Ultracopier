#!/usr/bin/env python3
"""REAL destination-full (ENOSPC) resilience on the io_uring backend.

The LD_PRELOAD shim cannot fault io_uring's data plane (it goes through io_uring_enter(2), not
libc -- see cases/write_faults.py and memory project_iouring_not_libc_faultable). So to fault the
io_uring WRITE path for real we copy into a genuinely size-capped filesystem: an unprivileged
user+mount namespace (`unshare -Umr`) with a small `tmpfs` mounted at the destination. io_uring's
writes then hit a REAL ENOSPC partway through the copy.

Source tree (~3.4 MB) into a 1.5 MB tmpfs, fileError=Skip. REQUIRED resilient behaviour:
  * the engine must NOT crash (a tray app never exits 0 on its own; a non-zero exit = crash),
  * every file that is present at FULL size must be byte-identical (zero corruption of a file
    the engine treats as complete),
  * at least the files that fit must be copied correctly (resilient partial backup),
  * a file that could not fit is absent or truncated -- never a full-size wrong-content file.

Environment-gated: if the kernel denies unprivileged user namespaces (`unshare -Umr`) this cannot
run and is reported as SKIPPED (capability gate, like the Windows-only cases on Linux) -- NOT a
silent pass that hides a failure. Where the capability exists (CI image permitting), it runs.
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


# Runs INSIDE the user+mount namespace: mount a small tmpfs at the dest, run the copy, verify the
# copied subset, report a one-line RESULT and exit 0 (resilient+correct) / 1 (crash or corruption).
_WRAPPER = r'''
import os, sys, time, subprocess
binp, src, dest, home, size = sys.argv[1:6]
subprocess.run(["mount","-t","tmpfs","-o","size="+size,"tmpfs",dest], check=True)
env = dict(os.environ, HOME=home, QT_QPA_PLATFORM="offscreen", DISPLAY="",
           XDG_CONFIG_HOME=os.path.join(home,".config"))
def dbytes():
    t=0
    for dp,_,fs in os.walk(dest):
        for f in fs:
            try: t+=os.path.getsize(os.path.join(dp,f))
            except OSError: pass
    return t
p=subprocess.Popen([binp,"cp",src,dest],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
last=-1; stable=0; crashed=False
for _ in range(60):
    time.sleep(1)
    rc=p.poll()
    if rc is not None and rc!=0:
        crashed=True; break
    cur=dbytes()
    stable = stable+1 if cur==last else 0
    last=cur
    if stable>=6: break
p.kill()
copied=os.path.join(dest, os.path.basename(src))
full_wrong=[]; full_ok=0; truncated=0; missing=0
for dp,_,fs in os.walk(src):
    for f in fs:
        s=os.path.join(dp,f); rel=os.path.relpath(s,src); d=os.path.join(copied,rel)
        try: ds=os.path.getsize(d)
        except OSError: missing+=1; continue
        ss=os.path.getsize(s)
        if ds==ss:
            if subprocess.run(["cmp","-s",s,d]).returncode==0: full_ok+=1
            else: full_wrong.append(rel)
        else: truncated+=1
print("RESULT crashed=%s full_ok=%d truncated=%d missing=%d corrupt_fullsize=%d dest_bytes=%d"
      % (crashed, full_ok, truncated, missing, len(full_wrong), dbytes()))
if full_wrong: print("CORRUPT:"+",".join(full_wrong[:8]))
subprocess.run(["umount",dest],stderr=subprocess.DEVNULL)
# Resilient+correct iff: no crash, ALL 3 small files fully copied (positive: the copy made real
# progress before ENOSPC, not just an early bail), and ZERO full-size-but-wrong-content files.
sys.exit(0 if (not crashed and full_ok>=3 and not full_wrong) else 1)
'''


def _userns_available() -> bool:
    """True iff this kernel lets us create an unprivileged user+mount namespace AND mount tmpfs."""
    try:
        r = subprocess.run(
            ["unshare", "-Umr", "bash", "-c",
             'd=$(mktemp -d); mount -t tmpfs -o size=1m tmpfs "$d" && umount "$d" && rmdir "$d"'],
            capture_output=True, timeout=20)
        return r.returncode == 0
    except (OSError, subprocess.TimeoutExpired):
        return False


def run(backends=None, memcheck=H.NONE) -> bool:
    if not _userns_available():
        print("    [iouring_dest_full] SKIPPED: unprivileged user namespaces / tmpfs mount "
              "not permitted on this kernel (capability gate, not a failure)")
        return True

    binp = H.binary_for(H.IO_URING, H.load_config(), asan=False)

    src = pathlib.Path(K.fresh_src_root("ns_src"))
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    # 3 small files (fit) + one 3 MiB file (will hit ENOSPC) -> total ~3.4 MiB into a 1.5 MiB tmpfs.
    for n, sz in (("a.dat", 100000), ("b.dat", 120000), ("c.dat", 90000)):
        (src / n).write_bytes(bytes((i * 13) & 0xFF for i in range(sz)))
    (src / "big.dat").write_bytes(bytes((i * 7) & 0xFF for i in range(3 * 1024 * 1024)))

    dest = K.fresh_dest("ns_dest")
    pathlib.Path(dest).mkdir(parents=True, exist_ok=True)
    home = pathlib.Path(tempfile.mkdtemp(prefix="uc-ns-home-"))
    H.write_config(home, file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.MERGE,
                   file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP)

    wrapper = pathlib.Path(tempfile.mkstemp(prefix="uc-ns-wrap-", suffix=".py")[1])
    wrapper.write_text(_WRAPPER)
    try:
        r = subprocess.run(
            ["unshare", "-Umr", "python3", str(wrapper),
             binp, str(src), dest, str(home), "1500k"],
            capture_output=True, text=True, timeout=180)
    finally:
        wrapper.unlink(missing_ok=True)
        subprocess.run(["pkill", "-9", "-x", "ultracopier"], capture_output=True)
        shutil.rmtree(home, ignore_errors=True)

    for line in (r.stdout + r.stderr).splitlines():
        if line.strip():
            print(f"      {line.strip()}")
    ok = (r.returncode == 0)
    print(f"    [iouring_dest_full] {'PASS' if ok else 'FAIL'}  "
          f"(real ENOSPC on io_uring: no crash, complete files byte-correct, no corruption)")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
