#!/usr/bin/env python3
"""folderCollision policy: Skip(2) vs Rename(3), honored on async + io_uring + IOCP.

The destination is PRE-SEEDED with a folder of the SAME name as the source folder
(<dest>/<base>) holding a file the SOURCE does NOT contain ("marker_preexisting.txt").
The source folder holds DIFFERENT files ("marker_source.txt" + a nested file). So the
top-level source folder COLLIDES with an existing destination folder, and the chosen
folderCollision index decides what happens.

Engine behaviour verified against the code (ScanFileOrFolder::listFolder, the
checkDestinationExists block ~line 562, reached when alwaysDoThisActionForFolderExists
!= FolderExists_Merge; the combo INDEX is mapped in CopyEngine::setFolderCollision:
index 2 -> FolderExists_Skip, index 3 -> FolderExists_Rename):

  * Skip(2)   -> on the colliding folder the scan `return`s immediately: NONE of the
                 source children are listed/transferred. The pre-existing <dest>/<base>
                 folder is left UNTOUCHED (keeps only its pre-seeded file), the source is
                 NOT merged in, and no new folder is created.
  * Rename(3) -> destination is renamed to "<base> - copy" (the default %name% - copy
                 pattern, firstRenamingRule empty) and the source children are scanned
                 INTO that NEW folder. The ORIGINAL <dest>/<base> is left intact (keeps
                 only its pre-seeded file); the renamed sibling holds the FULL source tree
                 and NOT the pre-existing file.

The two modes diverge OBSERVABLY in BOTH directions, and each direction is asserted:
  - Skip:   source files NEVER appear under dest;     pre-existing file survives untouched.
  - Rename: source files appear in a NEW sibling folder; pre-existing file survives untouched
            in the original folder; the source files are NOT in the original folder.
A blind "the folder exists" check would pass for the wrong reason (the folder exists
regardless), so we assert the PRESENCE/ABSENCE of the marker files, which differs by mode.

IOCP applies identically (NTFS folder collision is the same policy), so it is wired via
winlane.run_windows with dest_pre_seed + a post_verify that inspects the box; it self-skips
(PASS) when no [windows] host is configured and is REAL when one exists.
"""
import sys, pathlib, os
# Strip cases/ from sys.path so stdlib imports are never shadowed by a cases/*.py
# module when this file is run directly (see opt_perms_dates.py / windows_ads.py).
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

# Dot-free folder name so the engine's "%name% - copy" rename is unambiguous.
SRC_NAME = "colfolder_src"

PREEXIST_REL = "marker_preexisting.txt"          # only ever in the pre-seeded dest folder
PREEXIST_DATA = b"PRE-EXISTING destination file - must survive untouched\n"

SRC_FILE_REL = "marker_source.txt"               # only ever in the source folder
SRC_FILE_DATA = b"SOURCE file - lands only when the folder is actually copied/merged\n"
SRC_NESTED_REL = os.path.join("sub", "nested.dat")
SRC_NESTED_DATA = b"nested source payload\n"

# For reference only: the engine's default renamed name is "<base> - copy" (the %name% - copy
# pattern, firstRenamingRule empty). We do NOT hardcode it in assertions -- its exact suffix
# depends on a fragile rfind in ScanFileOrFolder.cpp -- so the renamed folder is DISCOVERED by
# content (the unique new sub-dir of dest that is not <base>) on both lanes.
EXPECTED_RENAMED_BASENAME = SRC_NAME + " - copy"


def _make_source(root: str) -> str:
    """Build the colliding source folder with files the dest pre-seed does NOT have."""
    import shutil
    if os.path.exists(root):
        shutil.rmtree(root)
    os.makedirs(root, exist_ok=True)
    K.write_file(os.path.join(root, SRC_FILE_REL), SRC_FILE_DATA)
    K.write_file(os.path.join(root, SRC_NESTED_REL), SRC_NESTED_DATA)
    return root


