#!/usr/bin/env python3
"""Option verification: mkpath / mkFullPath (conf key "mkpath", widget ui->mkpath),
honored on async + io_uring + IOCP.

WHAT THE OPTION DOES (read from the engine, NOT guessed):

  CopyEngineFactory.cpp:145 / 249  registers "mkpath" (default "true") and pushes it through
  realObject->setMkFullPath(...) -> CopyEngine::setMkFullPath -> ListThread::setMkFullPath,
  which forwards it to the dedicated *MkPath worker* (mkPathQueue.setMkFullPath) -- the thread
  that materialises every DIRECTORY of the copy (ActionType_MkPath, queued by
  ScanFileOrFolder::listFolder -> ListThread::addToMkPath for each source folder).

  The flag is read in exactly ONE place, MkPath::internalDoThisPath (MkPath.cpp ~169-214):

      if(mkFullPath)  TransferThread::mkpath(dest)   // RECURSIVE: creates missing parents too
      else            TransferThread::mkdir (dest)   // SINGLE-LEVEL: fails if a parent is absent

  i.e. mkpath=true creates the full destination path (every missing intermediate component);
  mkpath=false creates only ONE level and FAILS (emit errorOnFolder) if an intermediate
  destination directory does not already exist.

  IMPORTANT engine fact this case relies on (verified across all three backends): the flag does
  NOT govern the *file* parent-directory creation. async/WriteThread.cpp:221 + :253,
  async/TransferThreadAsync.cpp:458/476, uring/TransferThreadUring.cpp:222 and
  win-iocp/TransferThreadWin.cpp:269 ALL call TransferThread::mkpath(parent) unconditionally
  before writing a file -- so a file ALWAYS gets its parent chain created recursively regardless
  of mkpath. (TransferThread also stores a mkFullPath member, but it is never read -- guarded by
  dead_option_ratchet only for the registered-key side; here the real consumer is the MkPath
  worker.) Consequence: to make the option's on/off effect OBSERVABLE we must use a source that
  contains NO regular files -- ONLY directories -- so that EVERY directory is created by the
  MkPath worker (the only mkFullPath consumer) and nothing is created by the always-recursive
  file path.

OBSERVABLE on-vs-off divergence we exploit (deterministic, race-free):

  Copy an all-directories-only source into a destination whose path has a MISSING intermediate
  component (<scratch>/.../miss_a/miss_b -- a path that does NOT exist). ultracopier copies the
  source FOLDER into <dest> -> the top MkPath destination is <dest>/<base>, whose parent
  (miss_b, and miss_a above it) does not exist.

    * mkpath = TRUE  -> MkPath uses TransferThread::mkpath -> the missing intermediate chain AND
      the whole empty-directory tree are created -> the copied tree equals the source
      (diff -rq --no-dereference == 0, harness content_ok True).
    * mkpath = FALSE -> MkPath uses single-level TransferThread::mkdir on <dest>/<base>; its
      parent is absent -> mkdir fails -> errorOnFolder. With folderCollision/folderError pinned
      to Skip the failure auto-skips (CopyEngine-collision-and-error.cpp:576 mkPathQueue.skip());
      because the source has NO files, the always-recursive file path is NEVER exercised, so
      NOTHING is created -> the copied root <dest>/<base> does NOT exist at all.

  We assert BOTH directions (on: full tree present + content_ok; off: copied root ABSENT) and a
  divergence guard, so neither side can pass for the wrong reason (the off-state cannot "exist
  anyway", and the on-state cannot "be created regardless").

IOCP (NTFS): the mkFullPath consumer (MkPath worker) and the recursive vs single-level
distinction are platform-independent (TransferThread::mkpath/mkdir have a full Win32 branch), so
the option applies on NTFS. It is wired REAL via winlane.run_windows with the SAME all-empty-dir
source. The off-state divergence on IOCP needs a destination with a missing intermediate
component, which the Windows lane does not let a case inject (winlane always creates <dest> and
copies into <dest>\\<base>); so the IOCP lane asserts the strongest direction it CAN force from
the outside: that mkpath=true RECONSTRUCTS the empty-directory tree on NTFS -- an observable the
winlane file-hash manifest is blind to (it lists files+symlinks only, never empty dirs), so a
bare run with no post_verify would falsely "pass" with nothing copied. The post_verify checks
the DEEPEST empty directory is present on the box, which fails loudly if empty-dir / mkpath
creation ever broke on Windows. The off-direction is fully asserted on async + io_uring above;
the missing-intermediate limitation of the IOCP lane is recorded honestly (see the module
docstring / the spec open_questions). The lane self-skips (PASS) when no [windows] host exists
and is REAL when one is configured.
"""
import sys, pathlib, os
# Strip cases/ from sys.path so stdlib imports are never shadowed by a cases/*.py module
# when this file is run directly (mirrors opt_perms_dates.py / opt_check_dest_folder.py).
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

