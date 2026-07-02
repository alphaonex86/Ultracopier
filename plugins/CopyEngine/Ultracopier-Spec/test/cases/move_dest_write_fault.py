#!/usr/bin/env python3
"""MOVE where the destination WRITE fails MID-FILE (not at open): the source must survive a
PARTIAL destination.

move_destination_error covers the dest OPEN failing (pre-existing 0444 file -> EACCES, zero
bytes flow). This case covers the nastier variant: the write starts fine and dies partway --
ENOSPC, dying target disk -- leaving a PARTIAL dest file that EXISTS. The historical
data-loss bug keyed source-removal on `if (exists(destination))`, and a partial dest passes
that test: if any backend's per-file success gate degrades to existence (or size>0), the
user's only copy is deleted while the dest holds garbage.

  * async: shim verb `enospc:<victim>:<bytes>` -- writes succeed to 1 MiB then fail ENOSPC
    (every retry re-fails; the disk never empties). fileError=SKIP, cross-fs mv.
  * io_uring: the userns small-tmpfs rig (iouring_dest_full) but running MODE `mv` -- a REAL
    ENOSPC on the ring data plane mid-move.

ASSERTS: job completes + stays alive; the good sibling files are MOVED (dest correct, their
sources gone -- proving per-file delete still works); the victim's SOURCE survives
byte-identical; the partial dest (if left) is SHORT, never full-size-wrong.
"""
import os, sys, shutil, tempfile, pathlib, subprocess, hashlib
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

VICTIM = "victim_big.bin"
V_SIZE = 3 * 1024 * 1024
SIBS = {"sib_a.dat": b"sibling A moves fine\n" * 32,
        os.path.join("sub", "sib_b.dat"): b"sibling B nested\n" * 16}


def _async_lane(memcheck) -> bool:
    src_dir = tempfile.mkdtemp(prefix="uc-mvwf-src-", dir="/tmp")
    src = os.path.join(src_dir, "mvwf_tree")
    v_data = bytes((i * 17) & 0xFF for i in range(V_SIZE))
    K.write_file(os.path.join(src, VICTIM), v_data)
    for rel, data in SIBS.items():
        K.write_file(os.path.join(src, rel), data)
    dest = K.fresh_dest("mvwf_dest")
    try:
        K.with_scenario(f"enospc:{VICTIM}:1048576")   # writes OK to 1 MiB, then ENOSPC forever
        r = H.run(H.ASYNC, "mv", [src], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.SKIP,
                  expect_dir=None, fs_preload=K.fs_so(), memcheck=memcheck)
        K.with_scenario("")
        problems = []
        if not (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0):
            problems.append(f"run: completed={r.completed} alive={r.stayed_alive} "
                            f"oom={r.oom_killed} mem_err={r.mem_errors}")
        vp = os.path.join(src, VICTIM)
        if not os.path.exists(vp):
            problems.append("victim SOURCE deleted despite a mid-write dest failure -- DATA LOSS")
        elif K.read_file(vp) != v_data:
            problems.append("victim SOURCE corrupted")
        copied = os.path.join(dest, os.path.basename(src))
        for rel, data in SIBS.items():
            dp = os.path.join(copied, rel)
            if not os.path.exists(dp) or K.read_file(dp) != data:
                problems.append(f"sibling {rel} not moved correctly")
            if os.path.exists(os.path.join(src, rel)):
                problems.append(f"sibling {rel} source not removed (move incomplete)")
        vd = os.path.join(copied, VICTIM)
        if os.path.exists(vd) and os.path.getsize(vd) >= V_SIZE:
            problems.append("partial-dest victim is FULL-SIZE -- a size-only recheck would "
                            "treat it as complete (silent corruption)")
        if problems:
            print("      [async] FAIL: " + "; ".join(problems))
        return not problems
    finally:
        shutil.rmtree(src_dir, ignore_errors=True)


