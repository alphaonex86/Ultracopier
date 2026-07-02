#!/usr/bin/env python3
"""Destination INSIDE the source (recursion guard): cp /X -> /X/sub_dest.

If the scan (or a collision-triggered folder re-list) picks up the destination it is itself
creating under the source, the copy recurses X/sub_dest/X/sub_dest/... until the disk fills
-- a runaway that eats the machine. The entire run happens inside an unprivileged user+mount
namespace with a small tmpfs (the iouring_dest_full rig), so a runaway hits ENOSPC in
seconds instead of filling real disk, and the blast radius is zero.

Source ~600 KiB in an 8 MiB tmpfs: a correct copy uses ~1.2 MiB (source + one nested copy);
a recursion would nest repeatedly (visible directly as sub_dest/<base>/sub_dest/<base>/...)
and push toward the 8 MiB cap. ASSERTS (from the wrapper, exit!=0 on violation):
  * the engine does NOT crash (nonzero exit while running = crash; it is a tray app);
  * NO runaway: the copied tree contains at most ONE level of self-nesting -- the marker
    path sub_dest/<base>/sub_dest/<base> must NOT exist;
  * the job goes quiet (dest stable) within the window -- a live-lock never settling is a
    fail;
  * the SOURCE's original files are untouched (byte-identical afterwards).
Backend note: driven for BOTH Linux backends via the same wrapper (the recursion decision
is in the shared scan code, but both data planes are cheap to pin here).
"""
import os, sys, shutil, tempfile, pathlib, subprocess
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

_WRAPPER = r'''
import os, sys, time, subprocess, hashlib
binp, work, home = sys.argv[1:4]
subprocess.run(["mount","-t","tmpfs","-o","size=8m","tmpfs",work], check=True)
src = os.path.join(work, "rg_tree")
os.makedirs(os.path.join(src, "sub"), exist_ok=True)
files = {}
for rel, n in (("f1.bin", 200*1024), ("f2.bin", 300*1024), (os.path.join("sub","f3.bin"), 100*1024)):
    p = os.path.join(src, rel)
    data = bytes((i*31) & 0xFF for i in range(n))
    open(p, "wb").write(data)
    files[p] = hashlib.sha256(data).hexdigest()
dest = os.path.join(src, "sub_dest")
os.makedirs(dest, exist_ok=True)
env = dict(os.environ, HOME=home, QT_QPA_PLATFORM="offscreen", DISPLAY="",
           ULTRACOPIER_SOCKET_SUFFIX="test",
           XDG_CONFIG_HOME=os.path.join(home, ".config"))
p = subprocess.Popen([binp, "cp", src, dest], env=env,
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
def dbytes():
    t = 0
    for dp, _, fs in os.walk(dest):
        for f in fs:
            try: t += os.path.getsize(os.path.join(dp, f))
            except OSError: pass
    return t
last=-1; stable=0; crashed=False; settled=False
for _ in range(90):
    time.sleep(1)
    rc = p.poll()
    if rc is not None and rc != 0:
        crashed = True; break
    cur = dbytes()
    stable = stable+1 if cur == last else 0
    last = cur
    if stable >= 6:
        settled = True; break
p.kill()
base = os.path.basename(src)
runaway = os.path.isdir(os.path.join(dest, base, "sub_dest", base))
src_bad = []
for pth, want in files.items():
    try:
        got = hashlib.sha256(open(pth,"rb").read()).hexdigest()
        if got != want: src_bad.append(os.path.basename(pth) + ":corrupt")
    except OSError:
        src_bad.append(os.path.basename(pth) + ":missing")
print("RESULT crashed=%s settled=%s runaway=%s src_bad=%s dest_bytes=%d"
      % (crashed, settled, runaway, ",".join(src_bad) or "none", dbytes()))
subprocess.run(["umount", work], stderr=subprocess.DEVNULL)
sys.exit(0 if (not crashed and settled and not runaway and not src_bad) else 1)
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


def run(backends=None, memcheck=H.NONE) -> bool:
    if not _userns_available():
        print("    [dest_inside_source] SKIPPED: unprivileged user namespaces / tmpfs mount "
              "not permitted on this kernel (capability gate, not a failure)")
        return True

    cfg = H.load_config()
    bks = backends or [H.ASYNC, H.IO_URING]
    ok = True
    for backend in bks:
        if backend not in (H.ASYNC, H.IO_URING):
            continue
        try:
            binp = H.binary_for(backend, cfg, asan=False)
        except Exception as e:
            print(f"    [{backend}] SKIP (binary unavailable: {e})")
            continue
        work = tempfile.mkdtemp(prefix="uc-rg-work-")
        home = pathlib.Path(tempfile.mkdtemp(prefix="uc-rg-home-"))
        H.write_config(home, file_collision=H.FileCollision.OVERWRITE,
                       folder_collision=H.FolderCollision.MERGE,
                       file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP)
        wrapper = pathlib.Path(tempfile.mkstemp(prefix="uc-rg-wrap-", suffix=".py")[1])
        wrapper.write_text(_WRAPPER)
        try:
            r = subprocess.run(["unshare", "-Umr", "python3", str(wrapper),
                                binp, work, str(home)],
                               capture_output=True, text=True, timeout=200)
        finally:
            wrapper.unlink(missing_ok=True)
            subprocess.run(["pkill", "-9", "-x", "ultracopier"], capture_output=True)
            shutil.rmtree(home, ignore_errors=True)
            shutil.rmtree(work, ignore_errors=True)
        for line in (r.stdout + r.stderr).splitlines():
            if line.strip():
                print(f"      [{backend}] {line.strip()}")
        sub_ok = (r.returncode == 0)
        print(f"    [{backend} dest-inside-source] {'PASS' if sub_ok else 'FAIL'}")
        ok = ok and sub_ok
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
