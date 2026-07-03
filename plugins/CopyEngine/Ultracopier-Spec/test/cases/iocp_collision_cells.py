#!/usr/bin/env python3
"""IOCP collision cells that never fired on real Windows before (coverage-audit gap 9):

  1. SKIP(1) with a REAL pre-seeded collision  -> the stale dest bytes survive, siblings copy
     (iocp_parity's sub_skip has no dest_pre_seed, so its Skip never actually triggered);
  2. RENAME(6) with a real collision           -> original keeps stale bytes, exactly one NEW
     sibling appears holding the SOURCE bytes (the \\?\-path retarget class that already
     produced one IOCP-only crash: trailing-dot rename);
  3. MOVE + OVERWRITE collision                -> dest replaced with source bytes AND the
     source tree is removed (per-file delete after a collision-overwrite on Windows);
  4. CASE-INSENSITIVE collision (NTFS)         -> source `CaseFile.TXT` vs pre-seeded
     `casefile.txt` MUST be treated as a collision (SKIP keeps the stale bytes, and no second
     case-variant file appears -- a blind case-sensitive exists() check would duplicate or
     clobber).

All bytes are role-distinct (collision_file_modes discipline) so no assertion can pass for
the wrong reason. Self-skips (PASS) when no [windows] host is configured; REAL on the laptop.
"""
import sys, pathlib, os, hashlib, shutil
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

STALE = b"IOCP-STALE-pre-seed-bytes-MUST-SURVIVE-skip-and-rename\r\n"
SRCB = b"IOCP-SOURCE-bytes-land-only-on-overwrite-or-rename-copy\r\n"
SIB = b"IOCP-sibling-must-always-copy\r\n"