_WRAPPER = r'''
import os, sys, time, subprocess, hashlib
binp, work, home = sys.argv[1:4]
dest = os.path.join(work, "dest")
os.makedirs(dest, exist_ok=True)
subprocess.run(["mount","-t","tmpfs","-o","size=1500k","tmpfs",dest], check=True)
src = os.path.join(work, "src_tree")
os.makedirs(os.path.join(src,"sub"), exist_ok=True)
vdata = bytes((i*17)&0xFF for i in range(3*1024*1024))
open(os.path.join(src,"victim_big.bin"),"wb").write(vdata)
sibs = {"sib_a.dat": b"sibling A moves fine\n"*32, os.path.join("sub","sib_b.dat"): b"sibling B nested\n"*16}
for rel,data in sibs.items():
    open(os.path.join(src,rel),"wb").write(data)
vhash = hashlib.sha256(vdata).hexdigest()
env = dict(os.environ, HOME=home, QT_QPA_PLATFORM="offscreen", DISPLAY="",
           ULTRACOPIER_SOCKET_SUFFIX="test",
           XDG_CONFIG_HOME=os.path.join(home,".config"))
p = subprocess.Popen([binp,"mv",src,dest],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
def dbytes():
    t=0
    for dp,_,fs in os.walk(dest):
        for f in fs:
            try: t+=os.path.getsize(os.path.join(dp,f))
            except OSError: pass
    return t
last=-1; stable=0; crashed=False
for _ in range(90):
    time.sleep(1)
    rc=p.poll()
    if rc is not None and rc!=0: crashed=True; break
    cur=dbytes()
    stable = stable+1 if cur==last else 0
    last=cur
    if stable>=6: break
p.kill()
problems=[]
if crashed: problems.append("crashed")
vp=os.path.join(src,"victim_big.bin")
if not os.path.exists(vp): problems.append("victim SOURCE deleted -- DATA LOSS")
elif hashlib.sha256(open(vp,"rb").read()).hexdigest()!=vhash: problems.append("victim SOURCE corrupted")
copied=os.path.join(dest, os.path.basename(src))
for rel,data in sibs.items():
    dp=os.path.join(copied,rel)
    if not os.path.exists(dp) or open(dp,"rb").read()!=data: problems.append("sibling %s not moved"%rel)
    if os.path.exists(os.path.join(src,rel)): problems.append("sibling %s source not removed"%rel)
vd=os.path.join(copied,"victim_big.bin")
if os.path.exists(vd) and os.path.getsize(vd)>=3*1024*1024:
    problems.append("partial dest victim FULL-SIZE")
print("RESULT " + ("; ".join(problems) if problems else
      "ok: victim source safe, siblings moved, partial dest short"))
subprocess.run(["umount",dest],stderr=subprocess.DEVNULL)
sys.exit(1 if problems else 0)
'''


def _userns_available() -> bool:
    try:
        r = subprocess.run(
            ["unshare", "-Umr", "bash", "-c",
             'd=$(mktemp -d); mount -t tmpfs -o size=1m tmpfs "$d" && umount "$d" && rmdir "$d"'],
            capture_output=True, timeout=20)
        return r.returncode == 0
    except (OSError, subprocess.TimeoutExpired):
        return False


def _iouring_lane() -> bool:
    if not _userns_available():
        print("      [io_uring] SKIPPED: unprivileged userns/tmpfs not permitted (capability gate)")
        return True
    binp = H.binary_for(H.IO_URING, H.load_config(), asan=False)
    work = tempfile.mkdtemp(prefix="uc-mvwf-work-")
    home = pathlib.Path(tempfile.mkdtemp(prefix="uc-mvwf-home-"))
    H.write_config(home, file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.MERGE,
                   file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP)
    wrapper = pathlib.Path(tempfile.mkstemp(prefix="uc-mvwf-wrap-", suffix=".py")[1])
    wrapper.write_text(_WRAPPER)
    try:
        r = subprocess.run(["unshare", "-Umr", "python3", str(wrapper), binp, work, str(home)],
                           capture_output=True, text=True, timeout=200)
    finally:
        wrapper.unlink(missing_ok=True)
        subprocess.run(["pkill", "-9", "-x", "ultracopier"], capture_output=True)
        shutil.rmtree(home, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)
    for line in (r.stdout + r.stderr).splitlines():
        if line.strip():
            print(f"      [io_uring] {line.strip()}")
    return r.returncode == 0


def run(backends=None, memcheck=H.NONE) -> bool:
    bks = backends or [H.ASYNC, H.IO_URING]
    ok = True
    if H.ASYNC in bks:
        a = _async_lane(memcheck)
        print(f"    [async mv-dest-write-fault] {'PASS' if a else 'FAIL'}")
        ok = ok and a
    if H.IO_URING in bks:
        try:
            H.binary_for(H.IO_URING, H.load_config(), asan=False)
            b = _iouring_lane()
            print(f"    [io_uring mv-dest-full] {'PASS' if b else 'FAIL'}")
            ok = ok and b
        except Exception as e:
            print(f"    [io_uring] SKIP (binary unavailable: {e})")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
