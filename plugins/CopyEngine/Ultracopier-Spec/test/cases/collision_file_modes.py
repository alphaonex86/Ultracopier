#!/usr/bin/env python3
"""fileCollision conditional-overwrite modes 3,4,5,7,8 -- predicate HONORED, both directions.

The engine's per-file collision predicate lives in TransferThread::destinationExists()
(TransferThread.cpp ~383-540). For each conditional mode it compares the SOURCE file
against the PRE-EXISTING destination file and decides OVERWRITE (do the transfer -> dest
ends up with the SOURCE bytes) or KEEP (skip the transfer -> dest keeps its STALE bytes):

  idx 4  OverwriteIfNewer            overwrite IFF src_mtime  >  dst_mtime   (TT.cpp:383,409)
  idx 5  OverwriteIfOlder            overwrite IFF src_mtime  <  dst_mtime   (TT.cpp:420,446)
  idx 8  OverwriteIfNotSameMdate     overwrite IFF src_mtime  != dst_mtime   (TT.cpp:457,483)
  idx 7  OverwriteIfNotSameSize      overwrite IFF size(src)  != size(dst)   (TT.cpp:494,496)
  idx 3  OverwriteIfNotSameSizeAndDate
                                     overwrite IFF mtime differ OR size differ (TT.cpp:507,531)
                                     i.e. KEEP only when BOTH size AND mtime match.

NB the headless conf value is the QComboBox INDEX (../CLAUDE.md). The runtime consumer is
CopyEngine::setFileCollision(int) (CopyEngine.cpp:812-853), whose switch maps 3,4,5,7,8 to
the right FileExists_* enum -- the `default:` arm collapses an UNKNOWN index to
FileExists_NotSet (== "Ask"), which under autoStart would block forever on the collision
dialog (no dialog is shown headless) -> the copy never reaches idle -> r.completed==False.

So for EVERY index we assert BOTH directions diverge observably and unambiguously:
  * an OVERWRITE-victim file whose size/mtime relationship makes the predicate TRUE  -> dest
    must end with the SOURCE bytes;
  * a KEEP-victim file whose relationship makes the predicate FALSE                  -> dest
    must end with the STALE pre-seeded bytes.
Both victims carry DIFFERENT content from each other and from their seeds, so neither
assertion could pass for the wrong reason: "overwrite" cannot be satisfied by a file that
would already be there, and "keep" cannot be satisfied by an overwrite (stale != source).

VALIDATOR REGRESSION (idx 7 and 8). CopyEngineFactory::setFileCollision(int)
(CopyEngineFactory.cpp:578-592) -- the UI combo-change slot -- still has a switch that only
persists indices 0..6 and DROPS 7/8 (logs "unknow index, ignored"), even though the UI offers
8 entries and the engine consumes 8. The headless conf write bypasses that slot, so 7/8 reach
the engine; this case proves the ENGINE honors 7 and 8 (not clamped to Ask, not silently
turned into plain Overwrite/Skip) by requiring the exact size-only (7) and mdate-only (8)
divergence below. If a future "fix" routed the headless value back through the clamping slot,
or the engine switch lost 7/8, the keep-direction would wrongly overwrite (or the run would
hang on Ask) and this case fails.

Runs on async + io_uring (full predicate) AND IOCP (winlane: pre-seed on the box, post_verify
the dest bytes). The predicate is fully cross-platform (NTFS mtime via FILETIME->seconds in
readFileMDateTime, size via file_stat_size), so IOCP is wired, not skipped; it self-skips with
PASS only when no [windows] host is configured.
"""
import sys, pathlib, os
# Strip cases/ from sys.path so stdlib imports are not shadowed by a cases/*.py of the same
# name when this file is run directly (mirrors opt_perms_dates.py / windows_ads.py).
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

# Two well-separated second-granularity mtimes (engine compares st_mtim.tv_sec; NTFS is
# FILETIME/100ns truncated to seconds). > the harness MTIME_TOL_S so they never alias.
T_OLD = 1000000000   # 2001-09-09 01:46:40 UTC
T_NEW = 1500000000   # 2017-07-14 02:40:00 UTC

