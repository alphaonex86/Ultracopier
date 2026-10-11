#!/usr/bin/env python3
"""IOCP (Windows laptop): an OVERWRITE whose source turns out unreadable must leave the user's
pre-existing destination untouched -- data-safety guard #9, the io_uring twin of
cases/iouring_overwrite_keeps_old_on_fault.py.

TransferThreadWin::openDestFile() opened every non-resume destination with CREATE_ALWAYS, which
truncates it to 0 at open; a read fault at byte 0 then skipped the file and the 0-byte wreck
stayed -- the previous good copy gone (the async-Windows path did the same). Now a PRE-EXISTING
destination is opened without truncation (OPEN_ALWAYS), the new content overwrites it in place and
it is cut to the new size only once the whole copy SUCCEEDED (operator decision 2026-10-06: data
safety over the CREATE_ALWAYS shortcut).

Rig (winlane): the fs_hook.dll `readfail` verb makes every ReadFile of the *UNREADABLE* source fail;
dest_pre_seed lays the OLD destination file down first; fileCollision=OVERWRITE, fileError=Skip,
1 inode thread. post_verify hashes the destination on the box: the victim must still be the OLD
bytes, the healthy sibling byte-identical to its source. Skips cleanly without the laptop."""
import sys, os, pathlib, shutil, hashlib, tempfile
_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K, winlane

VICTIM = "dead_UNREADABLE.bin"
FINE = "fine.bin"
OLD = b"the previous good copy: must survive an unreadable source under OVERWRITE\r\n" * 100


def _box_hash(box, path):
    out = box.ps("if (Test-Path -LiteralPath '" + path + "') { "
                 "(Get-FileHash -LiteralPath '" + path + "' -Algorithm SHA256).Hash.ToLower() } else { 'ABSENT' }")
    return next((l.strip().lower() for l in out.stdout.splitlines()
                 if l.strip().lower() == "absent" or len(l.strip()) == 64), "?")


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IOCP not in backends:
        print("    [skip] iocp_overwrite_keeps_old_on_fault is IOCP-only (Windows lane)")
        return True
    if not K.windows_host_configured():
        print("    [skip] no [windows] host configured")
        return True
    src = pathlib.Path(K.fresh_src_root("iocp_ovrkeep_src"))
    shutil.rmtree(src, ignore_errors=True); src.mkdir(parents=True)
    (src / VICTIM).write_bytes(synthtree._seeded_bytes_fast(3, 300000))
    (src / FINE).write_bytes(synthtree._seeded_bytes_fast(4, 200000))
    pre_dir = tempfile.mkdtemp(prefix="uc-iocp-ovrkeep-pre-")
    pathlib.Path(pre_dir, VICTIM).write_bytes(OLD)
    want_old = hashlib.sha256(OLD).hexdigest()
    want_fine = hashlib.sha256((src / FINE).read_bytes()).hexdigest()

    def pv(box, dest, srcs):
        cop = winlane.win_join(dest, src.name)
        got_old = _box_hash(box, winlane.win_join(cop, VICTIM))
        got_fine = _box_hash(box, winlane.win_join(cop, FINE))
        problems = []
        if got_old != want_old:
            problems.append(f"victim: pre-existing destination NOT kept ({got_old[:12]} != OLD)")
        if got_fine != want_fine:
            problems.append(f"sibling {FINE}: {got_fine[:12]} != source")
        return (not problems), ("; ".join(problems) if problems else "old destination kept, sibling byte-perfect")

    try:
        r = winlane.run_windows("cp", [str(src)],
                                file_collision=H.FileCollision.OVERWRITE,
                                folder_collision=H.FolderCollision.MERGE,
                                file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                                extra_options={"inodeThreads": "1"},
                                dest_pre_seed=pre_dir, post_verify=pv, fs_scenario="readfail:UNREADABLE")
    finally:
        shutil.rmtree(pre_dir, ignore_errors=True)
    ok = r.ok
    print(f"    [iocp overwrite keeps old on fault] {'PASS' if ok else 'FAIL'}  completed={r.completed} "
          f"alive={r.stayed_alive} crashes={r.mem_errors} {('notes=' + r.notes[-160:]) if not ok else ''}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
