#!/usr/bin/env python3
"""The FILE-MANAGER patches themselves, compiled and run against a live Ultracopier.

`file-manager/*.patch` teach Dolphin, pcmanfm-qt and libfm-qt to hand a Ctrl+V to Ultracopier over
the catchcopy socket -- and, since the protocol-refusal fix, to notice when Ultracopier REFUSES the
job (an unsupported protocol like `sftp://`) and copy the files with their own engine instead of
losing the paste. That decision is made by code that lives ONLY inside those patch files, so it
would otherwise never be compiled or executed by anything here.

This case takes the code straight out of each patch (the new side of every hunk, so the exact
lines that will land in the file manager), links it with a tiny driver that performs the same paste
sequence the patched `pasteToUrl()` / `pasteFilesFromClipboard()` does, and points it at a REAL
running Ultracopier:

  * a plain local `cp`      -> the driver must report ACCEPTED, and the file must really be copied
                               (a false "refused" would make every file manager copy twice);
  * a `cp` with an sftp URL -> the driver must report REFUSED, so the file manager falls back.

It therefore guards both halves of the contract, and it cannot drift from what is shipped: the
patch file is the single source of truth for the code under test. Skipped (clean PASS) only when
no Qt is available to build the driver."""
import sys, os, re, pathlib, shutil, subprocess, tempfile

_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K
from lib import uclaunch as U

_FM_DIR = pathlib.Path(__file__).resolve().parents[5] / "file-manager"
_PATCHES = ["dolphin-0002-add-ultracopier-advanced-copier-support-to-paste.patch",
            "libfm-qt-uc.patch", "pcmanfm-qt-uc-0.14.patch",
            "pcmanfm-qt-uc-0.16.patch", "pcmanfm-qt-uc-1.12.patch"]

_FUNC = re.compile(r'^(?:static )?(?:void|bool) '
                   r'(?:sendRawOrderList[A-Za-z]*|ultracopierRefusedOrder[A-Za-z]*)\(.*?^\}$',
                   re.S | re.M)

_DRIVER = r"""
#include <QCoreApplication>
#include <QLocalSocket>
#include <QStringList>
#include <iostream>

/* The paste sequence of the patched file manager, verbatim: connect, announce the protocol, send
 * the order, then ask whether Ultracopier took it. Prints ACCEPTED or REFUSED. */
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc < 5) {
        std::cout << "usage: driver <socket> <cp|mv> <source> <destination>" << std::endl;
        return 3;
    }
    QLocalSocket socket;
    socket.connectToServer(QString::fromUtf8(argv[1]));
    socket.waitForConnected(10000);
    if (socket.state() != QLocalSocket::ConnectedState) {
        std::cout << "NOCONNECT" << std::endl;
        return 3;
    }
    SENDFUNC(QStringList() << QStringLiteral("protocol") << QStringLiteral("0002"), socket, 1);
    socket.waitForReadyRead(10000);
    socket.readAll();
    QStringList l;
    l << QString::fromUtf8(argv[2]) << QString::fromUtf8(argv[3]) << QString::fromUtf8(argv[4]);
    SENDFUNC(l, socket, 2);
    socket.waitForBytesWritten(10000);
    const bool refused = VERDICTFUNC(socket, 2);
    socket.close();
    std::cout << (refused ? "REFUSED" : "ACCEPTED") << std::endl;
    return refused ? 1 : 0;
}
"""


def _new_side(patch_text: str) -> str:
    """The post-patch content of every hunk (context + added lines): what the file manager will
    really compile, even where the diff turned one of our added lines into context."""
    out = []
    in_hunk = False
    for line in patch_text.split("\n"):
        if line.startswith("@@"):
            in_hunk = True
            continue
        if line.startswith("diff ") or line.startswith("--- ") or line.startswith("+++ "):
            in_hunk = False
            continue
        if not in_hunk:
            continue
        if line.startswith("+"):
            out.append(line[1:])
        elif line.startswith(" ") or line == "":
            out.append(line[1:] if line else "")
        # '-' lines are gone after the patch; '\' markers are not content
    return "\n".join(out)


def _extract(patch: pathlib.Path):
    """(functions source, send function name, verdict function name) or (None, ...) if absent."""
    text = _new_side(patch.read_text())
    funcs = _FUNC.findall(text)
    if not funcs:
        return None, None, None
    names = [re.match(r'^(?:static )?(?:void|bool) (\w+)\(', f).group(1) for f in funcs]
    send = next((n for n in names if n.startswith("sendRawOrderList")), None)
    verdict = next((n for n in names if n.startswith("ultracopierRefusedOrder")), None)
    if send is None or verdict is None:
        return None, send, verdict
    # keep one send/verdict pair (the patches define an identical pair per patched file)
    keep = [f for f, n in zip(funcs, names) if n in (send, verdict)]
    return "\n\n".join(keep), send, verdict


