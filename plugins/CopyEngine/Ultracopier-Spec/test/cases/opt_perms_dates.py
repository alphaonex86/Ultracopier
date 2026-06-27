#!/usr/bin/env python3
"""Option verification: doRightTransfer (permissions) + the date-always-copied invariant.

This is the metadata overlay the plain content diff (`diff -rq --no-dereference`) is blind
to. Two facts about the engine (verified against the 3.0 baseline -- see
project_keepdate_semantics / project_headless_options_take_effect):

  * doRightTransfer=true  -> the source permission bits ARE reproduced on the destination.
    doRightTransfer=false -> the destination gets the process umask default instead
                             (the source perms are NOT transferred).
  * The file modification DATE is ALWAYS copied for a regular file, on every backend,
    independent of keepDate/doRightTransfer (keepDate only escalates a date *failure*).

So:
  do_right=True  : every dest file mode == its source mode      AND mtime == source mtime.
  do_right=False : every dest file mode == one uniform umask default (NOT source-derived,
                   proven by all-equal + differing from a restrictive source mode)
                                                                 AND mtime == source mtime.

Runs on async + io_uring (full perms + dates). On IOCP it verifies the DATE-always-copied
invariant on the box (LastWriteTime preserved) -- the POSIX octal-mode assertion does not
map to NTFS ACLs, so the doRightTransfer ACL specifics live in a dedicated IOCP case
(opt_iocp_acl); here IOCP still asserts the date is honored, so the option is covered on
all three backends per the requirement.
"""
import sys, pathlib, os
# Strip cases/ from sys.path so stdlib `import copy` is not shadowed by cases/copy.py
# when this file is run directly (see windows_ads.py).
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

OLD_MTIME = 981173106  # 2001-02-03 04:05:06 UTC, set by make_meta_tree on every entry


def run(backends=None, memcheck=H.NONE) -> bool:
    src = K.fresh_src_root("optmeta_src")
    modes = K.make_meta_tree(src)            # {relpath: mode}, all mtime = 981173106

    def one(backend):
        ok = True
        # ---- doRightTransfer = TRUE : perms + dates reproduced exactly ----------------
        dest = K.fresh_dest("optmeta_dest_on")
        r = H.run(backend, "cp", [src], dest, do_right=True, keep_date=True,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=src, memcheck=memcheck)
        copied = K.copied_root(dest, src)
        if not (r.ok and r.content_ok):
            print(f"      [on] not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"content={r.content_ok} mem_errors={r.mem_errors}\n{r.diff_text}")
            ok = False
        probs = K.verify_tree_meta(src, copied, check_mode=True, check_mtime=True)
        if probs:
            print("      [on] metadata mismatches (perms+dates must match source):")
            for p in probs:
                print(f"        - {p}")
            ok = False

        # ---- doRightTransfer = FALSE : perms NOT transferred, dates STILL copied -------
        dest2 = K.fresh_dest("optmeta_dest_off")
        r2 = H.run(backend, "cp", [src], dest2, do_right=False, keep_date=True,
                   file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.MERGE,
                   expect_dir=src, memcheck=memcheck)
        copied2 = K.copied_root(dest2, src)
        if not (r2.ok and r2.content_ok):
            print(f"      [off] not ok: completed={r2.completed} alive={r2.stayed_alive} "
                  f"content={r2.content_ok} mem_errors={r2.mem_errors}\n{r2.diff_text}")
            ok = False
        # dates: still copied even with doRightTransfer off
        date_probs = K.verify_tree_meta(src, copied2, check_mode=False, check_mtime=True)
        if date_probs:
            print("      [off] date NOT copied (must always be copied, indep. of doRightTransfer):")
            for p in date_probs:
                print(f"        - {p}")
            ok = False
        # perms: every dest file should share ONE uniform default mode (umask-derived),
        # and that default must differ from a deliberately-restrictive source mode --
        # proving the source perms were NOT transferred.
        dest_modes = {}
        for rel in modes:
            dp = os.path.join(copied2, rel)
            if os.path.exists(dp):
                dest_modes[rel] = os.lstat(dp).st_mode & 0o7777
        uniq = set(dest_modes.values())
        if len(uniq) != 1:
            print(f"      [off] dest modes not uniform (perms leaked from source?): {{ "
                  + ", ".join(f'{k}={oct(v)}' for k, v in dest_modes.items()) + " }")
            ok = False
        else:
            default_mode = next(iter(uniq))
            # secret.dat is 0o600 in the source; a real default (e.g. 0o755/0o644) must differ.
            if default_mode == modes["secret.dat"]:
                print(f"      [off] dest mode {oct(default_mode)} equals restrictive source "
                      f"mode -> perms WERE transferred despite doRightTransfer=off")
                ok = False
            else:
                print(f"      [off] perms not transferred (uniform default {oct(default_mode)}, "
                      f"source secret.dat was {oct(modes['secret.dat'])}) -- correct")
        return ok

    def iocp_one():
        """Assert the date-always-copied invariant is honored on IOCP: every dest file's
        LastWriteTime must still be the source's 2001 mtime after a do_right copy."""
        from lib import winlane
        if not K.windows_host_configured():
            print("    [iocp] SKIP (no Windows host configured)")
            return True

        def pv(box, dest, srcs):
            copied = winlane.win_join(dest, os.path.basename(src))
            problems = []
            for rel in modes:
                dp = winlane.win_join(copied, rel.replace("/", "\\"))
                m = winlane.win_meta(box, dp)
                if not m:
                    problems.append(f"{rel}: missing on box")
                elif abs(m.get("mtime", 0) - OLD_MTIME) > 2:
                    problems.append(f"{rel}: LastWriteTime {m.get('mtime')} != {OLD_MTIME}")
            if problems:
                return False, "date not preserved: " + "; ".join(problems[:5])
            return True, "date preserved on all files"

        r = winlane.run_windows("cp", [src], do_right=True, keep_date=True,
                                file_collision=H.FileCollision.OVERWRITE,
                                folder_collision=H.FolderCollision.MERGE,
                                expect=src, post_verify=pv)
        if not r.ok:
            print(f"      [iocp] FAIL: completed={r.completed} alive={r.stayed_alive} "
                  f"content={r.content_ok} crashes={r.mem_errors} notes={r.notes}")
        return r.ok

    return K.for_option_backends(backends, one, iocp_one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
