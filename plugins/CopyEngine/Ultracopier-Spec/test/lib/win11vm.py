#!/usr/bin/env python3
"""The Windows 11 VM lane: GUI-session tests (Explorer, the PluginLoader keyboard hook) on a local
QEMU guest built by test/win11vm/vm.sh -- no physical box, nothing to deploy by hand.

Config (~/.config/ultracopier-spec-test.ini):  [win11vm] dir = <VM data dir>   (empty -> lane skipped)
The VM is booted headless when down and shut down again afterwards; ultracopier.exe is cross-built
with MXE Qt6 from the current tree (or from $UC_WIN11_SOURCES, e.g. an older export for a red run).
"""
from __future__ import annotations
import os, pathlib, re, shutil, subprocess, tempfile, time

from lib import harness as H
from lib import winlane

VM_SH = H.TEST_DIR / "win11vm" / "vm.sh"
SSH_PORT = 2222
MXE = pathlib.Path("/mnt/data/perso/progs/mxe/x86_64/usr")
MXE_TARGET = MXE / "x86_64-w64-mingw32.shared"
MXE_QMAKE = MXE_TARGET / "qt6" / "bin" / "qmake6"
DLL_DIRS = [MXE_TARGET / "qt6" / "bin", MXE_TARGET / "bin"]


def vm_dir(cfg) -> str:
    return cfg.get("win11vm", "dir", fallback="").strip()


def _vm_sh(vdir: str, *args, timeout=600) -> subprocess.CompletedProcess:
    env = dict(os.environ, UC_WIN11_DIR=vdir, UC_WIN11_SSH_PORT=str(SSH_PORT))
    return subprocess.run([str(VM_SH), *args], env=env, capture_output=True, text=True, timeout=timeout)


def box(vdir: str) -> winlane._Box:
    return winlane._Box("uc@127.0.0.1", ["-i", f"{vdir}/ssh/id_ed25519", "-o", f"Port={SSH_PORT}",
                                         "-o", f"UserKnownHostsFile={vdir}/ssh/known_hosts"])


def _wait(pred, seconds: int, what: str):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if pred():
            return
        time.sleep(5)
    raise RuntimeError(f"win11 VM: timeout after {seconds}s waiting for {what}")


def _ssh_ok(b) -> bool:
    try:
        return "UP" in b.ssh_raw("echo UP", timeout=30).stdout
    except subprocess.TimeoutExpired:
        return False


def ensure_up(vdir: str) -> bool:
    """Boot the VM if needed and wait for ssh + the auto-logged-on desktop. Returns True if WE booted it."""
    b = box(vdir)
    if _ssh_ok(b):
        return False
    r = _vm_sh(vdir, "start", timeout=120)
    if r.returncode != 0:
        raise RuntimeError(f"vm.sh start failed: {(r.stdout + r.stderr).strip()}")
    _wait(lambda: _ssh_ok(b), 600, "ssh")
    _wait(lambda: "explorer.exe" in b.ssh_raw('tasklist /FI "IMAGENAME eq explorer.exe" /NH', timeout=30).stdout.lower(),
          300, "the auto-logged-on desktop (explorer.exe)")
    time.sleep(20)   # let the shell finish its logon startup before driving it
    return True


def shutdown(vdir: str) -> None:
    r = _vm_sh(vdir, "stop", timeout=300)
    if r.returncode != 0:
        print(f"    [win11] WARNING: vm.sh stop failed: {(r.stdout + r.stderr).strip()}")


def screenshot(vdir: str, out: str) -> None:
    r = _vm_sh(vdir, "shot", out, timeout=60)
    if r.returncode != 0:
        print(f"    [win11] screenshot failed: {(r.stdout + r.stderr).strip()}")


def build_exe() -> pathlib.Path:
    """Cross-build the all-in-one ultracopier.exe (release, nodebug) with MXE Qt6."""
    sources = pathlib.Path(os.environ.get("UC_WIN11_SOURCES") or H.SOURCES)
    tag = "" if sources == H.SOURCES else "-" + re.sub(r"[^A-Za-z0-9]+", "_", str(sources))[-40:]
    bdir = H.SOURCES / "build" / f"test-win11-mxe{tag}"
    env = dict(os.environ)
    env["PATH"] = f"{MXE_QMAKE.parent}{os.pathsep}{MXE / 'bin'}{os.pathsep}{env.get('PATH', '')}"
    H.qmake_build(bdir, sources / "ultracopier.pro", ["CONFIG+=release", "CONFIG+=nodebug"], env,
                  qmake=str(MXE_QMAKE), spec="win32-g++")
    exe = bdir / "release" / "ultracopier.exe"
    if not exe.exists():
        raise RuntimeError(f"MXE build produced no {exe}")
    return exe


def _imports(pe: pathlib.Path) -> list[str]:
    r = subprocess.run([str(MXE / "bin" / "x86_64-w64-mingw32.shared-objdump"), "-p", str(pe)],
                       capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(f"objdump {pe} failed: {r.stderr.strip()}")
    return re.findall(r"DLL Name: (\S+)", r.stdout)


def stage(exe: pathlib.Path) -> pathlib.Path:
    """exe + every MXE DLL it needs (recursive) + the qwindows platform plugin, in a fresh 'app' dir."""
    app = pathlib.Path(tempfile.mkdtemp(prefix="uc-win11-stage-")) / "app"
    (app / "platforms").mkdir(parents=True)
    shutil.copy2(exe, app / exe.name)
    shutil.copy2(MXE_TARGET / "qt6" / "plugins" / "platforms" / "qwindows.dll", app / "platforms" / "qwindows.dll")
    todo = [app / exe.name, app / "platforms" / "qwindows.dll"]
    seen = set()
    while todo:
        for dll in _imports(todo.pop()):
            key = dll.lower()
            if key not in seen:
                seen.add(key)
                found = next((d / dll for d in DLL_DIRS if (d / dll).exists()), None)
                if found is not None:   # not in MXE = a Windows system DLL
                    shutil.copy2(found, app / dll)
                    todo.append(app / dll)
    return app


def deploy(b: winlane._Box, app: pathlib.Path, remote_root: str) -> str:
    """Replace remote_root\\app with the staged dir; returns the remote exe path."""
    r = b.ps("Get-Process ultracopier -ErrorAction SilentlyContinue | Stop-Process -Force; Start-Sleep 2; "
             f"if (Test-Path '{remote_root}\\app') {{ Remove-Item -Recurse -Force -LiteralPath '{remote_root}\\app' }}; "
             f"New-Item -ItemType Directory -Force -Path '{remote_root}' | Out-Null")
    if r.returncode != 0:
        raise RuntimeError(f"cleaning {remote_root}\\app failed: {(r.stdout + r.stderr).strip()}")
    b.push(str(app), remote_root)
    return f"{remote_root}\\app\\ultracopier.exe"
