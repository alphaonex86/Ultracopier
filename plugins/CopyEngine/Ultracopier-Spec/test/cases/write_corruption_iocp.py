#!/usr/bin/env python3
"""IOCP write-corruption: prove #25's checksum-verify catches+removes a corrupt destination on REAL
Windows/IOCP -- the analogue of iouring_write_corruption.py (which proved it on io_uring via the writable
FUSE) and write_corruption_async.py (async via the LD_PRELOAD `wflip` shim).

The Windows fault-injection seam is fs_hook.dll (UC_FS_SCENARIO=wflip:<substr>:<offset> -> a silent 1-byte
XOR-0xFF flip in the matching dest's IOCP WriteFile, returning the full byte count so the engine believes
the write succeeded). It is injected into the freshly-spawned ultracopier by uc_inject.exe via winlane
(authorized DLL injection; test-only, never in the shipping exe).

Two runs against the laptop (option key is "checksum" + "checksumOnlyOnError", NOT transferChecksum):
  * checksum OFF -> the flip lands in the dest victim and is NOT detected: the dest is CORRUPT. This
    VALIDATES THE SEAM (the hook actually flips a byte on the IOCP data plane). If this run shows the
    victim intact, the seam (injection/timing) is wrong -- fix the seam, not the engine.
  * checksum ON  -> the engine re-reads the dest, xxh3 mismatches the source, and #25 REMOVES the corrupt
    victim. The good siblings are byte-perfect in BOTH runs; no crash/WER.

Skipped cleanly (ok=True) when [windows] host is empty in config.ini (no laptop configured)."""
import os
import sys
import shutil
import hashlib
import pathlib

_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import winlane
from lib import casekit as K

VIC_BYTE = 0xAB
FLIP_OFFSET = 524288               # 512 KiB into the 1 MiB victim -> a mid-file IOCP write
VIC_SIZE = 1024 * 1024
GOOD_A = b"AAAA" * 8192            # 32 KiB
GOOD_B = b"BBBB" * 4096           # 16 KiB
_HASH_A = hashlib.sha256(GOOD_A).hexdigest()
_HASH_B = hashlib.sha256(GOOD_B).hexdigest()


# "0000_lead.bin" sorts FIRST so, with inodeThreads=1 (serial copy in scan order), the engine spends
# ~half a second copying it BEFORE it reaches victim.dat -- long enough for the fs_hook.dll injection
# (spawn -> CreateRemoteThread(LoadLibraryW) -> installHooks, a few hundred ms) to be in place. Without
# it the 1 MiB victim's mid-file write at FLIP_OFFSET happens in the first ~100 ms and races the
# injection, so the wflip never lands (the byte is never flipped -> false "SEAM WRONG").
LEAD_NAME = "0000_lead.bin"
LEAD_SIZE = 48 * 1024 * 1024


def _make_src():
    src = pathlib.Path(K.fresh_src_root("wcorrupt_iocp_src"))
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    (src / LEAD_NAME).write_bytes(bytes((i * 37) & 0xFF for i in range(LEAD_SIZE)))
    (src / "victim.dat").write_bytes(bytes([VIC_BYTE]) * VIC_SIZE)
    (src / "good_a.dat").write_bytes(GOOD_A)
    (src / "good_b.dat").write_bytes(GOOD_B)
    return src


