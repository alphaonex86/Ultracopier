#!/usr/bin/env python3
r"""A source or destination whose PROTOCOL nothing can handle must be REFUSED -- loudly, and the
file manager that asked must be told so it can do the copy itself.

The reported bug, reproduced before the fix on the shipping binary:

  * `ultracopier cp sftp://root@127.0.0.1/XXX <dest>` -> the transfer window opened and NOTHING
    happened, ever. No file, no error, no notification: the order simply vanished.
  * `ultracopier cp <src> sftp://root@127.0.0.1/XXX` -> WORSE. The URL was treated as a local
    path, so the engine created a junk directory tree `./sftp:/root@127.0.0.1/XXX/` relative to
    its working directory and copied into it. The user believes the file went to the remote host;
    it sits in a folder named after the URL. With `mv` that is the shape of real data loss.

Root cause: CopyListener declared EVERY source and destination to be the "file" protocol, so
CopyEngineManager was never given a chance to say it has no engine for "sftp" (it only has one on
a KIO-enabled build). The fix classifies the real protocol (extractProtocol, pinned by
cases/protocol_unit.py) and Core refuses the transfer BEFORE anything is opened, then reports the
refusal back: catchcopy answers the order with code 5004 so a patched file manager falls back to
its own copy engine (see file-manager/*.patch), and a command-line user gets the warning.

Asserted here, on both entry points and for cp AND mv:
  * an unsupported protocol transfers nothing, creates NO junk local path, and leaves the source
    intact (the mv data-loss guard);
  * the catchcopy client is told 5004 -- that is the whole fallback contract;
  * plain local paths and `file://` URIs are NOT refused (the file managers send those all day, so
    a false positive here would break every paste);
  * the app stays alive and healthy throughout.
"""
import sys, os, time, pathlib, shutil

_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K
from lib import uclaunch as U

PAYLOAD = b"protocol-refused-payload\n"
REMOTE_SRC = "sftp://root@127.0.0.1/XXX"
REMOTE_DST = "sftp://root@127.0.0.1/XXX"


def _mk(root: pathlib.Path, name: str, data=PAYLOAD) -> pathlib.Path:
    p = root / name
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_bytes(data)
    return p


def _fresh(root: pathlib.Path, name: str) -> pathlib.Path:
    p = root / name
    shutil.rmtree(p, ignore_errors=True)
    p.mkdir(parents=True)
    return p


class Checks:
    def __init__(self):
        self.rows = []

    def add(self, name, ok, detail=""):
        self.rows.append((name, bool(ok), detail))
        print(f"      [{'PASS' if ok else 'FAIL'}] {name}{('  -- ' + detail) if detail else ''}")
        return bool(ok)

    @property
    def ok(self):
        return all(r[1] for r in self.rows)

    def failed(self):
        return [r[0] for r in self.rows if not r[1]]


def _tree(path: pathlib.Path):
    return sorted(str(p.relative_to(path)) for p in path.rglob("*"))


