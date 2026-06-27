#!/usr/bin/env python3
"""coalesceSourceStat: the engine caches ONE source stat per file and serves both the
date read and the permission read from it, instead of stat()ing the source twice.

Verified observable (see TransferThread.cpp statSourceCached :1058, readSourceFileDateTime
:1130, readSourceFilePermissions :1218 -- both share the `permissions` buffer; per-file
cache reset in setFiles :171). With doRightTransfer=ON the engine reads BOTH date and
permissions, so:
    coalesceSourceStat=ON  -> 1 source stat per regular file (date fills, perms cache-hits)
    coalesceSourceStat=OFF -> 2 source stats per regular file (each re-stats)
=> the TOTAL number of source stat() calls is strictly LOWER with the option on. That count
is the objective proof the cache is honored; the option does NOT change WHICH value is used
(the date always comes from the first transfer-phase stat), so a value diff cannot tell on
from off -- the stat COUNT can. We measure it with the LD_PRELOAD statlog accounting
(UC_FS_STATLOG_MATCH/PATH), which counts stat/lstat/statx of the source.

Three checks (async + io_uring; IOCP runs the copy on the box for correctness -- the
stat-count delta uses the Linux LD_PRELOAD accounting and is Linux-only, because IOCP
coalesces via CreateFile opens, not libc stat):
  1. HONORED   : stats(on) < stats(off)   (cache eliminates the second per-file stat)
  2. CORRECT   : content + perms + dates identical with the option on (coalescing must not
                 corrupt anything)
  3. ROBUST    : with the option on AND the source stat MUTATED after the cache fill
                 (statmut), the copy still completes with correct content and stays under
                 the memory cap -- the cached snapshot makes the engine immune to a
                 post-cache source change ("change real data while cached").

NOTE (tracked separately): the "source error flushes the cache but does NOT regenerate it
for that file" requirement needs tracing the put-to-end/retry -> setFiles path and is built
in a focused follow-up (it asserts a specific flush/no-regen count, not covered here).
"""
import sys, pathlib, os, tempfile
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import synthtree, casekit as K


def _count_source_stats(backend, src, dest, *, coalesce, memcheck, statmut=None):
    """Run one copy with statlog on the source; return (Result, copied_root, stat_count)."""
    so = K.fs_so()
    logf = tempfile.mktemp(prefix="uc-statlog-", suffix=".txt")
    if os.path.exists(logf):
        os.unlink(logf)
    os.environ["UC_FS_STATLOG_MATCH"] = os.path.basename(src)
    os.environ["UC_FS_STATLOG_PATH"] = logf
    os.environ["UC_FS_SCENARIO"] = (statmut or "")
    try:
        r = H.run(backend, "cp", [src], dest, do_right=True, keep_date=True,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=src, fs_preload=so, memcheck=memcheck,
                  extra_options={"coalesceSourceStat": "true" if coalesce else "false"})
    finally:
        for k in ("UC_FS_STATLOG_MATCH", "UC_FS_STATLOG_PATH", "UC_FS_SCENARIO"):
            os.environ.pop(k, None)
    count = None
    try:
        count = int(pathlib.Path(logf).read_text().strip())
    except (OSError, ValueError):
        count = None
    finally:
        if os.path.exists(logf):
            os.unlink(logf)
    return r, K.copied_root(dest, src), count


def run(backends=None, memcheck=H.NONE) -> bool:
    src = synthtree.make_tree(K.fresh_src_root("coalesce_src"), "default")

    def one(backend):
        ok = True
        # 1) HONORED: stats(on) < stats(off)
        r_on, copied_on, n_on = _count_source_stats(
            backend, src, K.fresh_dest("coalesce_on"), coalesce=True, memcheck=memcheck)
        r_off, copied_off, n_off = _count_source_stats(
            backend, src, K.fresh_dest("coalesce_off"), coalesce=False, memcheck=memcheck)
        if not (r_on.ok and r_on.content_ok):
            print(f"      [on] copy not ok: {r_on.notes}\n{r_on.diff_text}"); ok = False
        if not (r_off.ok and r_off.content_ok):
            print(f"      [off] copy not ok: {r_off.notes}\n{r_off.diff_text}"); ok = False
        if n_on is None or n_off is None:
            print(f"      stat accounting missing (on={n_on} off={n_off}) -- statlog not flushed?")
            ok = False
        elif not (n_on < n_off):
            print(f"      NOT honored: source stats on={n_on} not < off={n_off} "
                  f"(coalesce should remove the 2nd per-file stat)")
            ok = False
        else:
            print(f"      honored: source stats on={n_on} < off={n_off} (delta={n_off-n_on})")

        # 2) CORRECT: perms + dates identical with the option on
        probs = K.verify_tree_meta(src, copied_on, check_mode=True, check_mtime=True)
        if probs:
            print("      [on] metadata wrong under coalescing:")
            for p in probs[:8]:
                print(f"        - {p}")
            ok = False

        # 3) ROBUST: source stat mutated AFTER the cache fill -> copy still correct.
        # statmut:<src>:2 leaves the early stats (scan + the transfer-phase fill) genuine and
        # mutates only later stats; with the option on there are no later source stats, so the
        # copy is immune. Assert content correct + alive + no mem-error (no crash/corruption).
        r_mut, copied_mut, _ = _count_source_stats(
            backend, src, K.fresh_dest("coalesce_mut"), coalesce=True, memcheck=memcheck,
            statmut=f"statmut:{os.path.basename(src)}:2")
        if not (r_mut.completed and r_mut.stayed_alive and r_mut.content_ok
                and r_mut.mem_errors == 0):
            print(f"      [mut] not robust to a post-cache source change: completed="
                  f"{r_mut.completed} alive={r_mut.stayed_alive} content={r_mut.content_ok} "
                  f"mem_errors={r_mut.mem_errors}\n{r_mut.diff_text}")
            ok = False
        else:
            print("      robust: copy correct despite source stat mutated after the cache fill")
        return ok

    def iocp_one():
        """IOCP coalesces via CreateFile opens (not libc stat), so the Linux statlog delta
        does not apply; assert the copy is CORRECT + crash-free with the option on (the
        coalescing path is exactly where the LocalFree heap-corruption bug lived)."""
        from lib import winlane
        if not K.windows_host_configured():
            print("    [iocp] SKIP (no Windows host configured)")
            return True
        r = winlane.run_windows("cp", [src], do_right=True, keep_date=True,
                                file_collision=H.FileCollision.OVERWRITE,
                                folder_collision=H.FolderCollision.MERGE,
                                expect=src,
                                extra_options={"coalesceSourceStat": "true"})
        if not r.ok:
            print(f"      [iocp] FAIL: completed={r.completed} alive={r.stayed_alive} "
                  f"content={r.content_ok} crashes={r.mem_errors} notes={r.notes}")
        return r.ok

    return K.for_option_backends(backends, one, iocp_one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
