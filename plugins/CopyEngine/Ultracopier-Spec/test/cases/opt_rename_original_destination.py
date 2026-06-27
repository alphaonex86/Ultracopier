#!/usr/bin/env python3
"""Option verification: renameTheOriginalDestination + the firstRenamingRule placeholder.

WHAT THE ENGINE ACTUALLY DOES (verified by reading the shipping code, NOT by guessing):

  * The renaming machinery lives in TransferThread::checkAlwaysRename()
    (TransferThread.cpp ~630). It only runs when the per-file collision action is
    FileExists_Rename -- i.e. the headless `fileCollision` combo INDEX is 6 (RENAME),
    mapped CopyEngine::setFileCollision case 6 -> FileExists_Rename (CopyEngine.cpp:839).
    With fileCollision=OVERWRITE (index 2 -> FileExists_Overwrite) the engine returns
    early in destinationExists() (TransferThread.cpp:355) and NEVER reaches the rename
    path, so renameTheOriginalDestination would be a no-op. The option is therefore
    exercised under the RENAME collision policy, which is the real code path -- the task's
    "OVERWRITE" wording does not match the shipping behaviour, so we assert the behaviour
    that actually exists (see ../CLAUDE.md "the test is wrong vs the code is wrong": here
    the engine is right and the test must assert the engine's real contract).

  * The backup name is built from firstRenamingRule (num==1 branch, TransferThread.cpp:651):
    the colliding basename is split by renameRegex ^(.*)(\\.[a-zA-Z0-9]+)$ into %name% and
    %suffix%; %name%/%suffix% are substituted into firstRenamingRule. We set
        firstRenamingRule = "%name%.bak%suffix%"
    so a collision on "report.txt" yields the backup name "report.bak.txt" (num==1; the
    do/while stops because that name does not yet exist). The rule survives the factory
    validator (CopyEngineFactory.cpp:285 requires both %name% and %suffix%; ours has both).

  * The DIRECTION that renameTheOriginalDestination controls (TransferThread.cpp:677):
      - renameTheOriginalDestination = TRUE  -> the EXISTING dest file (OLD content) is
        rename()'d to the backup name; `destination` keeps the original name, so the NEW
        source content is written at the ORIGINAL name.
            => report.txt     == NEW (source) content
               report.bak.txt == OLD (pre-existing) content
      - renameTheOriginalDestination = FALSE -> `destination = newDestination`: the NEW
        source content is written at the BACKUP name; the existing dest file is left
        untouched, so it keeps its OLD content at the ORIGINAL name.
            => report.txt     == OLD (pre-existing) content
               report.bak.txt == NEW (source) content

So ON and OFF produce the SAME two files but with the content-to-name mapping SWAPPED.
That is an unambiguous, non-vacuous divergence we assert in BOTH directions: a test that
only checked "report.bak.txt exists" would pass for the wrong reason (it exists either way),
so we assert the exact CONTENT at each name for each option state.

Headless control: the option keys (renameTheOriginalDestination, firstRenamingRule) are
written verbatim into the isolated conf via extra_options; CopyEngineFactory::resetOptions()
copies conf -> the option members / UI at startup (renameTheOriginalDestination via
ui->renameTheOriginalDestination, firstRenamingRule via the member read at getInstance:148),
and getInstance() pushes both into the engine (see project_headless_options_take_effect).

Runs on async + io_uring + IOCP. checkAlwaysRename() is shared TransferThread code (the
Windows rename() uses MoveFileExW(MOVEFILE_REPLACE_EXISTING)), the backup name
"report.bak.txt" is NTFS-valid, so the option applies on all three backends; IOCP is wired
via winlane with dest_pre_seed (self-skips PASS when no [windows] host is configured).
"""
import sys, pathlib, os, shutil
# Strip cases/ from sys.path so stdlib imports are not shadowed by sibling case files
# when this file is run directly (mirrors opt_perms_dates.py / windows_ads.py).
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

COLLIDE_NAME = "report.txt"        # the name the source ships AND the dest pre-seeds
BACKUP_NAME  = "report.bak.txt"    # firstRenamingRule "%name%.bak%suffix%" applied to it
OLD_CONTENT  = b"OLD pre-existing destination content - this is the original\n"
NEW_CONTENT  = b"NEW source content - this is what ultracopier brought in\n"
RULE         = "%name%.bak%suffix%"   # -> report.bak.txt for report.txt (num==1)

# fileCollision INDEX 6 == RENAME (the ONLY policy under which the rename rule + the
# renameTheOriginalDestination flag are consumed; see module docstring).
FILECOLLISION_RENAME = 6


