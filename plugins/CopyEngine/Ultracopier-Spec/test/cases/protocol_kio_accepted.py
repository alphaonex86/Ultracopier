#!/usr/bin/env python3
"""With the KDE/KIO flag ON, a protocol the copy engine declares must be ACCEPTED -- refusing is
only for what nothing can handle.

`cases/protocol_refused.py` pins the OFF side: a stock build declares only "file", so `sftp://...`
is refused and the file manager falls back. This case pins the ON side of the very same switch:
built with `DEFINES+=ULTRACOPIER_PLUGIN_KIO`, `CopyEngineFactory::supportedProtocolsForThe
Source/Destination()` also declares sftp/smb/ftp/fish/nfs/mtp/webdav(s), and Core must then hand
the transfer to the engine instead of refusing it.

Both answers come from the SAME place: the list is declared ENTIRELY inside the copy-engine plugin
(`plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.cpp`, under the `#ifdef`). The core never
names a protocol -- it only compares what the transfer uses against what the loaded plugins say
they support -- so support (or its absence) stays 100% a plugin matter, and the general refusal
still applies to anything no plugin declares, KIO build or not.

Asserted on a real KIO build, through the catchcopy socket (the file-manager path):
  * `cp sftp://... <local dest>` is NOT refused -- no 5004 comes back, the engine took the job;
  * a protocol NOBODY declares even with KIO (`gopher://`) IS still refused with 5004, so the
    fallback contract keeps working for genuinely unsupported protocols;
  * for contrast, the SAME sftp order IS refused by the stock (no-KIO) build -- proving the flag,
    and only the flag, is what changes the answer.

The transfer itself is not expected to succeed (there is no reachable sftp server, and asserting
otherwise would need a live remote host): what is asserted is the ACCEPT/REFUSE decision, which is
what the file manager acts on. Self-skips (clean PASS) when KF6 KIOCore is not installed."""
import sys, os, time, pathlib, shutil, subprocess

_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K
from lib import uclaunch as U

_SOURCES = pathlib.Path(__file__).resolve().parents[5]
_HOOKS = pathlib.Path(__file__).resolve().parents[1] / "hooks"
_BUILD = _SOURCES / "build" / "test-async-kio"

# One protocol is enough to prove the switch; sftp is the one the report was about.
KIO_PROTOCOL = "sftp://root@127.0.0.1/XXX"
# A scheme no build declares -- the general refusal must survive the KIO flag.
UNKNOWN_PROTOCOL = "gopher://example.org/0/x"


def _kio_available() -> bool:
    """KF6 KIOCore present? (headers + library, the two the .pro needs)."""
    inc = pathlib.Path(os.environ.get("KF6_INCLUDE_PATH", "/usr/include/KF6"))
    if (inc / "KIOCore" / "KIO" / "CopyJob").exists() and (inc / "KIO" / "kio_version.h").exists():
        return True
    return subprocess.run(["pkg-config", "--exists", "KF6KIOCore"]).returncode == 0