SRC_NAME = "mkpath_src"                 # dot-free: no rename-suffix ambiguity
# Deepest empty-directory chain INSIDE the source folder. Directories only -- NO files anywhere,
# so the MkPath worker (the sole mkFullPath consumer) is the only thing that can create them.
DEEP_REL = os.path.join("d1", "d2", "d3", "leaf")   # <base>/d1/d2/d3/leaf
SECOND_REL = os.path.join("other", "branch")        # a second empty branch, for good measure


def _make_dir_only_source(root: str) -> str:
    """Build a source folder containing ONLY (nested, empty) directories -- never a file."""
    import shutil
    if os.path.exists(root):
        shutil.rmtree(root)
    os.makedirs(os.path.join(root, DEEP_REL), exist_ok=True)
    os.makedirs(os.path.join(root, SECOND_REL), exist_ok=True)
    return root


def run(backends=None, memcheck=H.NONE) -> bool:
    src = _make_dir_only_source(K.fresh_src_root(SRC_NAME))
    base = os.path.basename(src)

    def one(backend):
        ok = True

        # ---- mkpath = TRUE : missing intermediate chain + empty-dir tree are created ----------
        # A scratch base that EXISTS, then a sub-path under it that does NOT exist: the copy must
        # create miss_a/miss_b itself (recursively) before it can create <base> and its tree.
        dest_base_on = K.fresh_dest("mkpath_on_destbase")
        dest_on = os.path.join(dest_base_on, "miss_a", "miss_b")   # missing intermediate parents
        assert not os.path.exists(dest_on), "off-by-design: dest intermediate must start absent"
        r_on = H.run(backend, "cp", [src], dest_on,
                     file_collision=H.FileCollision.OVERWRITE,
                     folder_collision=H.FolderCollision.SKIP,
                     file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                     extra_options={"mkpath": "true"},
                     expect_dir=src, memcheck=memcheck)
        copied_on = K.copied_root(dest_on, src)        # <dest_on>/<base>
        # the full empty-directory tree must be reconstructed (content diff is the oracle here)
        deep_on_present = os.path.isdir(os.path.join(copied_on, DEEP_REL))
        second_on_present = os.path.isdir(os.path.join(copied_on, SECOND_REL))
        on_ok = r_on.ok and r_on.content_ok and deep_on_present and second_on_present
        if not on_ok:
            print(f"      [on] not ok: completed={r_on.completed} alive={r_on.stayed_alive} "
                  f"content={r_on.content_ok} mem_errors={r_on.mem_errors} "
                  f"deep_present={deep_on_present} second_present={second_on_present} "
                  f"(mkpath=true must create the missing intermediates + the empty tree)\n"
                  f"{r_on.diff_text}\n{r_on.notes}")
            ok = False

        # ---- mkpath = FALSE : single-level mkdir on <dest>/<base> fails (parent absent) -------
        # Same missing-intermediate dest. With no files in the source, nothing falls back to the
        # always-recursive file path, so NOTHING is created.
        dest_base_off = K.fresh_dest("mkpath_off_destbase")
        dest_off = os.path.join(dest_base_off, "miss_a", "miss_b")
        assert not os.path.exists(dest_off), "off-by-design: dest intermediate must start absent"
        r_off = H.run(backend, "cp", [src], dest_off,
                      file_collision=H.FileCollision.OVERWRITE,
                      folder_collision=H.FolderCollision.SKIP,
                      file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                      extra_options={"mkpath": "false"},
                      expect_dir=None, memcheck=memcheck)
        copied_off = os.path.join(dest_off, base)      # the would-be copied root
        # the copied root must NOT exist, AND the missing intermediate must still be absent:
        # mkdir(<dest>/<base>) could not even create its parents, so the whole path is gone.
        copied_off_absent = not os.path.exists(copied_off)
        intermediate_absent = not os.path.exists(dest_off)
        # the process must still finish + stay alive (the skipped mkdir error must not hang/crash)
        off_ok = (r_off.completed and r_off.stayed_alive and r_off.mem_errors == 0
                  and copied_off_absent and intermediate_absent)
        if not off_ok:
            print(f"      [off] not ok: completed={r_off.completed} alive={r_off.stayed_alive} "
                  f"mem_errors={r_off.mem_errors} copied_off_absent={copied_off_absent} "
                  f"intermediate_absent={intermediate_absent} "
                  f"(mkpath=false must FAIL to create a path with a missing parent)\n{r_off.notes}")
            ok = False

        # ---- divergence guard: on-state created the deep dir, off-state did not ---------------
        # If both behaved the same the option had no observable effect and the case is vacuous.
        diverged = deep_on_present and copied_off_absent
        if not diverged:
            print(f"      [{backend}] NO DIVERGENCE: option made no observable difference "
                  f"(deep_on_present={deep_on_present}, copied_off_absent={copied_off_absent})")
            ok = False

        return ok

    def iocp_one():
        """mkpath=true must RECONSTRUCT the empty-directory tree on NTFS, verified on the box.

        winlane's file-hash content diff lists files+symlinks only and is BLIND to empty dirs, so
        a bare run would falsely pass with nothing copied; the post_verify asserts the deepest
        empty directory is actually present under <dest>\\<base>. This is non-vacuous: it fails if
        empty-dir / TransferThread::mkpath creation ever breaks on Windows.

        The off-direction (single-level mkdir failing on a missing parent) cannot be forced on the
        IOCP lane: winlane always creates <dest> and copies into <dest>\\<base>, so a case cannot
        inject a missing-intermediate destination here. It is fully asserted on async + io_uring
        above; this is recorded honestly in the spec open_questions. Self-skips (PASS) without a
        [windows] host; REAL when one is configured."""
        from lib import winlane
        if not K.windows_host_configured():
            print("    [iocp] SKIP (no Windows host configured)")
            return True

        def _isdir(box, p) -> bool:
            out = box.ps(f"if (Test-Path -LiteralPath '{p}' -PathType Container) "
                         f"{{ Write-Output 'YES' }} else {{ Write-Output 'NO' }}")
            return "YES" in out.stdout

        def pv_on(box, dest, srcs):
            orig = winlane.win_join(dest, base)                 # <dest>\<base>
            deep = winlane.win_join(orig, DEEP_REL.replace("/", "\\"))
            second = winlane.win_join(orig, SECOND_REL.replace("/", "\\"))
            deep_ok = _isdir(box, deep)
            second_ok = _isdir(box, second)
            ok = deep_ok and second_ok
            return ok, (f"on: deep_dir={deep_ok} second_dir={second_ok} "
                        f"(mkpath=true must reconstruct the empty-dir tree on NTFS)")

        r_on = winlane.run_windows("cp", [src],
                                   file_collision=H.FileCollision.OVERWRITE,
                                   folder_collision=H.FolderCollision.SKIP,
                                   file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                                   extra_options={"mkpath": "true"},
                                   expect=None, post_verify=pv_on)
        if not r_on.ok:
            print(f"      [iocp/on] FAIL: completed={r_on.completed} alive={r_on.stayed_alive} "
                  f"content={r_on.content_ok} crashes={r_on.mem_errors} notes={r_on.notes}")
        return r_on.ok

    return K.for_option_backends(backends, one, iocp_one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