def _cli_refusals(root, backend, memcheck, c: Checks):
    """Each refusal legitimately warns the user with a modal, which blocks that instance -- so
    every command line gets its own instance, and each is killed externally."""
    src = _mk(_fresh(root, "cli_src"), "keep.txt")

    # 1. unsupported SOURCE protocol
    workdir = _fresh(root, "cli_work1")
    dest = _fresh(root, "cli_dest1")
    with U.Instance("proto-cli1", backend=backend, memcheck=memcheck, cwd=workdir,
                    args=["cp", REMOTE_SRC, str(dest)]) as inst:
        time.sleep(6 * inst.slow)
        c.add("CLI `cp sftp://... <dest>`: nothing is transferred",
              inst.alive() and not any(dest.iterdir()),
              f"dest={_tree(dest)} alive={inst.alive()} exit={inst.exit_code()}")

    # 2. unsupported DESTINATION protocol -- the junk-folder bug
    workdir = _fresh(root, "cli_work2")
    with U.Instance("proto-cli2", backend=backend, memcheck=memcheck, cwd=workdir,
                    args=["cp", str(src), REMOTE_DST]) as inst:
        time.sleep(6 * inst.slow)
        c.add("CLI `cp <src> sftp://...`: no junk local folder is created",
              inst.alive() and not any(workdir.iterdir()) and src.exists(),
              f"workdir={_tree(workdir)} source={src.exists()} alive={inst.alive()} exit={inst.exit_code()}")

    # 3. the same on a MOVE: the source must survive (this is the data-loss shape)
    workdir = _fresh(root, "cli_work3")
    mv_src = _mk(_fresh(root, "cli_mvsrc"), "must-survive.txt")
    with U.Instance("proto-cli3", backend=backend, memcheck=memcheck, cwd=workdir,
                    args=["mv", str(mv_src), REMOTE_DST]) as inst:
        time.sleep(6 * inst.slow)
        c.add("CLI `mv <src> sftp://...`: the source is NOT removed",
              inst.alive() and mv_src.exists() and mv_src.read_bytes() == PAYLOAD
              and not any(workdir.iterdir()),
              f"source_kept={mv_src.exists()} workdir={_tree(workdir)} alive={inst.alive()} exit={inst.exit_code()}")


def _catchcopy_refusals(root, backend, memcheck, c: Checks):
    """The file-manager contract: a refused order is answered 5004 so the caller copies it itself,
    while everything it legitimately sends (plain paths, file:// URIs) is accepted."""
    src_dir = _fresh(root, "cc_src")
    src = _mk(src_dir, "a.txt")
    mv_src = _mk(src_dir, "m.txt", b"move-me\n")
    workdir = _fresh(root, "cc_work")

    with U.Instance("proto-cc", backend=backend, memcheck=memcheck, cwd=workdir) as inst:
        if not c.add("catchcopy socket is listening", inst.wait_listening("catchcopy", 60)):
            return
        # --- accepted: a plain local copy is NOT refused (no false positive)
        d_ok = _fresh(root, "cc_ok")
        with U.Catchcopy(inst) as cc:
            cc.handshake(1)
            cc.order(2, ["cp", str(src), str(d_ok)])
            refused, replies = cc.verdict(2, 3)
            c.add("catchcopy: a plain local `cp` is NOT refused", not refused, f"replies={replies}")
            c.add("catchcopy: ...and it really transfers",
                  inst.wait_until(lambda: (d_ok / "a.txt").exists(), 60))

        # --- accepted: file:// URIs (what the patched file managers actually send)
        d_uri = _fresh(root, "cc_uri")
        with U.Catchcopy(inst) as cc:
            cc.handshake(1)
            cc.order(2, ["cp", f"file://{src}", f"file://{d_uri}"])
            refused, replies = cc.verdict(2, 3)
            c.add("catchcopy: a `file://` URI is NOT refused", not refused, f"replies={replies}")
            c.add("catchcopy: ...and the file:// transfer really happens",
                  inst.wait_until(lambda: (d_uri / "a.txt").exists(), 60))

        # --- refused: unsupported source protocol
        d_ref = _fresh(root, "cc_refused")
        with U.Catchcopy(inst) as cc:
            cc.handshake(1)
            cc.order(2, ["cp", REMOTE_SRC, str(d_ref)])
            refused, replies = cc.verdict(2, 3)
            c.add("catchcopy: `cp sftp://... <dest>` is refused with 5004", refused,
                  f"replies={replies}")
            time.sleep(2 * inst.slow)
            c.add("catchcopy: ...and nothing is transferred", not any(d_ref.iterdir()))

        # --- refused: unsupported destination protocol, and no junk folder
        with U.Catchcopy(inst) as cc:
            cc.handshake(1)
            cc.order(2, ["cp", str(src), REMOTE_DST])
            refused, replies = cc.verdict(2, 3)
            c.add("catchcopy: `cp <src> sftp://...` is refused with 5004", refused,
                  f"replies={replies}")
            time.sleep(2 * inst.slow)
            c.add("catchcopy: ...and no junk local folder is created", not any(workdir.iterdir()),
                  f"workdir={_tree(workdir)}")

        # --- refused: a MOVE to an unsupported destination keeps the source
        with U.Catchcopy(inst) as cc:
            cc.handshake(1)
            cc.order(2, ["mv", str(mv_src), REMOTE_DST])
            refused, replies = cc.verdict(2, 3)
            c.add("catchcopy: `mv <src> sftp://...` is refused with 5004", refused,
                  f"replies={replies}")
            time.sleep(2 * inst.slow)
            c.add("catchcopy: ...and the source is NOT removed",
                  mv_src.exists() and mv_src.read_bytes() == b"move-me\n")

        # --- refused: the other protocols a file manager can hand us
        for scheme in ("smb://server/share/file", "ftp://ftp.example.org/pub/file",
                       "webdav://host/x", "SFTP://root@127.0.0.1/UPPER"):
            d_s = _fresh(root, "cc_" + scheme.split(":")[0].lower())
            with U.Catchcopy(inst) as cc:
                cc.handshake(1)
                cc.order(2, ["cp", scheme, str(d_s)])
                refused, replies = cc.verdict(2, 3)
                c.add(f"catchcopy: `cp {scheme.split('://')[0]}://...` is refused", refused,
                      f"replies={replies}")

        # --- refused: a mixed list -- ONE unsupported source refuses the whole order (partial
        #     acceptance would leave the file manager unable to know what it still has to copy)
        d_mix = _fresh(root, "cc_mixed")
        with U.Catchcopy(inst) as cc:
            cc.handshake(1)
            cc.order(2, ["cp", str(src), REMOTE_SRC, str(d_mix)])
            refused, replies = cc.verdict(2, 3)
            c.add("catchcopy: a list mixing a local and a remote source is refused", refused,
                  f"replies={replies}")

        c.add("the instance survived every refusal", inst.alive())
        c.add("no memory error on the refusal paths", inst.mem_errors() == 0)