def run(backends=None, memcheck=H.NONE) -> bool:
    src = _make_source(K.fresh_src_root(SRC_NAME))
    base = os.path.basename(src)

    def _seed_dest(dest: str):
        """Pre-seed <dest>/<base>/marker_preexisting.txt -> a same-named existing folder."""
        K.write_file(os.path.join(dest, base, PREEXIST_REL), PREEXIST_DATA)

    def one(backend):
        ok = True

        # ---- Skip(2): the pre-existing folder is left untouched, source NOT merged ----
        dest_skip = K.fresh_dest("colfolder_skip_dest")
        _seed_dest(dest_skip)
        orig = os.path.join(dest_skip, base)
        # No expect_dir: a Skip leaves the dest folder == the pre-seed (NOT the source),
        # so the harness content diff is irrelevant here; we assert presence/absence below.
        # checkDestinationFolder=true is REQUIRED: the folderCollision switch is reached only
        # inside `if(checkDestinationExists)` (ScanFileOrFolder.cpp:563), and that flag =
        # checkDestinationFolderExists && action!=Merge (ListThreadScan.cpp:49). Headlessly the
        # checkbox defaults OFF, so without this the engine MERGES and never honors Skip/Rename.
        r1 = H.run(backend, "cp", [src], dest_skip,
                   file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.SKIP,
                   extra_options={"checkDestinationFolder": "true"},
                   expect_dir=None, memcheck=memcheck)
        pre_survived = (os.path.exists(os.path.join(orig, PREEXIST_REL)) and
                        K.read_file(os.path.join(orig, PREEXIST_REL)) == PREEXIST_DATA)
        # the source files MUST NOT have landed anywhere under dest (no merge, no rename):
        # the only entry under dest must be the untouched <base> folder.
        src_in_orig = os.path.exists(os.path.join(orig, SRC_FILE_REL))
        dest_entries = sorted(os.listdir(dest_skip))
        renamed_created = (dest_entries != [base])
        skip_ok = (pre_survived and not src_in_orig and not renamed_created)
        if not (r1.ok and skip_ok):
            print(f"      [skip] not ok: completed={r1.completed} alive={r1.stayed_alive} "
                  f"mem_errors={r1.mem_errors} pre_survived={pre_survived} "
                  f"src_in_orig={src_in_orig} renamed_created={renamed_created}\n{r1.notes}")
            ok = False

        # ---- Rename(3): a renamed dest folder is created, the original left intact ----
        dest_ren = K.fresh_dest("colfolder_rename_dest")
        _seed_dest(dest_ren)
        orig_r = os.path.join(dest_ren, base)
        r2 = H.run(backend, "cp", [src], dest_ren,
                   file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.RENAME,
                   extra_options={"checkDestinationFolder": "true"},
                   expect_dir=None, memcheck=memcheck)
        # original folder intact: still only the pre-existing file, NOT merged with source
        orig_pre_ok = (os.path.exists(os.path.join(orig_r, PREEXIST_REL)) and
                       K.read_file(os.path.join(orig_r, PREEXIST_REL)) == PREEXIST_DATA)
        orig_clean = not os.path.exists(os.path.join(orig_r, SRC_FILE_REL))
        # DISCOVER the renamed sibling: a new dir != <base> created under dest. (The engine's
        # default name is "<base> - copy", but its exact suffix depends on a fragile rfind in
        # ScanFileOrFolder.cpp, so we locate it by content instead of hardcoding the name.)
        siblings = [d for d in os.listdir(dest_ren)
                    if d != base and os.path.isdir(os.path.join(dest_ren, d))]
        renamed = os.path.join(dest_ren, siblings[0]) if len(siblings) == 1 else None
        # renamed folder holds the FULL source tree and NOT the pre-existing file
        renamed_has_src = (renamed is not None and
                           os.path.exists(os.path.join(renamed, SRC_FILE_REL)) and
                           K.read_file(os.path.join(renamed, SRC_FILE_REL)) == SRC_FILE_DATA and
                           os.path.exists(os.path.join(renamed, SRC_NESTED_REL)) and
                           K.read_file(os.path.join(renamed, SRC_NESTED_REL)) == SRC_NESTED_DATA)
        renamed_clean = (renamed is not None and
                         not os.path.exists(os.path.join(renamed, PREEXIST_REL)))
        rename_ok = (orig_pre_ok and orig_clean and renamed_has_src and renamed_clean)
        if not (r2.ok and rename_ok):
            print(f"      [rename] not ok: completed={r2.completed} alive={r2.stayed_alive} "
                  f"mem_errors={r2.mem_errors} orig_pre_ok={orig_pre_ok} "
                  f"orig_clean={orig_clean} renamed_has_src={renamed_has_src} "
                  f"renamed_clean={renamed_clean}\n{r2.notes}")
            ok = False

        return ok

    def iocp_one():
        """Same Skip vs Rename divergence on IOCP/NTFS, verified on the Windows box.

        Two separate run_windows calls (Skip then Rename), each with dest_pre_seed laying
        down <dest>\\<base>\\marker_preexisting.txt before ultracopier runs, and a
        post_verify that inspects the box and returns (ok, note). The post_verify enforces
        BOTH directions just like the Linux lane."""
        from lib import winlane
        if not K.windows_host_configured():
            print("    [iocp] SKIP (no Windows host configured)")
            return True

        # dest_pre_seed pushes the CONTENTS of this tree into <dest>\<basename(src)>, so the
        # pre-existing file lands at <dest>\<base>\marker_preexisting.txt (a same-named folder).
        seed_root = K.fresh_src_root("colfolder_iocp_seed")
        import shutil
        if os.path.exists(seed_root):
            shutil.rmtree(seed_root)
        os.makedirs(seed_root, exist_ok=True)
        K.write_file(os.path.join(seed_root, PREEXIST_REL), PREEXIST_DATA)

        def _exists(box, p) -> bool:
            out = box.ps(f"if (Test-Path -LiteralPath '{p}') {{ Write-Output 'YES' }} "
                         f"else {{ Write-Output 'NO' }}")
            return "YES" in out.stdout

        def _child_dirs(box, p) -> list:
            """Names of immediate sub-directories of p on the box (one per line)."""
            out = box.ps(f"Get-ChildItem -LiteralPath '{p}' -Directory -Force "
                         f"-ErrorAction SilentlyContinue | ForEach-Object {{ $_.Name }}")
            return [ln.strip() for ln in out.stdout.splitlines() if ln.strip()]

        def pv_skip(box, dest, srcs):
            orig = winlane.win_join(dest, base)
            pre_ok = _exists(box, winlane.win_join(orig, PREEXIST_REL))
            src_in_orig = _exists(box, winlane.win_join(orig, SRC_FILE_REL))
            # the only sub-dir of dest must be the untouched <base> folder (no rename created)
            renamed_created = (_child_dirs(box, dest) != [base])
            ok = pre_ok and (not src_in_orig) and (not renamed_created)
            return ok, (f"skip: pre_ok={pre_ok} src_in_orig={src_in_orig} "
                        f"renamed_created={renamed_created}")

        def pv_rename(box, dest, srcs):
            orig = winlane.win_join(dest, base)
            orig_pre_ok = _exists(box, winlane.win_join(orig, PREEXIST_REL))
            orig_clean = not _exists(box, winlane.win_join(orig, SRC_FILE_REL))
            # DISCOVER the renamed sibling: a new sub-dir of dest != <base>.
            siblings = [d for d in _child_dirs(box, dest) if d != base]
            renamed = winlane.win_join(dest, siblings[0]) if len(siblings) == 1 else None
            renamed_has_src = (renamed is not None and
                               _exists(box, winlane.win_join(renamed, SRC_FILE_REL)) and
                               _exists(box, winlane.win_join(renamed,
                                                             SRC_NESTED_REL.replace("/", "\\"))))
            renamed_clean = (renamed is not None and
                             not _exists(box, winlane.win_join(renamed, PREEXIST_REL)))
            ok = orig_pre_ok and orig_clean and renamed_has_src and renamed_clean
            return ok, (f"rename: orig_pre_ok={orig_pre_ok} orig_clean={orig_clean} "
                        f"siblings={siblings} renamed_has_src={renamed_has_src} "
                        f"renamed_clean={renamed_clean}")

        r_skip = winlane.run_windows("cp", [src],
                                     file_collision=H.FileCollision.OVERWRITE,
                                     folder_collision=H.FolderCollision.SKIP,
                                     extra_options={"checkDestinationFolder": "true"},
                                     expect=None, dest_pre_seed=seed_root, post_verify=pv_skip)
        if not r_skip.ok:
            print(f"      [iocp/skip] FAIL: completed={r_skip.completed} "
                  f"alive={r_skip.stayed_alive} content={r_skip.content_ok} "
                  f"crashes={r_skip.mem_errors} notes={r_skip.notes}")

        r_ren = winlane.run_windows("cp", [src],
                                    file_collision=H.FileCollision.OVERWRITE,
                                    folder_collision=H.FolderCollision.RENAME,
                                    extra_options={"checkDestinationFolder": "true"},
                                    expect=None, dest_pre_seed=seed_root, post_verify=pv_rename)
        if not r_ren.ok:
            print(f"      [iocp/rename] FAIL: completed={r_ren.completed} "
                  f"alive={r_ren.stayed_alive} content={r_ren.content_ok} "
                  f"crashes={r_ren.mem_errors} notes={r_ren.notes}")

        return r_skip.ok and r_ren.ok

    return K.for_option_backends(backends, one, iocp_one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