def _build_src():
    """A minimal, deterministic source: one regular file 'report.txt' carrying NEW_CONTENT.
    Keeping the tree to a single colliding file makes the rename divergence unambiguous."""
    src = K.fresh_src_root("renorig_src")
    if os.path.exists(src):
        shutil.rmtree(src)
    os.makedirs(src, exist_ok=True)
    K.write_file(os.path.join(src, COLLIDE_NAME), NEW_CONTENT)
    return src


def _build_expect(rename_original: bool):
    """The exact tree that must appear under dest/<basename(src)>/ after the copy, for the
    given option state. ON swaps content vs OFF -- so content_ok (the harness diff) is itself
    a real discriminator, not just our extra asserts."""
    exp = K.fresh_src_root("renorig_expect_" + ("on" if rename_original else "off"))
    if os.path.exists(exp):
        shutil.rmtree(exp)
    os.makedirs(exp, exist_ok=True)
    if rename_original:
        # TRUE: original name gets NEW (source) content; backup gets OLD (pre-existing).
        K.write_file(os.path.join(exp, COLLIDE_NAME), NEW_CONTENT)
        K.write_file(os.path.join(exp, BACKUP_NAME), OLD_CONTENT)
    else:
        # FALSE: original name keeps OLD content; backup name receives NEW (source) content.
        K.write_file(os.path.join(exp, COLLIDE_NAME), OLD_CONTENT)
        K.write_file(os.path.join(exp, BACKUP_NAME), NEW_CONTENT)
    return exp