def _build_kio_binary() -> str:
    """Build the all-in-one binary WITH the KIO flag (its own build dir, so it never disturbs the
    per-backend builds the harness owns). Same test hooks as every other test build."""
    _BUILD.mkdir(parents=True, exist_ok=True)
    flags = ["CONFIG+=release", "CONFIG+=nodebug", "DEFINES+=ULTRACOPIER_PLUGIN_KIO",
             f"SOURCES+={_HOOKS / 'FileErrorDialogHook.cpp'}",
             f"SOURCES+={_HOOKS / 'CollisionDialogHook.cpp'}"]
    env = dict(os.environ)
    # Force the async backend: liburing on the box would otherwise select io_uring here, and this
    # case is about the protocol decision, which is backend-independent.
    env["PKG_CONFIG_LIBDIR"] = "/tmp/uc-empty-pc"
    os.makedirs("/tmp/uc-empty-pc", exist_ok=True)
    stamp = _BUILD / ".uc-qmake-flags"
    want = "\n".join(flags)
    if not (_BUILD / "Makefile").exists() or not stamp.exists() or stamp.read_text() != want:
        r = subprocess.run(["qmake6", "-o", "Makefile", str(_SOURCES / "ultracopier.pro"),
                            "-spec", "linux-g++"] + flags, cwd=_BUILD, env=env,
                           capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError(f"qmake (KIO) failed:\n{r.stdout[-1500:]}\n{r.stderr[-1500:]}")
        stamp.write_text(want)
    r = subprocess.run(["make", f"-j{os.cpu_count()}"], cwd=_BUILD, env=env,
                       capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(f"build (KIO) failed:\n{r.stdout[-2000:]}\n{r.stderr[-2000:]}")
    return str(_BUILD / "ultracopier")


def _verdict(inst, order, dest) -> tuple:
    """Send one catchcopy cp order and return (refused, replies) -- exactly the question a patched
    file manager asks before deciding whether to copy the files itself."""
    with U.Catchcopy(inst) as cc:
        cc.handshake(1)
        cc.order(2, ["cp", order, str(dest)])
        return cc.verdict(2, 3)


def run(backends=None, memcheck=H.NONE) -> bool:
    if memcheck == H.TSAN:
        print("    [protocol_kio_accepted] SKIP under TSan (the full app dies in Qt's threadpool "
              "under TSan; the thread gate is cases/tsan_engine_api.py)")
        return True
    if not _kio_available():
        print("    [protocol_kio_accepted] SKIP (KF6 KIOCore not installed -- nothing to build "
              "the ULTRACOPIER_PLUGIN_KIO variant with)")
        return True

    H._kill_all_ultracopier()
    root = pathlib.Path(K.fresh_dest("protocol_kio"))
    rows = []
    try:
        kio_binary = _build_kio_binary()
        dest = root / "dest"
        dest.mkdir(parents=True, exist_ok=True)
        workdir = root / "work"
        workdir.mkdir(parents=True, exist_ok=True)

        # --- the flag ON: the engine declares sftp, so the transfer must be ACCEPTED
        with U.Instance("kio-on", memcheck=memcheck, cwd=workdir, binary=kio_binary) as inst:
            if not inst.wait_listening("catchcopy", 60):
                print("    [protocol_kio_accepted] FAIL: the KIO build never listened")
                return False
            refused, replies = _verdict(inst, KIO_PROTOCOL, dest)
            rows.append(("KIO build: `cp sftp://...` is ACCEPTED (no 5004)", not refused,
                         f"replies={replies}"))
            refused_unknown, replies_u = _verdict(inst, UNKNOWN_PROTOCOL, dest)
            rows.append(("KIO build: a protocol nobody declares is STILL refused",
                         refused_unknown, f"replies={replies_u}"))
            rows.append(("KIO build: the instance survived both orders", inst.alive(), ""))

        # --- the flag OFF (the shipping default): the same order must be refused
        with U.Instance("kio-off", memcheck=memcheck, cwd=workdir) as inst:
            if not inst.wait_listening("catchcopy", 60):
                print("    [protocol_kio_accepted] FAIL: the stock build never listened")
                return False
            refused_off, replies_off = _verdict(inst, KIO_PROTOCOL, dest)
            rows.append(("stock build (no KIO): the same `cp sftp://...` IS refused", refused_off,
                         f"replies={replies_off}"))
    except Exception as e:
        print(f"    [protocol_kio_accepted] EXCEPTION: {e}")
        return False
    finally:
        H._kill_all_ultracopier()
        shutil.rmtree(root, ignore_errors=True)

    ok = all(r[1] for r in rows)
    for name, good, detail in rows:
        print(f"      [{'PASS' if good else 'FAIL'}] {name}{('  -- ' + detail) if detail else ''}")
    print(f"    [protocol_kio_accepted] {'PASS' if ok else 'FAIL'}  ({len(rows)} checks)")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
