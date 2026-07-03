"""IOCP dying-disk salvage on the real Windows box (coverage-audit gap 16): the FLAGSHIP use
case -- backing up a failing drive -- had ZERO IOCP coverage because fs_hook.cpp could not fault
a read. It now supports readfail / eio_after (whole-file bad sector / bad sector partway in),
the Windows analogue of the Linux fs_preload.c verbs used by faulty_hdd.

A healthy tree with two BADSECTOR victims (one whole-file readfail, one eio_after partway) plus
several always-good files, copied with fileError=Skip. REQUIRED resilient behaviour (mirrors
faulty_hdd): the process stays alive and the job completes (no hang, no crash / heap corruption),
every READABLE file is byte-identical on the box, and the unreadable victims are skipped (absent
or short) -- never a full-size wrong-content file. peak RSS bounded, no WER Event-1000 crash.

Self-skips (PASS) when no [windows] host is configured; REAL on the laptop (with your standing
authorization to drive it). fs_hook.dll injection is the same authorized seam write_corruption_iocp
already uses.
"""
import sys, pathlib, os, hashlib, shutil
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

# "0000_lead.bin" sorts FIRST so, with inodeThreads=1 (serial copy in scan order), the engine
# spends ~half a second copying it BEFORE it reaches a BADSECTOR victim -- long enough for the
# fs_hook.dll injection (spawn -> Sleep -> CreateRemoteThread(LoadLibraryW) -> installHooks, a few
# hundred ms) to be fully in place. Without this lead + serial ordering the 2 MiB victims copy in
# the first ~100 ms and race the injection, so the read fault never lands (the file materialises
# whole). The lead is also a good file that must survive intact.
LEAD_GOOD = "0000_lead.bin"
LEAD_SIZE = 48 * 1024 * 1024
GOOD = {
    LEAD_GOOD: bytes((i * 37) & 0xFF for i in range(LEAD_SIZE)),
    "readme.txt": b"good file A -- must survive the salvage\r\n" * 50,
    os.path.join("docs", "note.txt"): b"nested good file\r\n" * 80,
    "big_good.bin": bytes((i * 37) & 0xFF for i in range(3 * 1024 * 1024)),   # multi-chunk good
}
# victims: names contain BADSECTOR so the scenario substring matches. "zz_" prefix sorts them AFTER
# the good files so the serial copy reaches them only after the injection window has closed.
VICTIM_WHOLE = "zz_BADSECTOR_whole.dat"
VICTIM_PARTIAL = "zz_BADSECTOR_partial.dat"


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IOCP not in backends:
        print("    [skip] iocp_faulty_hdd is IOCP-only (drives the Windows laptop)")
        return True
    if not K.windows_host_configured():
        print("    [iocp_faulty_hdd] SKIP (no Windows host configured)")
        return True
    from lib import winlane

    src = K.fresh_src_root("iofh_src")
    shutil.rmtree(src, ignore_errors=True)
    good_hash = {}
    for rel, data in GOOD.items():
        K.write_file(os.path.join(src, rel), data)
        good_hash[rel.replace(os.sep, "/")] = hashlib.sha256(data).hexdigest()
    K.write_file(os.path.join(src, VICTIM_WHOLE), bytes((i * 5) & 0xFF for i in range(2 * 1024 * 1024)))
    K.write_file(os.path.join(src, VICTIM_PARTIAL), bytes((i * 9) & 0xFF for i in range(2 * 1024 * 1024)))

    def make_pv(faulted_victim):
        """The scenario faults exactly ONE victim; the OTHER victim is a healthy file that MUST
        copy whole (asserting both are short was a false-positive -- each scenario faults one)."""
        def pv(box, dest, srcs):
            cop = winlane.win_join(dest, os.path.basename(src))
            problems = []
            # every good (readable) file must be byte-identical on the box
            healthy = dict(good_hash)
            other = VICTIM_PARTIAL if faulted_victim == VICTIM_WHOLE else VICTIM_WHOLE
            healthy[other] = hashlib.sha256(K.read_file(os.path.join(src, other))).hexdigest()
            for rel, want in healthy.items():
                p = winlane.win_join(cop, rel.replace("/", "\\"))
                out = box.ps("if (Test-Path -LiteralPath '" + p + "') { "
                             "(Get-FileHash -LiteralPath '" + p + "' -Algorithm SHA256).Hash.ToLower() "
                             "} else { 'ABSENT' }")
                got = next((l.strip().lower() for l in out.stdout.splitlines()
                            if l.strip().lower() == "absent" or len(l.strip()) == 64), "?")
                if got != want:
                    problems.append(f"good {rel}: {got[:12]} != want (readable file lost/corrupt)")
            # THIS scenario's unreadable victim: absent OR short, NEVER full-size (2 MiB)
            p = winlane.win_join(cop, faulted_victim)
            out = box.ps("if (Test-Path -LiteralPath '" + p + "') { (Get-Item -LiteralPath '" + p +
                         "').Length } else { Write-Output 'ABSENT' }")
            tok = next((l.strip() for l in out.stdout.splitlines() if l.strip()), "")
            if tok.isdigit() and int(tok) >= 2 * 1024 * 1024:
                problems.append(f"victim {faulted_victim} is FULL-SIZE ({tok}) -- unreadable file materialised whole "
                                "(read-fault injection did not reach the IOCP overlapped ReadFile)")
            if problems:
                return False, "; ".join(problems)
            return True, "readable files salvaged byte-perfect; unreadable victim skipped"
        return pv

    ok = True
    for label, scenario, victim in (
            ("readfail(whole)", "readfail:BADSECTOR_whole", VICTIM_WHOLE),
            ("eio_after(partial)", "eio_after:BADSECTOR_partial:262144", VICTIM_PARTIAL)):
        r = winlane.run_windows("cp", [src],
                                file_collision=H.FileCollision.OVERWRITE,
                                folder_collision=H.FolderCollision.MERGE,
                                file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                                extra_options={"inodeThreads": "1"},   # serial copy -> victim reached after injection
                                post_verify=make_pv(victim), fs_scenario=scenario)
        sub = r.ok
        print(f"    [iocp salvage {label:18s}] {'PASS' if sub else 'FAIL'}  "
              f"completed={r.completed} alive={r.stayed_alive} crashes={r.mem_errors} "
              f"{('notes=' + r.notes[-140:]) if not sub else ''}")
        ok = ok and sub
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