def _verify(box, dest, srcs, *, expect_victim):
    """post_verify (runs ON THE BOX before cleanup). expect_victim='corrupt' (present + the byte at
    FLIP_OFFSET flipped) or 'absent' (removed by #25). Always also requires the good siblings byte-perfect.
    Returns (ok, note)."""
    base = os.path.basename(str(srcs[0]).replace("\\", "/"))
    j = winlane._win_join
    vic, ga, gb = j(dest, base, "victim.dat"), j(dest, base, "good_a.dat"), j(dest, base, "good_b.dat")
    ps = (
        f"$v='{vic}'; "
        f"if (Test-Path -LiteralPath $v) {{ $b=[System.IO.File]::ReadAllBytes($v); "
        f"  Write-Output ('VIC=present'); Write-Output ('LEN='+$b.Length); "
        f"  if($b.Length -gt {FLIP_OFFSET}){{ Write-Output ('BYTE='+$b[{FLIP_OFFSET}]) }} }} "
        f"else {{ Write-Output 'VIC=absent' }}; "
        f"foreach($f in @('{ga}','{gb}')){{ if(Test-Path -LiteralPath $f){{ "
        f"  Write-Output ('GOOD='+(Get-FileHash -LiteralPath $f -Algorithm SHA256).Hash.ToLower()) }} "
        f"  else {{ Write-Output 'GOOD=missing' }} }}"
    )
    out = box.ps(ps)
    vic_state, vic_byte, goods = None, None, []
    for ln in out.stdout.splitlines():
        ln = ln.strip()
        if ln.startswith("VIC="):
            vic_state = ln[4:]
        elif ln.startswith("BYTE="):
            try: vic_byte = int(ln[5:])
            except ValueError: vic_byte = None
        elif ln.startswith("GOOD="):
            goods.append(ln[5:])

    siblings_ok = (goods == [_HASH_A, _HASH_B])
    flipped = VIC_BYTE ^ 0xFF
    if expect_victim == "corrupt":
        seam_ok = (vic_state == "present" and vic_byte == flipped)
        note = f"seam: victim={vic_state} byte={vic_byte}(want {flipped}) siblings_ok={siblings_ok}"
    else:  # 'absent'
        seam_ok = (vic_state == "absent")
        note = (f"#25: victim={vic_state}(want absent) byte={vic_byte}(corrupt={flipped}/orig={VIC_BYTE}) "
                f"siblings_ok={siblings_ok}")
    return (seam_ok and siblings_ok, note)


def _run(ck, expect_victim, mem):
    src = _make_src()
    return winlane.run_windows(
        "cp", [str(src)],
        file_collision=H.FileCollision.OVERWRITE,
        folder_collision=H.FolderCollision.MERGE,
        extra_options={"checksum": ck, "checksumOnlyOnError": "false", "inodeThreads": "1"},
        fs_scenario=f"wflip:victim:{FLIP_OFFSET}",
        force_local_source=True,   # MUST copy OUR synthetic victim, not config's real SOURCEWINDOWS tree
        post_verify=lambda box, dest, srcs: _verify(box, dest, srcs, expect_victim=expect_victim),
    )


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IOCP not in backends:
        print("    [skip] write_corruption_iocp is IOCP-only")
        return True
    rOFF = _run("false", "corrupt", memcheck)   # validate the seam: the flip must land (dest corrupt)
    if rOFF.skipped:
        print(f"    [skip] {rOFF.notes}")
        return True
    rON = _run("true", "absent", memcheck)       # validate #25: the corrupt dest must be removed

    seam_ok = rOFF.content_ok and rOFF.completed and rOFF.stayed_alive and rOFF.mem_errors == 0
    fix_ok = rON.content_ok and rON.completed and rON.stayed_alive and rON.mem_errors == 0
    ok = seam_ok and fix_ok
    print(f"      SEAM(checksum OFF): {rOFF.notes} | completed={rOFF.completed} alive={rOFF.stayed_alive} crashes={rOFF.mem_errors}")
    print(f"      #25 (checksum ON):  {rON.notes} | completed={rON.completed} alive={rON.stayed_alive} crashes={rON.mem_errors}")
    if not seam_ok:
        print("        SEAM WRONG: the wflip did NOT corrupt the IOCP dest (injection/timing) -- fix the test seam, not the engine")
    if seam_ok and not fix_ok:
        print("        ENGINE: #25 did NOT remove the checksum-corrupt dest on IOCP")
    print(f"    [write_corruption_iocp] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run(backends={H.IOCP}) else 1)
