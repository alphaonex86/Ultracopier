#!/usr/bin/env python3
"""#HIGH hang (2026-07-02 coverage review, reproduced): a DESTINATION that is (or resolves through) a
symlink is resolved by ScanFileOrFolder in UNGUARDED while loops (addToList `while(is_symlink(dest))`
and resolvDestination `while(nbytes!=-1) readlink`). A CYCLIC destination symlink (a -> b -> a, or a
self-loop) makes those loops never terminate -> the scan thread SPINS a core forever = HANG (the copy
never starts, the process never goes idle).

FIX: cap each resolution loop at MAXSYMLINKS (40, Linux) -- a too-deep/cyclic chain stops resolving
(the dest is then handled by the normal create path, which fails ELOOP gracefully) instead of spinning.

ASSERTS (async + io_uring):
 (a) POSITIVE -- a dest that is a symlink to a REAL directory resolves correctly: the file lands in the
     symlink's TARGET dir, job completes/alive.
 (b) CYCLIC -- a dest symlink cycle does NOT hang: the engine reaches idle within a few seconds and does
     NOT peg a core (measured via /proc CPU ticks). RED before the fix (spins to the copy timeout).
"""
import sys, os, pathlib, shutil, subprocess, time, hashlib
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H


def _positive(backend):
    """dest is a symlink -> a real dir; the copy must land in the TARGET."""
    root = H.load_config().get("paths", "DESTINATIONLINUX", fallback="/tmp/ultracopier-spec-test")
    base = os.path.join(root, f"destsym_pos_{backend}_{os.getpid()}")
    shutil.rmtree(base, ignore_errors=True)
    src = os.path.join(base, "src"); os.makedirs(src)
    payload = b"lands-in-the-symlink-target\n"
    with open(os.path.join(src, "f.txt"), "wb") as f:
        f.write(payload)
    real_target = os.path.join(base, "realdst"); os.makedirs(real_target)
    link_dst = os.path.join(base, "linkdst")
    os.symlink(real_target, link_dst)     # dest is a symlink -> real dir
    ok = True
    note = ""
    try:
        r = H.run(backend, "cp", [os.path.join(src, "f.txt")], link_dst,
                  file_collision=H.FileCollision.OVERWRITE, folder_collision=H.FolderCollision.MERGE,
                  expect_dir=None)
        landed = os.path.join(real_target, "f.txt")   # must appear in the TARGET, not somewhere else
        good = os.path.exists(landed) and hashlib.sha256(open(landed, "rb").read()).hexdigest() == hashlib.sha256(payload).hexdigest()
        ok = r.stayed_alive and r.completed and good
        note = f"positive: alive={r.stayed_alive} completed={r.completed} landed_in_target={good}"
    finally:
        shutil.rmtree(base, ignore_errors=True)
    print(f"      [{backend}] {note}")
    return ok


def _cyclic(backend):
    """dest is a CYCLIC symlink -> must NOT hang (no core-pegging spin)."""
    binp = H.binary_for(backend, H.load_config())
    base = os.path.join("/dev/shm", f"destsym_cyc_{backend}_{os.getpid()}")
    home = os.path.join("/dev/shm", f"destsym_cychome_{backend}_{os.getpid()}")
    for p in (base, home):
        shutil.rmtree(p, ignore_errors=True)
    ok = True
    note = ""
    try:
        os.makedirs(os.path.join(base, "src"))
        with open(os.path.join(base, "src", "f.txt"), "wb") as f:
            f.write(b"hi\n")
        os.symlink(os.path.join(base, "cyclic2"), os.path.join(base, "cyclic"))   # a -> b
        os.symlink(os.path.join(base, "cyclic"), os.path.join(base, "cyclic2"))   # b -> a
        H.write_config(pathlib.Path(home), file_collision=H.FileCollision.OVERWRITE,
                       folder_collision=H.FolderCollision.MERGE, file_error=H.FileError.SKIP,
                       folder_error=H.FolderError.SKIP)
        env = dict(os.environ, HOME=home, XDG_CONFIG_HOME=os.path.join(home, ".config"),
                   QT_QPA_PLATFORM="offscreen", DISPLAY="", ULTRACOPIER_SOCKET_SUFFIX=f"destsymcyc{backend}")
        p = subprocess.Popen([binp, "cp", os.path.join(base, "src", "f.txt"), os.path.join(base, "cyclic")],
                             env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(3)
        pid = next((int(x) for x in subprocess.run(["pgrep", "-x", "ultracopier"], capture_output=True, text=True).stdout.split()), None)
        c1 = open(f"/proc/{pid}/stat").read().split()[13:15] if pid else None
        time.sleep(2)
        c2 = open(f"/proc/{pid}/stat").read().split()[13:15] if pid and os.path.exists(f"/proc/{pid}/stat") else None
        ticks = (int(c2[0]) + int(c2[1])) - (int(c1[0]) + int(c1[1])) if c1 and c2 else -1
        p.kill()   # OUR instance only (its suffix differs from the harness one), never the user's resident copy
        try: p.wait(timeout=5)
        except subprocess.TimeoutExpired: pass
        spinning = ticks > 150   # >1.5s CPU in a 2s window == pegging a core in the resolution loop
        ok = not spinning
        note = f"cyclic: cpu_ticks_in_2s={ticks} spinning={spinning}"
        if spinning:
            note += "  *** HANG: cyclic dest symlink spins the scan resolution loop ***"
    finally:
        for p in (base, home):
            shutil.rmtree(p, ignore_errors=True)
        H._kill_all_ultracopier()   # scoped: only THIS suite's instances, never the user's resident one
    print(f"      [{backend}] {note}")
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    bes = [b for b in (backends or [H.ASYNC]) if b in (H.ASYNC, H.IO_URING)]
    if not bes:
        print("    [dest_symlink] SKIP (async/io_uring only)")
        return True
    ok = True
    for b in bes:
        ok = _positive(b) and ok
        ok = _cyclic(b) and ok
    print(f"    [dest_symlink] {'PASS  (dest symlink resolves; cyclic dest does not hang)' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