def _sha(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def _mk(root: str, files: dict) -> str:
    shutil.rmtree(root, ignore_errors=True)
    for rel, data in files.items():
        K.write_file(os.path.join(root, rel), data)
    return root


def _cfg():
    """ALWAYS stage our synthetic trees (like iocp_parity). Without this, a configured
    [paths] SOURCEWINDOWS silently REPLACES our [src] argument in run_windows -- every
    assertion here is then against the wrong tree, and worse, sub-case 3 is a MOVE: it
    would move (= per-file DELETE) the user's real SOURCEWINDOWS directory. That is
    exactly what destroyed C:\\source (incl. the deployed test runtime) on 2026-07-02."""
    cfg = H.load_config()
    cfg.set("paths", "SOURCEWINDOWS", "")
    return cfg


def _hash_on_box(box, path: str) -> str:
    out = box.ps("if (Test-Path -LiteralPath '" + path + "') { "
                 "(Get-FileHash -LiteralPath '" + path + "' -Algorithm SHA256).Hash.ToLower() "
                 "} else { 'ABSENT' }")
    for ln in out.stdout.splitlines():
        ln = ln.strip().lower()
        if ln == "absent" or len(ln) == 64:
            return ln
    return "?"


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IOCP not in backends:
        print("    [skip] iocp_collision_cells is IOCP-only (drives the Windows laptop)")
        return True
    if not K.windows_host_configured():
        print("    [iocp_collision_cells] SKIP (no Windows host configured)")
        return True
    from lib import winlane
    ok = True

    # ---- 1. SKIP with a real collision ----
    src = _mk(K.fresh_src_root("icc_skip_src"), {"victim.txt": SRCB, "sib.txt": SIB})
    seed = _mk(K.fresh_src_root("icc_skip_seed"), {"victim.txt": STALE})

    def pv_skip(box, dest, srcs):
        cop = winlane.win_join(dest, os.path.basename(src))
        v = _hash_on_box(box, winlane.win_join(cop, "victim.txt"))
        s = _hash_on_box(box, winlane.win_join(cop, "sib.txt"))
        if v != _sha(STALE):
            return False, f"SKIP overwrote the victim (hash {v[:12]} != stale) -- data loss class"
        if s != _sha(SIB):
            return False, "sibling did not copy alongside the skipped victim"
        return True, "skip kept stale bytes, sibling copied"

    r = winlane.run_windows("cp", [src], cfg=_cfg(), file_collision=H.FileCollision.SKIP,
                            folder_collision=H.FolderCollision.MERGE,
                            dest_pre_seed=seed, post_verify=pv_skip)
    print(f"    [iocp skip-real-collision ] {'PASS' if r.ok else 'FAIL'}  {r.notes[-120:] if not r.ok else ''}")
    ok = ok and r.ok

    # ---- 2. RENAME with a real collision ----
    src = _mk(K.fresh_src_root("icc_ren_src"), {"victim.txt": SRCB, "sib.txt": SIB})
    seed = _mk(K.fresh_src_root("icc_ren_seed"), {"victim.txt": STALE})

    def pv_ren(box, dest, srcs):
        cop = winlane.win_join(dest, os.path.basename(src))
        if _hash_on_box(box, winlane.win_join(cop, "victim.txt")) != _sha(STALE):
            return False, "original victim no longer stale (rename clobbered it)"
        # discover the renamed file: any OTHER .txt in the dir whose hash == source bytes
        out = box.ps("Get-ChildItem -LiteralPath '" + cop + "' -File | "
                     "ForEach-Object { $_.Name + '|' + (Get-FileHash -LiteralPath $_.FullName "
                     "-Algorithm SHA256).Hash.ToLower() }")
        renamed = [l for l in out.stdout.splitlines()
                   if "|" in l and l.split("|")[1] == _sha(SRCB)
                   and l.split("|")[0].lower() != "victim.txt"]
        if len(renamed) != 1:
            return False, f"expected exactly 1 renamed copy holding source bytes, got {len(renamed)}"
        return True, f"renamed copy: {renamed[0].split('|')[0]}"

    r = winlane.run_windows("cp", [src], cfg=_cfg(), file_collision=H.FileCollision.RENAME,
                            folder_collision=H.FolderCollision.MERGE,
                            dest_pre_seed=seed, post_verify=pv_ren)
    print(f"    [iocp rename-real-collision] {'PASS' if r.ok else 'FAIL'}  {r.notes[-120:] if not r.ok else ''}")
    ok = ok and r.ok

    # ---- 3. MOVE + OVERWRITE collision ----
    src = _mk(K.fresh_src_root("icc_mv_src"), {"victim.txt": SRCB, "sib.txt": SIB})
    seed = _mk(K.fresh_src_root("icc_mv_seed"), {"victim.txt": STALE})

    def pv_mv(box, dest, srcs):
        cop = winlane.win_join(dest, os.path.basename(src))
        if _hash_on_box(box, winlane.win_join(cop, "victim.txt")) != _sha(SRCB):
            return False, "move+overwrite did not land the source bytes"
        if _hash_on_box(box, winlane.win_join(cop, "sib.txt")) != _sha(SIB):
            return False, "sibling not moved"
        # the STAGED source tree on the box must be gone (per-file delete + bottom-up rmdir)
        out = box.ps("if (Test-Path -LiteralPath '" + srcs[0] + "') { 'PRESENT' } else { 'GONE' }")
        if "GONE" not in out.stdout:
            return False, "source tree still present after a completed move"
        return True, "collision-overwrite moved + source removed"

    r = winlane.run_windows("mv", [src], cfg=_cfg(), file_collision=H.FileCollision.OVERWRITE,
                            folder_collision=H.FolderCollision.MERGE,
                            dest_pre_seed=seed, post_verify=pv_mv)
    print(f"    [iocp move-overwrite-coll  ] {'PASS' if r.ok else 'FAIL'}  {r.notes[-120:] if not r.ok else ''}")
    ok = ok and r.ok

    # ---- 4. CASE-INSENSITIVE collision (NTFS) ----
    src = _mk(K.fresh_src_root("icc_ci_src"), {"CaseFile.TXT": SRCB, "sib.txt": SIB})
    seed = _mk(K.fresh_src_root("icc_ci_seed"), {"casefile.txt": STALE})

    def pv_ci(box, dest, srcs):
        cop = winlane.win_join(dest, os.path.basename(src))
        out = box.ps("Get-ChildItem -LiteralPath '" + cop + "' -File | Where-Object "
                     "{ $_.Name -ieq 'casefile.txt' } | ForEach-Object { $_.Name + '|' + "
                     "(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower() }")
        variants = [l.strip() for l in out.stdout.splitlines() if "|" in l]
        if len(variants) != 1:
            return False, f"expected exactly 1 case-variant of casefile.txt, got {len(variants)}: {variants}"
        if variants[0].split("|")[1] != _sha(STALE):
            return False, "case-insensitive collision NOT honored: stale bytes clobbered by SKIP"
        return True, f"single case-variant kept stale bytes ({variants[0].split('|')[0]})"

    r = winlane.run_windows("cp", [src], cfg=_cfg(), file_collision=H.FileCollision.SKIP,
                            folder_collision=H.FolderCollision.MERGE,
                            dest_pre_seed=seed, post_verify=pv_ci)
    print(f"    [iocp case-insensitive-coll] {'PASS' if r.ok else 'FAIL'}  {r.notes[-120:] if not r.ok else ''}")
    ok = ok and r.ok

    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
