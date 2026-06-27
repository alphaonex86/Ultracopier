#!/usr/bin/env python3
"""Windows Alternate Data Stream (ADS) handling.

NTFS allows multiple data streams per file (e.g., `file.txt:stream`). Ultracopier
may ignore them (copy only the default stream) or preserve them (if the engine
supports ADS). This test creates a file with an ADS, copies it, and verifies at
least the default stream is correct; the ADS may be missing (ignored) or present
(preserved) — either is acceptable as long as the copy does not crash.

Only runs on the Windows lane (IOCP backend). On Linux, the case is skipped.
"""
import sys, pathlib, os, subprocess, tempfile, shutil, uuid
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import casekit as K


def run(backends=None, memcheck=H.NONE) -> bool:
    # Only run on Windows lane (IOCP). If no Windows host configured, skip.
    cfg = H.load_config()
    if not cfg.has_section("windows") or not cfg.get("windows", "host"):
        print("    [windows] SKIP (no Windows host configured)")
        return True

    # Only IOCP backend; async/io_uring are Linux-only.
    if backends and H.IOCP not in backends:
        print(f"    [windows] SKIP (requested backends {backends} do not include IOCP)")
        return True

    # Create a temporary directory on the Windows host.
    from lib import winlane
    box = winlane._Box(cfg.get("windows", "host"))
    # We'll stage a source tree with a file containing an ADS.
    # Use PowerShell to create ADS: `echo ads_content > file.txt:stream`
    # We'll create a small source tree, copy it via ultracopier, then verify.
    # Since we cannot rely on the Windows host having a temp location, we'll use
    # the winlane staging mechanism: push a tarball.
    tmp_lines = box.ps("$env:TEMP").stdout.splitlines()
    if not tmp_lines:
        print("    [windows] SKIP (could not get TEMP)")
        return True
    tmp = tmp_lines[0].strip()
    src_name = f"ads_test_{uuid.uuid4().hex[:8]}"
    dest_name = f"ads_test_dest_{uuid.uuid4().hex[:8]}"
    src = f"{tmp}\\{src_name}"
    dest = f"{tmp}\\{dest_name}"
    # Clean up any leftovers.
    box.ps(f"Remove-Item -Recurse -Force -ErrorAction SilentlyContinue '{src}'; "
           f"Remove-Item -Recurse -Force -ErrorAction SilentlyContinue '{dest}'")
    # Create source directory with a normal file and a file with ADS.
    box.ps(f"New-Item -ItemType Directory -Path '{src}' | Out-Null")
    box.ps(f"Set-Content -Path '{src}\\normal.txt' -Value 'hello world'")
    # Create ADS: write to file:stream
    box.ps(f"Set-Content -Path '{src}\\with_ads.txt' -Value 'default stream'")
    box.ps(f"Set-Content -Path '{src}\\with_ads.txt:secret' -Value 'alternate data'")
    # Also create a subdirectory with a file that has ADS.
    box.ps(f"New-Item -ItemType Directory -Path '{src}\\sub' | Out-Null")
    box.ps(f"Set-Content -Path '{src}\\sub\\deep.txt' -Value 'deep default'")
    box.ps(f"Set-Content -Path '{src}\\sub\\deep.txt:extra' -Value 'deep extra'")

    # Run ultracopier copy via winlane.
    # We'll use winlane.run_windows directly.
    from lib.winlane import run_windows
    result = run_windows(
        mode="cp",
        sources_local=[src],
        cfg=cfg,
        file_collision=H.FileCollision.OVERWRITE,
        folder_collision=H.FolderCollision.MERGE,
        file_error=H.FileError.SKIP,
        folder_error=H.FolderError.SKIP,
        keep_date=True,
        do_right=True,
        # (run_windows has no expect= here; ADS correctness is verified manually below)
        mem_limit_mb=1024,
        stay_alive_seconds=10,
    )
    if not result.ok:
        print(f"    [iocp] FAIL: {result}")
        # Clean up
        box.ps(f"Remove-Item -Recurse -Force -ErrorAction SilentlyContinue '{src}'; "
               f"Remove-Item -Recurse -Force -ErrorAction SilentlyContinue '{dest}'")
        return False

    # Verify default streams are correct.
    ok = True
    # normal.txt
    out = box.ps(f"Get-Content -Raw -Path '{dest}\\normal.txt' -ErrorAction SilentlyContinue").stdout.splitlines()
    if not out or out[0].strip() != "hello world":
        print(f"      normal.txt mismatch: {out}")
        ok = False
    # with_ads.txt default stream
    out = box.ps(f"Get-Content -Raw -Path '{dest}\\with_ads.txt' -ErrorAction SilentlyContinue").stdout.splitlines()
    if not out or out[0].strip() != "default stream":
        print(f"      with_ads.txt default mismatch: {out}")
        ok = False
    # sub\deep.txt default stream
    out = box.ps(f"Get-Content -Raw -Path '{dest}\\sub\\deep.txt' -ErrorAction SilentlyContinue").stdout.splitlines()
    if not out or out[0].strip() != "deep default":
        print(f"      sub\\deep.txt default mismatch: {out}")
        ok = False

    # ADS may be missing; that's fine. If present, verify content.
    # Use `Get-Item -Stream *` to list streams.
    out = box.ps(f"(Get-Item '{dest}\\with_ads.txt' -Stream * | Select-Object -ExpandProperty Stream) 2>$null").stdout.splitlines()
    streams = [s.strip() for s in out if s.strip()]
    if "secret" in streams:
        out = box.ps(f"Get-Content -Raw -Path '{dest}\\with_ads.txt' -Stream secret -ErrorAction SilentlyContinue").stdout.splitlines()
        if out and out[0].strip() != "alternate data":
            print(f"      with_ads.txt:secret mismatch: {out}")
            ok = False
        else:
            print(f"      ADS 'secret' preserved")
    else:
        print(f"      ADS 'secret' not present (ignored)")

    out = box.ps(f"(Get-Item '{dest}\\sub\\deep.txt' -Stream * | Select-Object -ExpandProperty Stream) 2>$null").stdout.splitlines()
    streams = [s.strip() for s in out if s.strip()]
    if "extra" in streams:
        out = box.ps(f"Get-Content -Raw -Path '{dest}\\sub\\deep.txt' -Stream extra -ErrorAction SilentlyContinue").stdout.splitlines()
        if out and out[0].strip() != "deep extra":
            print(f"      sub\\deep.txt:extra mismatch: {out}")
            ok = False
        else:
            print(f"      ADS 'extra' preserved")
    else:
        print(f"      ADS 'extra' not present (ignored)")

    # Clean up
    box.ps(f"Remove-Item -Recurse -Force -ErrorAction SilentlyContinue '{src}'; "
           f"Remove-Item -Recurse -Force -ErrorAction SilentlyContinue '{dest}'")

    print(f"    [iocp] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)