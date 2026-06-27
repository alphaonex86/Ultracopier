#!/usr/bin/env python3
"""Option verification: checkDestinationFolder (conf key "checkDestinationFolder",
widget checkBoxDestinationFolderExists), honored on async + io_uring + IOCP.

What the option does (verified against the engine, NOT guessed):

  ListThreadScan.cpp:49 wires the scanner with
      setCheckDestinationFolderExists(checkDestinationFolderExists
                                      && alwaysDoThisActionForFolderExists != FolderExists_Merge)
  i.e. the scanner's `checkDestinationExists` flag is the OPTION value, but ONLY when the
  folderCollision policy is NOT Merge. In ScanFileOrFolder::listFolder (ScanFileOrFolder.cpp
  ~line 562) that flag guards a SECOND folder-collision check: if the TOP-LEVEL destination
  folder (<dest>/<base>) already exists, the scanner emits folderAlreadyExists(...,false) and
  applies the folderExistsAction. With the flag OFF that whole block is skipped, so an existing
  top-level destination folder is NOT detected at scan time and the source is processed normally.

  CRITICAL consequence we exploit for an OBSERVABLE on/off divergence, with folderCollision
  pinned to Skip (a non-Merge value, so the option actually flows through line 49):

    * checkDestinationFolder = TRUE  + Skip -> the existing <dest>/<base> IS detected ->
      FolderExists_Skip -> the scan `return`s on the top-level folder: NONE of the source
      children are listed or transferred. The dest keeps ONLY its pre-seeded file; the source
      payload is ABSENT.
    * checkDestinationFolder = FALSE + Skip -> checkDestinationExists is false -> the existing
      top-level folder is NOT detected -> the source folder is scanned and its children are
      MERGED into <dest>/<base>: the source payload is PRESENT alongside the pre-seeded file.

So the SAME folderCollision (Skip) + the SAME pre-seeded colliding dest gives opposite results
purely from the option: source payload ABSENT (on) vs PRESENT (off). We assert BOTH directions
by the PRESENCE/ABSENCE of a source-only marker file (and that the pre-seeded file always
survives) -- never a blind "the folder exists" check, which would pass for the wrong reason
because the folder exists regardless of the option.

This is DISTINCT from collision_folder_modes.py: that case fixes the option ON and varies the
folderCollision *mode*; this case fixes the mode (Skip) and varies the *option*, so it is the
option's own on-vs-off effect that is under test.

(Note: at folderCollision=Merge the option is intentionally masked to no effect by line 49 --
Merge means "descend and merge" whether or not the top folder pre-exists -- so Merge cannot
exhibit an on/off difference and is deliberately NOT used here.)

IOCP applies identically (the scan-time folder-exists check is platform-independent), so it is
wired via winlane.run_windows with dest_pre_seed + a post_verify that inspects the box; it
self-skips (PASS) when no [windows] host is configured and is REAL when one exists.
"""
import sys, pathlib, os
# Strip cases/ from sys.path so stdlib imports are never shadowed by a cases/*.py
# module when this file is run directly (see opt_perms_dates.py / collision_folder_modes.py).
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

SRC_NAME = "cdf_src"            # dot-free, so no rename-suffix ambiguity ever applies

PREEXIST_REL = "marker_preexisting.txt"   # only ever in the pre-seeded dest folder
PREEXIST_DATA = b"PRE-EXISTING destination file - must always survive\n"

SRC_FILE_REL = "marker_source.txt"        # only ever in the source folder
SRC_FILE_DATA = b"SOURCE payload - present only when the source folder is actually scanned\n"
SRC_NESTED_REL = os.path.join("sub", "nested.dat")
SRC_NESTED_DATA = b"nested source payload\n"


