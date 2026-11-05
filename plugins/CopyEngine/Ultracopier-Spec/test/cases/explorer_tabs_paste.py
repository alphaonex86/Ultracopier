#!/usr/bin/env python3
"""Ctrl+V in a Windows 11 Explorer TAB must paste into THAT tab (user reports, 3.1).

All tabs of a Windows 11 Explorer window share one top-level HWND; the PluginLoader keyboard hook
(plugins/PluginLoader/catchcopy/pluginLoader.cpp pasteDestFolder) took the first IShellWindows entry
with that HWND, so the paste always landed in the FIRST tab. The fix matches the active tab
(ShellTabWindowClass) against IShellBrowser::GetWindow.

Runs on the Windows 11 VM lane (lib/win11vm.py): the real shipping exe in the real interactive
session, real Explorer tabs, injected Ctrl+V. Pass = for tabs 3, 2 and 1 the file lands ONLY in the
active tab AND Ultracopier's transfer log shows it did the copy (an uncaught Ctrl+V would be Explorer's
own paste). Runs in every all.py run (any --backend) when [win11vm] dir is set, else prints SKIP.
Red run: UC_WIN11_SOURCES=<pre-fix source export>.
"""
import sys, pathlib, json, time, tempfile, shutil
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import winlane, win11vm

WORK = r"C:\uc-tabs"
TASK = "uc_explorer_tabs_paste"
DRIVER = _TEST_DIR / "win11vm" / "explorer_tabs_paste.ps1"
# forward slashes: QSettings treats "\" as an escape in INI values
CONF = """[Ultracopier]
checkTheUpdate=false
displayOSSpecific2=false
[Write_log]
enabled=true
file=C:/uc-tabs/transfer.log
transfer=true
sync=true
"""


def _run_driver(b, exe: str) -> dict:
    winlane._scp_file(b, str(DRIVER), f"{WORK}\\explorer_tabs_paste.ps1")
    args = f"-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File {WORK}\\explorer_tabs_paste.ps1 -Exe {exe} -Work {WORK}"
    r = b.ps(f"Remove-Item -Force -ErrorAction SilentlyContinue -LiteralPath '{WORK}\\result.json'; "
             f"$a = New-ScheduledTaskAction -Execute 'powershell.exe' -Argument '{args}'; "
             "$p = New-ScheduledTaskPrincipal -UserId 'uc' -LogonType Interactive -RunLevel Limited; "
             f"Register-ScheduledTask -TaskName '{TASK}' -Action $a -Principal $p -Force | Out-Null; "
             f"Start-ScheduledTask -TaskName '{TASK}'")
    if r.returncode != 0:
        raise RuntimeError(f"starting the interactive task failed: {(r.stdout + r.stderr).strip()}")
    try:
        deadline = time.monotonic() + 300
        while time.monotonic() < deadline:
            out = b.ps(f"if (Test-Path '{WORK}\\result.json') {{ Get-Content -Raw '{WORK}\\result.json' }}").stdout.strip()
            if out:
                return json.loads(out.lstrip("\ufeff"))
            time.sleep(3)
        return {"status": "error", "detail": "no result.json after 300s", "cases": []}
    finally:
        b.ps(f"Unregister-ScheduledTask -TaskName '{TASK}' -Confirm:$false -ErrorAction SilentlyContinue; "
             "Get-Process ultracopier -ErrorAction SilentlyContinue | Stop-Process -Force")


def run(backends=None, memcheck=H.NONE) -> bool:
    # backend-independent (it tests the PluginLoader, not a copy backend): runs in every suite run with a VM
    vdir = win11vm.vm_dir(H.load_config())
    if not vdir:
        print("    [win11] SKIP ([win11vm] dir empty in the test config -> Windows 11 VM lane disabled)")
        return True
    app = win11vm.stage(win11vm.build_exe())
    (app / "Ultracopier.conf").write_text(CONF)
    booted = win11vm.ensure_up(vdir)
    try:
        b = win11vm.box(vdir)
        exe = win11vm.deploy(b, app, WORK)
        res = _run_driver(b, exe)
        for c in res.get("cases", []):
            print(f"      tab {c['tab']:<8} landed_in={c['landed_in']} by_ultracopier={c['by_ultracopier']} "
                  f"-> {'OK' if c['ok'] else 'WRONG'}")
        if res.get("status") != "pass":
            shot = tempfile.mkstemp(prefix="uc-win11-explorer-tabs-", suffix=".png")[1]
            win11vm.screenshot(vdir, shot)
            print(f"    [win11] {res.get('status')}: {res.get('detail', '')}  (guest screen: {shot})")
        return res.get("status") == "pass"
    finally:
        shutil.rmtree(app.parent, ignore_errors=True)
        if booted:
            win11vm.shutdown(vdir)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
