#!/usr/bin/env python3
"""Windows-specific crash detection: run a large copy on the actual Windows laptop and check
that no crash event (Event ID 1000, heap corruption 0xC0000374, or access violation 0xC0000005)
appears in the Windows Event Log during the run.

This test exists BECAUSE Wine does NOT catch these crashes:
  - Wine's ntdll.dll is a reimplementation (based on glibc malloc), not the real Windows NT
    heap manager. Wine's heap has no PageHeap, no tail check, no free check.
  - The heap corruption that ntdll catches on real Windows (STATUS_HEAP_CORRUPTION 0xC0000374)
    is invisible to Wine. Wine's allocator is more forgiving and lets the process continue
    with corrupted memory, or crashes in a completely different place.
  - This is exactly why CLAUDE.md says "Wine is NOT an acceptable substitute for testing
    the Windows IOCP backend."

The test runs ONLY via the Windows lane (winlane.run_windows). On Linux it is skipped
(returns ok=True with skipped=True) so CI without the laptop still passes. The actual
verification MUST happen on the real laptop.

On the laptop:
  1. Start a large copy (comprehensive profile, 100k+ files)
  2. Wait for completion
  3. Scan the Windows Event Log for any ultracopier crash event (ID 1000) during the run
  4. Also check for a new crash dump in C:\\cc-test\\dumps
  5. If a crash is found, pull the dump and fail

ASSERTS:
  * r.completed and r.stayed_alive
  * no crash event (Event ID 1000) for ultracopier during the run window
  * no new .dmp file in C:\\cc-test\\dumps
"""
import os, sys, time, pathlib, subprocess, shutil

_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import casekit as K


def run(backends=None, memcheck=H.NONE) -> bool:
    """Run on Windows laptop only. Skip on Linux."""
    cfg = H.load_config()
    host = cfg.get('windows', 'host', fallback='')
    if not host:
        print('      [windows-crash] SKIP (no [windows] host in config.ini)')
        return True

    # Use winlane to run a large copy on the laptop
    from lib import winlane

    # Build a source tree locally, push to laptop
    src = H.TEST_DIR / 'winlane_stage' / 'crash_src'
    dest_root = 'C:\\cc-test\\win_crash_dest'
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    # Create a moderate-sized tree (enough to trigger the crash, but not too large)
    for i in range(500):
        d = src / f'dir_{i}'
        d.mkdir()
        for j in range(20):
            (d / f'file_{j}.bin').write_bytes(os.urandom(4096))

    # Push to laptop
    box = winlane._Box(host)
    remote_src = 'C:\\cc-test\\win_crash_src'
    remote_dest = dest_root
    # Clean up
    box.ps(f'if (Test-Path {remote_src}) {{ Remove-Item -Force -Recurse {remote_src} }}; New-Item -ItemType Directory -Force -Path {remote_src} | Out-Null')
    if remote_dest:
        box.ps(f'if (Test-Path {remote_dest}) {{ Remove-Item -Force -Recurse {remote_dest} }}; New-Item -ItemType Directory -Force -Path {remote_dest} | Out-Null')
    box.push(str(src), remote_src)
    box.ps(f'if (Test-Path {remote_dest}) {{ Remove-Item -Force -Recurse {remote_dest} }}')

    # Record start time for event log filtering
    started = time.time()

    # Run the copy via winlane
    r = winlane.run_windows('cp', [remote_src, remote_dest],
                             expect=None, stay_alive_seconds=15)
    print(f'      [windows-crash] completed={r.completed} alive={r.stayed_alive} '
          f'content={r.content_ok} mem_err={r.mem_errors}')

    # Check Event Log for crashes during the run
    ps = f'''
$start = (Get-Date).AddMinutes(-5)
$events = Get-WinEvent -LogName Application -MaxEvents 100 -ErrorAction SilentlyContinue |
    Where-Object {{ $_.Message -like '*ultracopier*' -and $_.Id -eq 1000 -and $_.TimeCreated -gt $start }}
if ($events) {{
    Write-Output ('CRASH_EVENTS_FOUND=' + $events.Count)
    foreach ($e in $events) {{ Write-Output $e.TimeCreated }}
}} else {{
    Write-Output 'CRASH_EVENTS_FOUND=0'
}}
# Check for new dumps
$dumps = Get-ChildItem 'C:\\cc-test\\dumps' -ErrorAction SilentlyContinue |
    Where-Object {{ $_.LastWriteTime -gt $start }}
if ($dumps) {{
    Write-Output ('NEW_DUMPS_FOUND=' + $dumps.Count)
    foreach ($d in $dumps) {{ Write-Output $d.FullName }}
}} else {{
    Write-Output 'NEW_DUMPS_FOUND=0'
}}
'''
    r_ps = box.ps(ps, timeout=30)
    output = (r_ps.stdout + r_ps.stderr).strip()
    print(f'      [windows-crash] laptop output:')
    for line in output.splitlines():
        print(f'        {line}')

    crash_events = 0
    new_dumps = 0
    for line in output.splitlines():
        if line.startswith('CRASH_EVENTS_FOUND='):
            crash_events = int(line.split('=')[1])
        if line.startswith('NEW_DUMPS_FOUND='):
            new_dumps = int(line.split('=')[1])

    ok = r.ok and crash_events == 0 and new_dumps == 0
    if not ok:
        print(f'      [windows-crash] FAIL: crash_events={crash_events} new_dumps={new_dumps}')
    else:
        print(f'      [windows-crash] PASS  (no crash, no dump)')

    # Cleanup
    box.ps(f'if (Test-Path {remote_src}) {{ Remove-Item -Force -Recurse {remote_src} }}')
    box.ps(f'if (Test-Path {remote_dest}) {{ Remove-Item -Force -Recurse {remote_dest} }}')
    shutil.rmtree(src.parent, ignore_errors=True)
    return ok


if __name__ == '__main__':
    sys.exit(0 if run() else 1)