def _make_source(root: str) -> str:
    """Build the source folder with files the dest pre-seed does NOT have."""
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

        # ---- checkDestinationFolder = TRUE + Skip : top folder detected -> source SKIPPED ----
        dest_on = K.fresh_dest("cdf_on_dest")
        _seed_dest(dest_on)
        orig_on = os.path.join(dest_on, base)
        # No expect_dir: with the option ON the dest stays == the pre-seed (source NOT merged),
        # so the harness content diff is not the right oracle here; we assert presence/absence.
        r_on = H.run(backend, "cp", [src], dest_on,
                     file_collision=H.FileCollision.OVERWRITE,
                     folder_collision=H.FolderCollision.SKIP,
                     extra_options={"checkDestinationFolder": "true"},
                     expect_dir=None, memcheck=memcheck)
        pre_survived_on = (os.path.exists(os.path.join(orig_on, PREEXIST_REL)) and
                           K.read_file(os.path.join(orig_on, PREEXIST_REL)) == PREEXIST_DATA)
        # the source payload MUST NOT have landed: option ON + Skip skips the whole folder
        src_present_on = (os.path.exists(os.path.join(orig_on, SRC_FILE_REL)) or
                          os.path.exists(os.path.join(orig_on, SRC_NESTED_REL)))
        on_ok = (pre_survived_on and not src_present_on)
        if not (r_on.ok and on_ok):
            print(f"      [on] not ok: completed={r_on.completed} alive={r_on.stayed_alive} "
                  f"mem_errors={r_on.mem_errors} pre_survived={pre_survived_on} "
                  f"src_present={src_present_on} (option ON+Skip must SKIP the source)\n{r_on.notes}")
            ok = False

        # ---- checkDestinationFolder = FALSE + Skip : not detected -> source MERGED in --------
        dest_off = K.fresh_dest("cdf_off_dest")
        _seed_dest(dest_off)
        orig_off = os.path.join(dest_off, base)
        # Expectation with the option OFF = the UNION (pre-existing file survives + source added),
        # so the harness content diff is a real oracle. Build it explicitly.
        import shutil
        exp = K.fresh_src_root("cdf_off_expect")
        if os.path.exists(exp):
            shutil.rmtree(exp)
        shutil.copytree(src, exp, symlinks=True)
        K.write_file(os.path.join(exp, PREEXIST_REL), PREEXIST_DATA)
        r_off = H.run(backend, "cp", [src], dest_off,
                      file_collision=H.FileCollision.OVERWRITE,
                      folder_collision=H.FolderCollision.SKIP,
                      extra_options={"checkDestinationFolder": "false"},
                      expect_dir=exp, memcheck=memcheck)
        pre_survived_off = (os.path.exists(os.path.join(orig_off, PREEXIST_REL)) and
                            K.read_file(os.path.join(orig_off, PREEXIST_REL)) == PREEXIST_DATA)
        # the source payload MUST be present: option OFF means the folder collision is NOT
        # detected, so the source is merged into the existing dest folder.
        src_file_ok = (os.path.exists(os.path.join(orig_off, SRC_FILE_REL)) and
                       K.read_file(os.path.join(orig_off, SRC_FILE_REL)) == SRC_FILE_DATA)
        src_nested_ok = (os.path.exists(os.path.join(orig_off, SRC_NESTED_REL)) and
                         K.read_file(os.path.join(orig_off, SRC_NESTED_REL)) == SRC_NESTED_DATA)
        off_ok = (pre_survived_off and src_file_ok and src_nested_ok)
        if not (r_off.ok and r_off.content_ok and off_ok):
            print(f"      [off] not ok: completed={r_off.completed} alive={r_off.stayed_alive} "
                  f"content={r_off.content_ok} mem_errors={r_off.mem_errors} "
                  f"pre_survived={pre_survived_off} src_file_ok={src_file_ok} "
                  f"src_nested_ok={src_nested_ok} (option OFF+Skip must MERGE the source)\n"
                  f"{r_off.diff_text}\n{r_off.notes}")
            ok = False

        # ---- The on/off states MUST diverge observably (guard against a no-op option) -------
        # ON => source absent; OFF => source present. If both ended up the same, the option had
        # no effect and the test would be vacuous -- fail loudly.
        diverged = (src_present_on != (src_file_ok or src_nested_ok))
        if not diverged:
            print(f"      [{backend}] NO DIVERGENCE: option made no observable difference "
                  f"(src_present_on={src_present_on}, src_present_off={src_file_ok or src_nested_ok})")
            ok = False

        return ok

    def iocp_one():
        """Same on/off divergence on IOCP/NTFS, verified on the Windows box.

        Two run_windows calls (option ON then OFF), both with folderCollision=Skip and a
        dest_pre_seed laying down <dest>\\<base>\\marker_preexisting.txt before ultracopier runs.
        A post_verify inspects the box and asserts BOTH directions:
          ON  -> source markers ABSENT under <dest>\\<base>, pre-existing file survives.
          OFF -> source markers PRESENT under <dest>\\<base>, pre-existing file survives."""
        from lib import winlane
        if not K.windows_host_configured():
            print("    [iocp] SKIP (no Windows host configured)")
            return True

        # dest_pre_seed pushes the CONTENTS of this tree into <dest>\<basename(src)>, so the
        # pre-existing file lands at <dest>\<base>\marker_preexisting.txt (a same-named folder).
        seed_root = K.fresh_src_root("cdf_iocp_seed")
        import shutil
        if os.path.exists(seed_root):
            shutil.rmtree(seed_root)
        os.makedirs(seed_root, exist_ok=True)
        K.write_file(os.path.join(seed_root, PREEXIST_REL), PREEXIST_DATA)

        def _exists(box, p) -> bool:
            out = box.ps(f"if (Test-Path -LiteralPath '{p}') {{ Write-Output 'YES' }} "
                         f"else {{ Write-Output 'NO' }}")
            return "YES" in out.stdout

        def pv_on(box, dest, srcs):
            orig = winlane.win_join(dest, base)
            pre_ok = _exists(box, winlane.win_join(orig, PREEXIST_REL))
            src_present = (_exists(box, winlane.win_join(orig, SRC_FILE_REL)) or
                           _exists(box, winlane.win_join(orig,
                                                         SRC_NESTED_REL.replace("/", "\\"))))
            ok = pre_ok and (not src_present)
            return ok, f"on(skip): pre_ok={pre_ok} src_present={src_present} (must be absent)"

        def pv_off(box, dest, srcs):
            orig = winlane.win_join(dest, base)
            pre_ok = _exists(box, winlane.win_join(orig, PREEXIST_REL))
            src_file = _exists(box, winlane.win_join(orig, SRC_FILE_REL))
            src_nested = _exists(box, winlane.win_join(orig,
                                                       SRC_NESTED_REL.replace("/", "\\")))
            ok = pre_ok and src_file and src_nested
            return ok, (f"off(skip): pre_ok={pre_ok} src_file={src_file} "
                        f"src_nested={src_nested} (must be present)")

        r_on = winlane.run_windows("cp", [src],
                                   file_collision=H.FileCollision.OVERWRITE,
                                   folder_collision=H.FolderCollision.SKIP,
                                   extra_options={"checkDestinationFolder": "true"},
                                   expect=None, dest_pre_seed=seed_root, post_verify=pv_on)
        if not r_on.ok:
            print(f"      [iocp/on] FAIL: completed={r_on.completed} alive={r_on.stayed_alive} "
                  f"content={r_on.content_ok} crashes={r_on.mem_errors} notes={r_on.notes}")

        r_off = winlane.run_windows("cp", [src],
                                    file_collision=H.FileCollision.OVERWRITE,
                                    folder_collision=H.FolderCollision.SKIP,
                                    extra_options={"checkDestinationFolder": "false"},
                                    expect=None, dest_pre_seed=seed_root, post_verify=pv_off)
        if not r_off.ok:
            print(f"      [iocp/off] FAIL: completed={r_off.completed} alive={r_off.stayed_alive} "
                  f"content={r_off.content_ok} crashes={r_off.mem_errors} notes={r_off.notes}")

        return r_on.ok and r_off.ok

    return K.for_option_backends(backends, one, iocp_one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