# Distinct content per role so an assertion can never pass for the wrong reason.
SRC_OVW = b"SRC-OVERWRITE-VICTIM-content-must-land-in-destination\n"   # source of overwrite file
SEED_OVW = b"STALE-overwrite-victim-pre-seed-MUST-BE-REPLACED\n"        # dest pre-seed (overwrite)
SRC_KEEP = b"SRC-keep-victim-content-MUST-NOT-reach-destination\n"      # source of keep file
SEED_KEEP = b"STALE-keep-victim-pre-seed-MUST-SURVIVE-unchanged!!\n"    # dest pre-seed (keep)

OVW = "ovw.dat"     # the OVERWRITE-direction victim (predicate TRUE  -> dest := source bytes)
KEEP = "keep.dat"   # the KEEP-direction victim      (predicate FALSE -> dest stays stale bytes)


# Per-index recipe. Each entry gives, for the two victims, the (source_mtime, source_pad,
# seed_mtime, seed_pad) needed to make the predicate fire the intended way. `*_pad` adjusts the
# file SIZE: padding is appended so SIZE relations are controllable independently of content.
# Sizes: base content length + pad. We choose pads so size relations are exactly as the predicate
# under test requires, while keeping the *content* always distinct (source != seed).
#
# Convention per victim: (src_mtime, src_extra_bytes, seed_mtime, seed_extra_bytes)
RECIPES = {
    # idx 4 OverwriteIfNewer: overwrite IFF src newer (src_mtime > dst_mtime). Size irrelevant
    # (predicate is mtime-only) -> keep sizes equal so only mtime drives the decision.
    4: {
        OVW:  (T_NEW, 0, T_OLD, 0),   # src newer  -> overwrite
        KEEP: (T_OLD, 0, T_NEW, 0),   # src older  -> keep (src not strictly newer)
    },
    # idx 5 OverwriteIfOlder: overwrite IFF src older (src_mtime < dst_mtime).
    5: {
        OVW:  (T_OLD, 0, T_NEW, 0),   # src older  -> overwrite
        KEEP: (T_NEW, 0, T_OLD, 0),   # src newer  -> keep (src not older)
    },
    # idx 8 OverwriteIfNotSameMdate (date-only): overwrite IFF mtimes differ. Sizes equal so
    # only mtime can drive it -> proves it is date-only, distinct from idx 7 (size-only).
    8: {
        OVW:  (T_NEW, 0, T_OLD, 0),   # mtimes differ -> overwrite
        KEEP: (T_OLD, 0, T_OLD, 0),   # mtimes equal  -> keep
    },
    # idx 7 OverwriteIfNotSameSize (size-only): overwrite IFF sizes differ. mtimes EQUAL so
    # only size can drive it -> proves it is size-only, distinct from idx 8 (date-only).
    7: {
        OVW:  (T_OLD, 32, T_OLD, 0),  # sizes differ (src 32B bigger) -> overwrite
        KEEP: (T_OLD, 0, T_OLD, 0),   # sizes equal (content still differs) -> keep
    },
    # idx 3 OverwriteIfNotSameSizeAndDate: overwrite IFF (mtime differs) OR (size differs);
    # KEEP only when BOTH match. OVW case: same size but mtime differs (so the OR fires via the
    # date leg, proving it is not pure size-only). KEEP case: same size AND same mtime.
    3: {
        OVW:  (T_NEW, 0, T_OLD, 0),   # same size, mtime differs -> overwrite
        KEEP: (T_OLD, 0, T_OLD, 0),   # same size AND same mtime -> keep
    },
}

# The seed content lengths must equal the source content lengths in the "same size" sub-cases.
# SRC_OVW vs SEED_OVW and SRC_KEEP vs SEED_KEEP differ in bytes; pad the SHORTER to match so a
# "same size" requirement is honest (size equal, content different). Computed once below.
def _sized(base: bytes, total_len: int) -> bytes:
    """Return `base` padded with trailing NULs to exactly total_len bytes (base must fit)."""
    assert len(base) <= total_len, (len(base), total_len)
    return base + b"\0" * (total_len - len(base))