def run(backends=None, memcheck=H.NONE) -> bool:
    if memcheck == H.TSAN:
        # The FULL ultracopier binary cannot run under ThreadSanitizer: it dies within seconds with
        # "CHECK failed: sanitizer_thread_registry.cpp:186" (QtGui/offscreen + QThreadPool pooled
        # threads exit in a way TSan's registry cannot track -- a Qt-vs-TSan incompatibility
        # OUTSIDE our code; re-confirmed 2026-09-09, exit 66 on a plain local copy). These cases
        # DRIVE that binary, so there is nothing to run here. The engine's real TSan gate is
        # cases/tsan_engine_api.py, which hosts the same engine threads without QtGui.
        print("    [protocol_refused] SKIP under TSan (the full app dies in Qt's threadpool under TSan; the\n"
              "        thread gate is cases/tsan_engine_api.py)")
        return True
    bes = [b for b in (backends or [H.ASYNC]) if b in (H.ASYNC, H.IO_URING)]
    if not bes:
        print("    [protocol_refused] SKIP (async/io_uring only)")
        return True
    backend = H.ASYNC if H.ASYNC in bes else bes[0]
    H._kill_all_ultracopier()
    root = pathlib.Path(K.fresh_dest("protocol_refused"))
    c = Checks()
    try:
        _cli_refusals(root, backend, memcheck, c)
        _catchcopy_refusals(root, backend, memcheck, c)
    finally:
        H._kill_all_ultracopier()
        shutil.rmtree(root, ignore_errors=True)
    if c.ok:
        print(f"    [protocol_refused] PASS  ({len(c.rows)} checks on {backend})")
    else:
        print(f"    [protocol_refused] FAIL  ({len(c.failed())}/{len(c.rows)}): {', '.join(c.failed())}")
    return c.ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