def _build(patch: pathlib.Path, workdir: pathlib.Path):
    src, send, verdict = _extract(patch)
    if src is None:
        return None, f"no send/verdict helper found (send={send} verdict={verdict})"
    cpp = workdir / (patch.stem.replace("-", "_").replace(".", "_") + ".cpp")
    cpp.write_text("#include <QStringList>\n#include <QByteArray>\n#include <QDataStream>\n"
                   "#include <QLocalSocket>\n#include <QString>\n\n" + src + "\n"
                   + _DRIVER.replace("SENDFUNC", send).replace("VERDICTFUNC", verdict))
    out = workdir / (cpp.stem + "_driver")
    cflags = subprocess.run(["pkg-config", "--cflags", "Qt6Network", "Qt6Core"],
                            capture_output=True, text=True)
    libs = subprocess.run(["pkg-config", "--libs", "Qt6Network", "Qt6Core"],
                          capture_output=True, text=True)
    if cflags.returncode != 0 or libs.returncode != 0:
        return None, "Qt6 not found by pkg-config"
    cmd = (["g++", "-std=c++17", "-fPIC"] + cflags.stdout.split() + [str(cpp), "-o", str(out)]
           + libs.stdout.split())
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        return None, f"build failed:\n{r.stderr[-1500:]}"
    return out, ""


def run(backends=None, memcheck=H.NONE) -> bool:
    if memcheck == H.TSAN:
        # The FULL ultracopier binary cannot run under ThreadSanitizer: it dies within seconds with
        # "CHECK failed: sanitizer_thread_registry.cpp:186" (QtGui/offscreen + QThreadPool pooled
        # threads exit in a way TSan's registry cannot track -- a Qt-vs-TSan incompatibility
        # OUTSIDE our code; re-confirmed 2026-09-09, exit 66 on a plain local copy). These cases
        # DRIVE that binary, so there is nothing to run here. The engine's real TSan gate is
        # cases/tsan_engine_api.py, which hosts the same engine threads without QtGui.
        print("    [file_manager_fallback] SKIP under TSan (the full app dies in Qt's threadpool under TSan; the\n"
              "        thread gate is cases/tsan_engine_api.py)")
        return True
    bes = [b for b in (backends or [H.ASYNC]) if b in (H.ASYNC, H.IO_URING)]
    if not bes:
        print("    [file_manager_fallback] SKIP (async/io_uring only)")
        return True
    backend = H.ASYNC if H.ASYNC in bes else bes[0]
    if shutil.which("pkg-config") is None or shutil.which("g++") is None:
        print("    [file_manager_fallback] SKIP (no g++/pkg-config to build the patch code)")
        return True

    H._kill_all_ultracopier()
    root = pathlib.Path(K.fresh_dest("fm_fallback"))
    workdir = pathlib.Path(tempfile.mkdtemp(prefix="uc-fmfallback-"))
    ok = True
    rows = []
    try:
        drivers = {}
        for name in _PATCHES:
            patch = _FM_DIR / name
            if not patch.exists():
                rows.append((name, False, "patch file missing"))
                ok = False
                continue
            binp, why = _build(patch, workdir)
            if binp is None:
                rows.append((name, False, why))
                ok = False
            else:
                drivers[name] = binp

        with U.Instance("fmfb", backend=backend, memcheck=memcheck, cwd=workdir) as inst:
            if not inst.wait_listening("catchcopy", 60):
                print("    [file_manager_fallback] FAIL: catchcopy socket never came up")
                return False
            sock = inst.socket_path("catchcopy")
            for name, binp in drivers.items():
                src_dir = root / name.split("-")[0]
                src_dir.mkdir(parents=True, exist_ok=True)
                good = src_dir / "good.txt"
                good.write_bytes(b"file-manager-fallback\n")
                dest = root / (name.split("-")[0] + "_dest")
                shutil.rmtree(dest, ignore_errors=True)
                dest.mkdir(parents=True)

                r = subprocess.run([str(binp), sock, "cp", str(good), str(dest)],
                                   capture_output=True, text=True, timeout=120)
                accepted = r.stdout.strip() == "ACCEPTED"
                copied = inst.wait_until(lambda: (dest / "good.txt").exists(), 60)
                rows.append((f"{name}: a local paste is ACCEPTED and transferred",
                             accepted and copied, f"driver={r.stdout.strip()} copied={copied}"))
                ok = ok and accepted and copied

                r = subprocess.run([str(binp), sock, "cp", "sftp://root@127.0.0.1/XXX", str(dest)],
                                   capture_output=True, text=True, timeout=120)
                refused = r.stdout.strip() == "REFUSED"
                rows.append((f"{name}: an sftp:// paste is REFUSED (the file manager falls back)",
                             refused, f"driver={r.stdout.strip()}"))
                ok = ok and refused

            no_junk = not any(p.name.startswith("sftp:") for p in workdir.iterdir())
            rows.append(("no junk local folder was created by the refused pastes", no_junk,
                         f"workdir={sorted(p.name for p in workdir.iterdir())}"))
            ok = ok and no_junk
            rows.append(("the instance survived every paste", inst.alive(), ""))
            ok = ok and inst.alive()
    finally:
        H._kill_all_ultracopier()
        shutil.rmtree(workdir, ignore_errors=True)
        shutil.rmtree(root, ignore_errors=True)

    for name, good, detail in rows:
        print(f"      [{'PASS' if good else 'FAIL'}] {name}{('  -- ' + detail) if detail else ''}")
    print(f"    [file_manager_fallback] {'PASS' if ok else 'FAIL'}  ({len(rows)} checks on {backend})")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
