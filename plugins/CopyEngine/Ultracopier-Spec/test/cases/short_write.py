#!/usr/bin/env python3
"""Partial-write accounting: every write() returns HALF the requested count -- the engine's
write loop must re-issue the remainder and the destination must end byte-identical.

The `shortwrite` verb is implemented in BOTH shims (fs_preload.c write(): writes count/2 REAL
bytes, returns count/2; fs_hook.cpp WriteFile) but was used by NO case. The DATA-SAFETY
invariant it guards: a short write must NEVER produce a dest that is FULL-SIZE but wrong-bytes
while the engine reports the file complete -- the classic silent-corruption class (a loop
that mis-advances the offset on a short write leaves a hole/shift; POSIX permits short writes
on NFS/FUSE/pipes, which Ultracopier does copy to).

This case asserts that INVARIANT, NOT a specific recovery policy: for the halved-write victim
the dest must be EITHER byte-identical to the source OR short/absent (the file not claimed
complete) -- never full-size-but-different. The async backend meets it by treating a short
write as a hard error and applying fileError (here Skip) -> the victim is left SHORT (a
deliberate, safe choice: local FS never short-writes; NFS short-writes surface as an error,
no corruption). A backend that instead advanced the offset wrong would leave a full-size
corrupt file and FAIL here.
  * async (libc write() data plane) -- the shim engages;
  * IOCP (fs_hook WriteFile, via winlane when a [windows] host is configured);
io_uring's data plane bypasses libc -- not faultable by this verb (short-completion handling
is exercised by the real-ENOSPC + FUSE cases).
"""
import sys, pathlib, os
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

VICTIM = "victim_multichunk.bin"
SIZE = 3 * 1024 * 1024 + 12345          # multi-chunk, deliberately unaligned


def _pattern(n: int) -> bytes:
    """Position-dependent bytes: data[i] = (i*2654435761) & 0xFF -- any shift/hole mismatches."""
    return bytes(((i * 2654435761) >> 3) & 0xFF for i in range(n))


def _make_src(name: str) -> str:
    src = K.fresh_src_root(name)
    K.write_file(os.path.join(src, VICTIM), _pattern(SIZE))
    K.write_file(os.path.join(src, "small_a.txt"), b"sibling A\n" * 16)
    K.write_file(os.path.join(src, "sub", "small_b.txt"), b"sibling B nested\n" * 8)
    return src


def run(backends=None, memcheck=H.NONE) -> bool:

    def one(backend):
        if backend != H.ASYNC:
            print(f"    [{backend}] SKIP (shortwrite is a libc write() interpose; the "
                  f"io_uring data plane bypasses libc -- covered by ENOSPC/FUSE cases)")
            return True
        src = _make_src("shortw_src")
        dest = K.fresh_dest("shortw_dest")
        K.with_scenario(f"shortwrite:{VICTIM}")
        # expect_dir=None: content-diff is NOT the gate (a safe skip legitimately leaves the
        # victim short); the corruption invariant is checked by hand below.
        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.SKIP,
                  expect_dir=None, fs_preload=K.fs_so(), memcheck=memcheck)
        K.with_scenario("")
        src_bytes = K.read_file(os.path.join(src, VICTIM))
        vd = os.path.join(dest, os.path.basename(src), VICTIM)
        problems = []
        if not (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0):
            problems.append(f"run: completed={r.completed} alive={r.stayed_alive} "
                            f"oom={r.oom_killed} mem_err={r.mem_errors}")
        # The no-silent-corruption invariant: byte-identical, OR short/absent -- never full-size-wrong.
        if os.path.exists(vd):
            vb = K.read_file(vd)
            if len(vb) == len(src_bytes) and vb != src_bytes:
                problems.append("victim is FULL-SIZE but WRONG bytes -- SILENT SHORT-WRITE CORRUPTION")
            elif len(vb) == len(src_bytes) and vb == src_bytes:
                print(f"      [{backend}] victim fully re-issued + byte-identical")
            else:
                print(f"      [{backend}] victim left SHORT ({len(vb)}/{len(src_bytes)}) "
                      f"-- error+skip, no corruption (safe)")
        else:
            print(f"      [{backend}] victim absent -- skipped, no corruption (safe)")
        # Small siblings (never faulted) must always be perfect.
        copied = os.path.join(dest, os.path.basename(src))
        for rel in ("small_a.txt", os.path.join("sub", "small_b.txt")):
            p = os.path.join(copied, rel)
            if not os.path.exists(p) or K.read_file(p) != K.read_file(os.path.join(src, rel)):
                problems.append(f"unfaulted sibling {rel} missing/wrong")
        if problems:
            print(f"      [{backend}] FAIL: " + "; ".join(problems))
        return not problems

    def iocp_one():
        from lib import winlane
        if not K.windows_host_configured():
            print("    [iocp] SKIP (no Windows host configured)")
            return True
        import hashlib
        src = _make_src("shortw_iocp_src")
        want = hashlib.sha256(K.read_file(os.path.join(src, VICTIM))).hexdigest()
        want_size = len(K.read_file(os.path.join(src, VICTIM)))

        def pv(box, dest, srcs):
            # Invariant (not a policy): byte-identical OR short/absent -- never full-size-wrong.
            copied = winlane.win_join(dest, os.path.basename(src))
            p = winlane.win_join(copied, VICTIM)
            out = box.ps("if (Test-Path -LiteralPath '" + p + "') { "
                         "$h=(Get-FileHash -LiteralPath '" + p + "' -Algorithm SHA256).Hash.ToLower(); "
                         "$s=(Get-Item -LiteralPath '" + p + "').Length; "
                         "Write-Output ('H='+$h); Write-Output ('S='+$s) } else { Write-Output 'ABSENT' }")
            got, size = "", -1
            for ln in out.stdout.splitlines():
                ln = ln.strip()
                if ln.startswith("H="):
                    got = ln[2:]
                elif ln.startswith("S="):
                    size = int(ln[2:])
                elif ln == "ABSENT":
                    return True, "victim absent -- skipped, no corruption"
            if got == want:
                return True, "victim byte-identical under halved WriteFile"
            if size == want_size and got != want:
                return False, f"victim FULL-SIZE ({size}) but wrong hash -- SILENT SHORT-WRITE CORRUPTION"
            return True, f"victim short ({size}/{want_size}) -- error+skip, no corruption"

        r = winlane.run_windows("cp", [src],
                                file_collision=H.FileCollision.OVERWRITE,
                                folder_collision=H.FolderCollision.MERGE,
                                keep_date=True, do_right=True,
                                expect=None, post_verify=pv,
                                fs_scenario=f"shortwrite:{VICTIM}")
        if not r.ok:
            print(f"      [iocp] FAIL: completed={r.completed} content={r.content_ok} "
                  f"crashes={r.mem_errors} notes={r.notes}")
        return r.ok

    return K.for_option_backends(backends, one, iocp_one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