def _build_index_tree(idx: int, src_root: str, seed_root: str, expect_root: str):
    """Lay down, for one collision index, a 2-file source, the colliding dest pre-seed, and the
    expected FINAL dest state. Returns nothing; writes to the three given roots.

    For each victim we equalise the OVW pair sizes and the KEEP pair sizes to whatever the recipe
    asks (via *_extra), keeping content distinct. The expected final dest is:
      OVW  -> the SOURCE bytes (predicate true => overwrite)
      KEEP -> the SEED   bytes (predicate false => keep)
    """
    import shutil
    for r in (src_root, seed_root, expect_root):
        if os.path.exists(r):
            shutil.rmtree(r)
        os.makedirs(r, exist_ok=True)

    rec = RECIPES[idx]
    for name, src_base, seed_base in ((OVW, SRC_OVW, SEED_OVW), (KEEP, SRC_KEEP, SEED_KEEP)):
        src_mtime, src_extra, seed_mtime, seed_extra = rec[name]
        # Target sizes: make the *intended* size relation exact. We define each side's length as
        # max(content)+extra so the requested delta is honoured; identical extra => equal sizes.
        base_len = max(len(src_base), len(seed_base))
        src_len = base_len + src_extra
        seed_len = base_len + seed_extra
        src_bytes = _sized(src_base, src_len)
        seed_bytes = _sized(seed_base, seed_len)

        sp = os.path.join(src_root, name)
        with open(sp, "wb") as fh:
            fh.write(src_bytes)
        os.utime(sp, (src_mtime, src_mtime))

        dp = os.path.join(seed_root, name)
        with open(dp, "wb") as fh:
            fh.write(seed_bytes)
        os.utime(dp, (seed_mtime, seed_mtime))

        # Expected final dest content for this victim.
        ep = os.path.join(expect_root, name)
        with open(ep, "wb") as fh:
            fh.write(src_bytes if name == OVW else seed_bytes)


