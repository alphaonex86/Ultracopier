#!/usr/bin/env python3
"""IOCP verification of the #9 OVERWRITE source-vanish + the dialog/headless skip-close, on the real
Win10 laptop. Mirrors iouring_source_vanish (Linux) but drives the IOCP backend over SSH via winlane.

The shared TransferThreadPipelined::skip() unlink (gated this run with destinationIsOursToRemove()) is
what IOCP inherits, so this confirms the fix on real Windows. We set up entirely on the box:
  * a source folder: 3 large files (>128KB) -- one is the VICTIM -- + 2 small good files,
  * the source VICTIM's data is denied (icacls /deny (RD)) so its copy-open fails like a vanished
    source, while the directory scan still lists it (so the engine TRIES it -> the skip path),
  * the destination victim is pre-seeded with OLD content (the collision target that MUST survive),
  * conf = OVERWRITE + SKIP (headless; no blocking dialog over SSH).

REQUIRED: the dest victim STILL holds OLD (no data loss), the large file AFTER it is copied full-size
(the cap-1 slot was freed), the good files copy, and NO crash (WER Event 1000).

Skipped cleanly if [windows] host is empty in config.ini."""
import os
import sys
import time
import pathlib

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import winlane as W

VICTIM_OLD = "OLD-DESTINATION-CONTENT-THAT-MUST-SURVIVE-THE-UNREADABLE-SOURCE"
LARGE = 524288   # > parallelizeIfSmallerThan (128 KiB) -> uses the single large-transfer slot


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IOCP not in backends:
        print("    [skip] iocp_source_vanish is IOCP-only (drives the Windows laptop)")
        return True
    cfg = H.load_config()
    host = cfg.get("windows", "host", fallback="").strip()
    exe = cfg.get("windows", "exe", fallback="").strip()
    if not host or not exe:
        print("    [iocp_source_vanish] SKIPPED: [windows] host/exe empty in config.ini")
        return True

    box = W._Box(host)
    exe_dir = exe.rsplit("\\", 1)[0]
    dest_root = cfg.get("paths", "DESTINATIONWINDOWS", fallback=r"C:\cc-test\uc-auto").strip()
    base = W._win_join(dest_root, "uc-iocp-sv")
    src = W._win_join(base, "src")
    dst = W._win_join(base, "dst")
    dvic = W._win_join(dst, "src")            # ultracopier copies <src> -> <dst>\<basename(src)>=\src

    try:
        # (1) clean + dirs
        box.ps(f"Stop-Process -Name ultracopier -Force -EA SilentlyContinue; Start-Sleep -Milliseconds 800; "
               f"if(Test-Path '{base}'){{icacls '{base}' /reset /T /C /Q | Out-Null; "
               f"Remove-Item -LiteralPath '{base}' -Recurse -Force -EA SilentlyContinue}}; "
               f"New-Item -ItemType Directory -Force -Path '{src}','{dvic}' | Out-Null")
        # (2) source: 3 large + 2 small
        box.ps(f"fsutil file createnew '{src}\\a_good_large.bin' {LARGE} | Out-Null; "
               f"fsutil file createnew '{src}\\m_victim_large.bin' {LARGE} | Out-Null; "
               f"fsutil file createnew '{src}\\z_good_large.bin' {LARGE} | Out-Null; "
               f"Set-Content -LiteralPath '{src}\\good1.txt' -Value 'good-file-one' -NoNewline; "
               f"Set-Content -LiteralPath '{src}\\good2.txt' -Value 'good-file-two' -NoNewline")
        # (3) pre-seed the dest victim with OLD content (the collision target)
        box.ps(f"Set-Content -LiteralPath '{dvic}\\m_victim_large.bin' -Value '{VICTIM_OLD}' -NoNewline")
        # (4) deny READ_DATA on the source victim (copy-open fails; scan still lists it)
        box.ps(f"icacls '{src}\\m_victim_large.bin' /deny \"$($env:USERNAME):(RD)\" | Out-Null")
        # (5) conf next to the exe: OVERWRITE + SKIP (headless)
        W._put_text(box, W._win_join(exe_dir, "Ultracopier.conf"), W._conf_text(
            file_collision=H.FileCollision.OVERWRITE, folder_collision=H.FolderCollision.MERGE,
            file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP, keep_date=True, do_right=True))
        # (6) launch headless, detached (survives the SSH session)
        r = box.ps(W._launch_script(exe, "cp", [src], dst, "", "", 1024), timeout=60)
        kv = W._parse_kv(r.stdout)
        pid, start = kv.get("PID"), kv.get("START")
        if not pid:
            print(f"    [iocp_source_vanish] FAIL launch: {r.stdout.strip()[:300]}")
            return False
        # (7) wait for CPU-idle completion
        completed, peak, oom, exit_code = W._wait_idle(box, pid, 180, 1024)
        time.sleep(6)
        box.ps(f"Stop-Process -Id {pid} -Force -EA SilentlyContinue; "
               f"Stop-Process -Name ultracopier -Force -EA SilentlyContinue")
        crashes, crash_note = W._scan_crashes(box, start)
        # (8) inspect the result tree
        chk = box.ps(f"""
$d='{dvic}'
$v=Join-Path $d 'm_victim_large.bin'
if(Test-Path -LiteralPath $v){{ if((Get-Content -LiteralPath $v -Raw) -like '*{VICTIM_OLD}*'){{Write-Output 'VICTIM=PRESERVED'}}else{{Write-Output 'VICTIM=CORRUPT'}} }}else{{Write-Output 'VICTIM=DELETED'}}
$z=Join-Path $d 'z_good_large.bin'
if(Test-Path -LiteralPath $z){{ Write-Output ('Z='+(Get-Item -LiteralPath $z).Length) }}else{{Write-Output 'Z=MISSING'}}
foreach($f in 'a_good_large.bin','good1.txt','good2.txt'){{ if(Test-Path -LiteralPath (Join-Path $d $f)){{Write-Output ('G_'+$f+'=OK')}}else{{Write-Output ('G_'+$f+'=MISSING')}} }}
""")
        kv2 = {}
        for line in chk.stdout.splitlines():
            line = line.strip()
            if "=" in line:
                k, v = line.split("=", 1); kv2[k] = v
        victim = kv2.get("VICTIM", "?")
        zsz = kv2.get("Z", "?")
        good_ok = all(kv2.get("G_" + f) == "OK" for f in ("a_good_large.bin", "good1.txt", "good2.txt"))
        victim_ok = (victim == "PRESERVED")
        z_ok = (zsz == str(LARGE))
        ok = (completed and victim_ok and z_ok and good_ok and crashes == 0 and not oom)
        print(f"      completed={completed} victim={victim} z={zsz}(want {LARGE}) good_ok={good_ok} "
              f"crashes={crashes} oom={oom} peakMB={peak}")
        if crash_note:
            print(f"      {crash_note}")
        if not victim_ok:
            print(f"      *** #9 DATA LOSS on IOCP: dest victim {victim} (expected PRESERVED) ***")
        if not z_ok:
            print(f"      *** skip-close: z_good_large.bin {zsz} (expected {LARGE}) ***")
        print(f"    [iocp_source_vanish] {'PASS' if ok else 'FAIL'}")
        return ok
    finally:
        box.ps(f"Stop-Process -Name ultracopier -Force -EA SilentlyContinue; "
               f"if(Test-Path '{base}'){{icacls '{base}' /reset /T /C /Q | Out-Null; "
               f"Remove-Item -LiteralPath '{base}' -Recurse -Force -EA SilentlyContinue}}")


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