def run(backends=None, memcheck=H.NONE) -> bool:
    src = _build_src()
    base = os.path.basename(src)
    exp_on = _build_expect(True)
    exp_off = _build_expect(False)

    def _seed_dest(dest):
        """Pre-place the colliding file (OLD content) where 'report.txt' will land, forcing
        a RENAME collision against real pre-existing content."""
        K.write_file(os.path.join(dest, base, COLLIDE_NAME), OLD_CONTENT)

    def one(backend):
        ok = True

        # ---- renameTheOriginalDestination = TRUE -------------------------------------
        # Existing dest file (OLD) is renamed to report.bak.txt; NEW source content takes
        # the original name report.txt.
        dest_on = K.fresh_dest("renorig_dest_on")
        _seed_dest(dest_on)
        r_on = H.run(backend, "cp", [src], dest_on,
                     file_collision=FILECOLLISION_RENAME,
                     folder_collision=H.FolderCollision.MERGE,
                     extra_options={"renameTheOriginalDestination": "true",
                                    "firstRenamingRule": RULE},
                     expect_dir=exp_on, memcheck=memcheck)
        croot_on = K.copied_root(dest_on, src)
        orig_on = os.path.join(croot_on, COLLIDE_NAME)
        bak_on = os.path.join(croot_on, BACKUP_NAME)
        on_orig_new = os.path.exists(orig_on) and K.read_file(orig_on) == NEW_CONTENT
        on_bak_old = os.path.exists(bak_on) and K.read_file(bak_on) == OLD_CONTENT
        if not (r_on.ok and r_on.content_ok and on_orig_new and on_bak_old):
            print(f"      [on] not honored: completed={r_on.completed} alive={r_on.stayed_alive} "
                  f"content={r_on.content_ok} mem_errors={r_on.mem_errors} "
                  f"orig==NEW={on_orig_new} backup==OLD={on_bak_old}\n{r_on.diff_text}")
            ok = False

        # ---- renameTheOriginalDestination = FALSE ------------------------------------
        # Existing dest file (OLD) is untouched at report.txt; NEW source content is written
        # to the backup name report.bak.txt. NO backing-up of the original occurs.
        dest_off = K.fresh_dest("renorig_dest_off")
        _seed_dest(dest_off)
        r_off = H.run(backend, "cp", [src], dest_off,
                      file_collision=FILECOLLISION_RENAME,
                      folder_collision=H.FolderCollision.MERGE,
                      extra_options={"renameTheOriginalDestination": "false",
                                     "firstRenamingRule": RULE},
                      expect_dir=exp_off, memcheck=memcheck)
        croot_off = K.copied_root(dest_off, src)
        orig_off = os.path.join(croot_off, COLLIDE_NAME)
        bak_off = os.path.join(croot_off, BACKUP_NAME)
        off_orig_old = os.path.exists(orig_off) and K.read_file(orig_off) == OLD_CONTENT
        off_bak_new = os.path.exists(bak_off) and K.read_file(bak_off) == NEW_CONTENT
        if not (r_off.ok and r_off.content_ok and off_orig_old and off_bak_new):
            print(f"      [off] not honored: completed={r_off.completed} alive={r_off.stayed_alive} "
                  f"content={r_off.content_ok} mem_errors={r_off.mem_errors} "
                  f"orig==OLD={off_orig_old} backup==NEW={off_bak_new}\n{r_off.diff_text}")
            ok = False

        # ---- the directions MUST diverge (guards against a vacuous pass) -------------
        # ON: original==NEW ; OFF: original==OLD. If both states produced the same mapping
        # the option had no effect -- fail loudly even if the per-state checks slipped.
        if on_orig_new and off_orig_old:
            print(f"      [{backend}] divergence confirmed: ON original==NEW & backup==OLD; "
                  f"OFF original==OLD & backup==NEW (option honored)")
        else:
            print(f"      [{backend}] NO observable divergence between on/off -> option ignored")
            ok = False
        return ok

    def iocp_one():
        """Assert renameTheOriginalDestination is honored on IOCP, BOTH directions, on the
        Windows box: pre-seed the dest with report.txt (OLD), copy with RENAME collision +
        the firstRenamingRule, and verify the content-to-name mapping via SHA on the box."""
        from lib import winlane
        if not K.windows_host_configured():
            print("    [iocp] SKIP (no Windows host configured)")
            return True

        import hashlib
        h_old = hashlib.sha256(OLD_CONTENT).hexdigest().lower()
        h_new = hashlib.sha256(NEW_CONTENT).hexdigest().lower()

        def _seed_tree(content_for_collide):
            """A LOCAL tree pushed INTO dest\\<base>\\ as the pre-existing collision target
            (winlane.dest_pre_seed lays the CONTENTS down under dest\\basename(src))."""
            seed = K.fresh_src_root("renorig_seed_iocp")
            if os.path.exists(seed):
                shutil.rmtree(seed)
            os.makedirs(seed, exist_ok=True)
            K.write_file(os.path.join(seed, COLLIDE_NAME), content_for_collide)
            return seed

        def _hash_on_box(box, path):
            # \\?\ prefix so NTFS path normalization never hides the file; matches winlane's
            # own remote hashing approach.
            ps = (f"$p='\\\\?\\' + '{path}'; "
                  f"if(Test-Path -LiteralPath '{path}'){{"
                  f"(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLower()}}"
                  f"else{{Write-Output 'MISSING'}}")
            return box.ps(ps).stdout.strip().splitlines()[-1].strip() if box else "MISSING"

        def _pv(want_orig, want_bak):
            def pv(box, dest, srcs):
                copied = winlane.win_join(dest, base)
                orig = winlane.win_join(copied, COLLIDE_NAME)
                bak = winlane.win_join(copied, BACKUP_NAME)
                go = _hash_on_box(box, orig)
                gb = _hash_on_box(box, bak)
                problems = []
                if go != want_orig:
                    problems.append(f"{COLLIDE_NAME} sha={go} != want={want_orig}")
                if gb != want_bak:
                    problems.append(f"{BACKUP_NAME} sha={gb} != want={want_bak}")
                if problems:
                    return False, "rename mapping wrong: " + "; ".join(problems)
                return True, "rename mapping correct"
            return pv

        # ON: original == NEW, backup == OLD
        r_on = winlane.run_windows("cp", [src],
                                   file_collision=FILECOLLISION_RENAME,
                                   folder_collision=H.FolderCollision.MERGE,
                                   extra_options={"renameTheOriginalDestination": "true",
                                                  "firstRenamingRule": RULE},
                                   dest_pre_seed=_seed_tree(OLD_CONTENT),
                                   post_verify=_pv(h_new, h_old))
        if not r_on.ok:
            print(f"      [iocp on] FAIL: completed={r_on.completed} alive={r_on.stayed_alive} "
                  f"content={r_on.content_ok} crashes={r_on.mem_errors} notes={r_on.notes}")

        # OFF: original == OLD, backup == NEW
        r_off = winlane.run_windows("cp", [src],
                                    file_collision=FILECOLLISION_RENAME,
                                    folder_collision=H.FolderCollision.MERGE,
                                    extra_options={"renameTheOriginalDestination": "false",
                                                   "firstRenamingRule": RULE},
                                    dest_pre_seed=_seed_tree(OLD_CONTENT),
                                    post_verify=_pv(h_old, h_new))
        if not r_off.ok:
            print(f"      [iocp off] FAIL: completed={r_off.completed} alive={r_off.stayed_alive} "
                  f"content={r_off.content_ok} crashes={r_off.mem_errors} notes={r_off.notes}")

        return r_on.ok and r_off.ok

    return K.for_option_backends(backends, one, iocp_one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