def run(backends=None, memcheck=H.NONE) -> bool:

    def one(backend):
        ok = True
        for idx in sorted(RECIPES):
            src = K.fresh_src_root(f"cfm_{idx}_src")
            seed = K.fresh_src_root(f"cfm_{idx}_seed")
            expect = K.fresh_src_root(f"cfm_{idx}_expect")
            _build_index_tree(idx, src, seed, expect)
            base = os.path.basename(src)

            dest = K.fresh_dest(f"cfm_{idx}_dest")
            # Pre-seed the colliding files at dest/<base>/<name> (where ultracopier lands them).
            for name in (OVW, KEEP):
                K.write_file(os.path.join(dest, base, name), b"")  # ensure dir
            import shutil
            for name in (OVW, KEEP):
                sp = os.path.join(seed, name)
                dp = os.path.join(dest, base, name)
                shutil.copyfile(sp, dp)
                st = os.stat(sp)
                os.utime(dp, (st.st_mtime, st.st_mtime))

            # DEFAULT thread count (do NOT force inode_threads=1): the collision predicate is
            # per-file, so ordering is irrelevant -- but inode_threads=1 triggers a SEPARATE,
            # known single-thread pump-stall in the async backend (CopyEngineFactory.cpp:153
            # "one transfer thread makes the put-to-end retry stall-prone ... nothing re-pumps"):
            # once a file resolves to SKIP, the lone thread goes Idle and the next file is never
            # re-pumped, so later files are silently DROPPED. That stall is unrelated to the
            # collision predicate under test (and is a real inode_threads=1 data-loss finding,
            # see project_inode1_skip_pump_stall) -- using the default thread count keeps this
            # case focused on the predicate, where every backend behaves correctly.
            r = H.run(backend, "cp", [src], dest,
                      file_collision=idx,
                      folder_collision=H.FolderCollision.MERGE,
                      expect_dir=expect, memcheck=memcheck)

            copied = K.copied_root(dest, src)
            ovw_path = os.path.join(copied, OVW)
            keep_path = os.path.join(copied, KEEP)
            # Read the FINAL dest content and check both directions explicitly (independent of
            # the diff so the failure message pinpoints which direction broke).
            ovw_final = K.read_file(ovw_path) if os.path.exists(ovw_path) else b""
            keep_final = K.read_file(keep_path) if os.path.exists(keep_path) else b""
            ovw_src = K.read_file(os.path.join(src, OVW))
            keep_seed = K.read_file(os.path.join(seed, KEEP))
            overwrote = (ovw_final == ovw_src)        # predicate TRUE  -> dest got source bytes
            kept = (keep_final == keep_seed)          # predicate FALSE -> dest kept stale bytes
            # `r.completed` False would mean the engine fell to Ask (unknown index) and hung;
            # assert it explicitly so a clamp-to-Ask regression is reported as such.
            sub_ok = (r.ok and r.content_ok and r.completed and overwrote and kept)
            if not sub_ok:
                print(f"      [idx {idx}] not ok: completed={r.completed} alive={r.stayed_alive} "
                      f"content={r.content_ok} mem_errors={r.mem_errors} "
                      f"overwrote={overwrote} kept={kept}\n{r.diff_text}")
            ok = ok and sub_ok
        return ok

    def iocp_one():
        """Same predicate matrix on IOCP via winlane: pre-seed the dest on the box, run each
        index once, and post_verify BOTH victims' final bytes ON THE BOX (Get-FileHash). The
        predicate is cross-platform (readFileMDateTime uses FILETIME->seconds; file_stat_size).
        Self-skips with PASS when no [windows] host is configured; REAL when one exists."""
        from lib import winlane
        if not K.windows_host_configured():
            print("    [iocp] SKIP (no Windows host configured)")
            return True

        import hashlib
        ok = True
        for idx in sorted(RECIPES):
            src = K.fresh_src_root(f"cfm_iocp_{idx}_src")
            seed = K.fresh_src_root(f"cfm_iocp_{idx}_seed")
            expect = K.fresh_src_root(f"cfm_iocp_{idx}_expect")
            _build_index_tree(idx, src, seed, expect)

            # Expected final hashes (computed locally from the expect tree the engine must produce).
            want_ovw = hashlib.sha256(K.read_file(os.path.join(expect, OVW))).hexdigest()
            want_keep = hashlib.sha256(K.read_file(os.path.join(expect, KEEP))).hexdigest()

            def pv(box, dest, srcs, _want_ovw=want_ovw, _want_keep=want_keep, _idx=idx):
                copied = winlane.win_join(dest, os.path.basename(src))
                problems = []
                for name, want in ((OVW, _want_ovw), (KEEP, _want_keep)):
                    p = winlane.win_join(copied, name)
                    out = box.ps("$h=(Get-FileHash -LiteralPath '\\\\?\\" + p +
                                 "' -Algorithm SHA256).Hash.ToLower(); Write-Output ('H='+$h)")
                    got = ""
                    for ln in out.stdout.splitlines():
                        ln = ln.strip()
                        if ln.startswith("H="):
                            got = ln[2:]
                    if got != want:
                        problems.append(f"idx {_idx} {name}: dest hash {got or '<missing>'} != want {want}")
                if problems:
                    return False, "; ".join(problems)
                return True, f"idx {_idx} overwrite+keep both correct"

            r = winlane.run_windows("cp", [src], file_collision=idx,
                                    folder_collision=H.FolderCollision.MERGE,
                                    keep_date=True, do_right=True,
                                    expect=expect, dest_pre_seed=seed, post_verify=pv)
            if not r.ok:
                print(f"      [iocp idx {idx}] FAIL: completed={r.completed} alive={r.stayed_alive} "
                      f"content={r.content_ok} crashes={r.mem_errors} notes={r.notes}")
            ok = ok and r.ok
        return ok

    return K.for_option_backends(backends, one, iocp_one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
