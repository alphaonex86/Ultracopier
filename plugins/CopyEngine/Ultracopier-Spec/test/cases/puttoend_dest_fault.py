#!/usr/bin/env python3
"""fileError=PUT_TO_END against a PERSISTENT destination write fault: bounded give-up.

The classic overnight failure: the destination fills (or dies) and the job spins at 100%
re-trying forever -- by morning nothing else was copied either. PUT_TO_END re-queues a
failed file for a later retry; with a fault that NEVER clears, the retry loop's give-up
bound is the only thing standing between "job completes minus the victim" and an infinite
loop. That bound is proven for async SOURCE-read faults (faulty_hdd) and for a RECOVERABLE
dest fault (media_reconnect wdisconnect); a PERSISTENT dest-write fault has zero coverage.

  * async: shim `enospc:<victim>:1048576` on a cp -- every retry re-fails (the disk never
    empties). fileError=PUT_TO_END.
  * io_uring: the userns small-tmpfs rig (REAL ENOSPC on the ring data plane, persistent --
    nothing is freed), fileError=PUT_TO_END.

ASSERTS: the job COMPLETES within the watchdog window (bounded give-up -- not completing ==
the infinite-retry hang); every sibling that fits is copied byte-perfect; the victim's dest
is absent or SHORT (never full-size-wrong); process alive, memory clean.
"""
import os, sys, shutil, tempfile, pathlib, subprocess, hashlib
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

VICTIM = "victim_never_fits.bin"
V_SIZE = 3 * 1024 * 1024
SIBS = {"sib_a.dat": b"sibling A fits\n" * 64,
        os.path.join("sub", "sib_b.dat"): bytes((i * 23) & 0xFF for i in range(200 * 1024))}


def _async_lane(memcheck) -> bool:
    src = K.fresh_src_root("pted_src")
    v_data = bytes((i * 19) & 0xFF for i in range(V_SIZE))
    K.write_file(os.path.join(src, VICTIM), v_data)
    for rel, data in SIBS.items():
        K.write_file(os.path.join(src, rel), data)
    dest = K.fresh_dest("pted_dest")
    K.with_scenario(f"enospc:{VICTIM}:1048576")     # persistent: every retry re-fails at 1 MiB
    r = H.run(H.ASYNC, "cp", [src], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.PUT_TO_END,
              expect_dir=None, fs_preload=K.fs_so(), memcheck=memcheck)
    K.with_scenario("")
    problems = []
    if not (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0):
        problems.append(f"run: completed={r.completed} alive={r.stayed_alive} "
                        f"oom={r.oom_killed} mem_err={r.mem_errors}"
                        + ("  <-- INFINITE RETRY (no bounded give-up)" if not r.completed else ""))
    copied = os.path.join(dest, os.path.basename(src))
    for rel, data in SIBS.items():
        p = os.path.join(copied, rel)
        if not os.path.exists(p) or K.read_file(p) != data:
            problems.append(f"sibling {rel} not copied correctly")
    vd = os.path.join(copied, VICTIM)
    if os.path.exists(vd) and os.path.getsize(vd) >= V_SIZE:
        problems.append("victim dest FULL-SIZE despite persistent ENOSPC -- silent corruption")
    if problems:
        print("      [async] FAIL: " + "; ".join(problems))
    return not problems


_WRAPPER = r'''
import os, sys, time, subprocess
binp, work, home = sys.argv[1:4]
dest = os.path.join(work, "dest"); os.makedirs(dest, exist_ok=True)
subprocess.run(["mount","-t","tmpfs","-o","size=1500k","tmpfs",dest], check=True)
src = os.path.join(work, "src_tree"); os.makedirs(os.path.join(src,"sub"), exist_ok=True)
sibs = {"sib_a.dat": b"sibling A fits\n"*64, os.path.join("sub","sib_b.dat"): bytes((i*23)&0xFF for i in range(200*1024))}
for rel,data in sibs.items():
    open(os.path.join(src,rel),"wb").write(data)
open(os.path.join(src,"victim_never_fits.bin"),"wb").write(bytes((i*19)&0xFF for i in range(3*1024*1024)))
env = dict(os.environ, HOME=home, QT_QPA_PLATFORM="offscreen", DISPLAY="",
           ULTRACOPIER_SOCKET_SUFFIX="test",
           XDG_CONFIG_HOME=os.path.join(home,".config"))
p = subprocess.Popen([binp,"cp",src,dest],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
def dbytes():
    t=0
    for dp,_,fs in os.walk(dest):
        for f in fs:
            try: t+=os.path.getsize(os.path.join(dp,f))
            except OSError: pass
    return t
last=-1; stable=0; crashed=False; settled=False
for _ in range(120):
    time.sleep(1)
    rc=p.poll()
    if rc is not None and rc!=0: crashed=True; break
    cur=dbytes()
    stable = stable+1 if cur==last else 0
    last=cur
    if stable>=8: settled=True; break
p.kill()
problems=[]
if crashed: problems.append("crashed")
if not settled: problems.append("never settled: infinite ENOSPC retry (no bounded give-up)")
copied=os.path.join(dest, os.path.basename(src))
for rel,data in sibs.items():
    dp=os.path.join(copied,rel)
    if not os.path.exists(dp) or open(dp,"rb").read()!=data: problems.append("sibling %s wrong"%rel)
vd=os.path.join(copied,"victim_never_fits.bin")
if os.path.exists(vd) and os.path.getsize(vd)>=3*1024*1024:
    problems.append("victim dest FULL-SIZE despite ENOSPC")
print("RESULT " + ("; ".join(problems) if problems else "ok: bounded give-up, siblings copied"))
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
    work = tempfile.mkdtemp(prefix="uc-pted-work-")
    home = pathlib.Path(tempfile.mkdtemp(prefix="uc-pted-home-"))
    H.write_config(home, file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.MERGE,
                   file_error=H.FileError.PUT_TO_END, folder_error=H.FolderError.SKIP)
    wrapper = pathlib.Path(tempfile.mkstemp(prefix="uc-pted-wrap-", suffix=".py")[1])
    wrapper.write_text(_WRAPPER)
    try:
        r = subprocess.run(["unshare", "-Umr", "python3", str(wrapper), binp, work, str(home)],
                           capture_output=True, text=True, timeout=260)
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
        print(f"    [async put-to-end dest-fault] {'PASS' if a else 'FAIL'}")
        ok = ok and a
    if H.IO_URING in bks:
        try:
            H.binary_for(H.IO_URING, H.load_config(), asan=False)
            b = _iouring_lane()
            print(f"    [io_uring put-to-end ENOSPC] {'PASS' if b else 'FAIL'}")
            ok = ok and b
        except Exception as e:
            print(f"    [io_uring] SKIP (binary unavailable: {e})")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
