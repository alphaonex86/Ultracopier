#!/usr/bin/env python3
"""Regression test for the clipboard-paste DOUBLE-COPY bug (the Dolphin Ctrl+V report).

A single paste can be delivered to Ultracopier TWICE -- from the KDE/catchcopy integration
AND the in-process Ctrl+V hook. The old dedup compared the RAW strings within a 1s window, so
two deliveries that differ in form (plain path vs file:// URI, a trailing slash on the dest) or
lag >1s slipped through and ran the copy TWICE: a "file exists" collision on a freshly-emptied
destination plus ~2x the time. Fix: CopyListener::isDuplicatePaste now compares NORMALIZED paths
(file:// stripped + percent-decoded, trailing slashes trimmed) within a 2s window.

We drive the REAL catchcopy socket (advanced-copier-<uid>, the same endpoint Dolphin's
integration uses), send the same paste twice in each tricky form, and assert it copies ONCE.

Backend-agnostic signal: with fileCollision=RENAME a deduped paste leaves dest == source, but a
DOUBLED paste makes the second job collide with the first's output and RENAME it -> the dest then
holds MORE files than the source. So we just count files (works on async/io_uring/IOCP alike).
The dedup itself is in CopyListener, BEFORE the copy engine, so it is identical on every backend.
"""
import sys, os, time, struct, socket, glob, tempfile, shutil, subprocess, pathlib, configparser
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K


# ---- minimal catchcopy (advanced-copier) wire protocol: [size][idNext][QStringList] ----
def _qstr(s):  b = s.encode("utf-16-be"); return struct.pack(">I", len(b)) + b
def _qslist(lst):
    out = struct.pack(">I", len(lst))
    for s in lst: out += _qstr(s)
    return out
def _blk(idn, order):
    body = struct.pack(">i", idn) + _qslist(order)
    return struct.pack(">i", 4 + len(body)) + body
def _sockpath():
    g = glob.glob(f"/tmp/advanced-copier-{os.getuid()}-{H.TEST_SOCKET_SUFFIX}*")
    return g[0] if g else None
def _send(order):
    p = _sockpath()
    if not p: raise RuntimeError("catchcopy socket not found")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM); s.connect(p)
    s.sendall(_blk(1, ["protocol", "0002"])); time.sleep(0.12)
    try: s.settimeout(1.0); s.recv(8192)
    except Exception: pass
    s.sendall(_blk(2, order)); time.sleep(0.2); s.close()


def _make_src(root):
    """A tiny tree (copies in well under the inter-send gap so the 2nd job, if any,
    deterministically collides with the 1st job's finished output)."""
    shutil.rmtree(root, ignore_errors=True)
    for d in range(5):
        sub = os.path.join(root, f"d{d}"); os.makedirs(sub, exist_ok=True)
        for f in range(10):
            with open(os.path.join(sub, f"f{f}.bin"), "wb") as fh:
                fh.write(bytes((d * 10 + f) % 256 for _ in range(256)))
    return root

def _count_files(root):
    return sum(1 for dp, _d, fs in os.walk(root) for _f in fs) if os.path.isdir(root) else -1


def _start_resident(backend):
    binp = H.binary_for(backend, configparser.ConfigParser())
    home = pathlib.Path(tempfile.mkdtemp(prefix="uc-dedup-home-"))
    # RENAME so a stray 2nd copy is visible as EXTRA files (no dialog, fully headless).
    H.write_config(home, file_collision=H.FileCollision.RENAME,
                   folder_collision=H.FolderCollision.MERGE,
                   file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP)
    H._kill_all_ultracopier()
    env = dict(os.environ); env.update(HOME=str(home), QT_QPA_PLATFORM="offscreen", DISPLAY="",
               XDG_CONFIG_HOME=str(home / ".config"), ULTRACOPIER_SOCKET_SUFFIX=H.TEST_SOCKET_SUFFIX)
    subprocess.Popen([binp], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for _ in range(40):
        time.sleep(0.25)
        if _sockpath() and H._test_ultracopier_pids(): break
    return home


def run(backends=None, memcheck=H.NONE) -> bool:
    src = _make_src(K.fresh_src_root("dedup_src"))
    n_src = _count_files(src)
    backends = backends or [H.ASYNC, H.IO_URING]

    # (name, second-source, second-dest-suffix, gap-between-sends<2s)
    VARIATIONS = [
        ("identical, 1.3s gap", src,            "",  1.3),
        ("trailing-slash dest", src,            "/", 0.8),
        ("path vs file:// src",  "file://" + src, "", 0.8),
    ]

    def one(backend):
        home = _start_resident(backend)
        try:
            if _sockpath() is None:
                print("        catchcopy socket never appeared"); return False
            ok = True
            for name, src2, dsuf, gap in VARIATIONS:
                dest = K.fresh_dest(f"dedup_{backend}")
                _send(["cp", src, dest]); time.sleep(gap); _send(["cp", src2, dest + dsuf])
                time.sleep(3.0)   # let both jobs (if any) finish
                tgt = os.path.join(dest, os.path.basename(src))
                n_dst = _count_files(tgt)
                single = (n_dst == n_src)
                print(f"        [{backend}] {name:22} files src={n_src} dst={n_dst} "
                      f"-> {'PASS (single)' if single else 'FAIL (DOUBLED)'}")
                ok = ok and single
                time.sleep(2.2)   # let the dedup window lapse before the next variation
            return ok
        finally:
            H._kill_all_ultracopier(); shutil.rmtree(home, ignore_errors=True)

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
