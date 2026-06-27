#!/usr/bin/env python3
r"""Option verification: ignoreBlackList  (registered key "ignoreBlackList", default "false").

WHAT THE OPTION DOES (read from the engine, ScanFileOrFolder.cpp -- the source of truth):

  * The engine keeps a `std::vector<INTERNALTYPEPATH> blackList` in ScanFileOrFolder.
  * `isBlackListed(destination)` returns early `false` when `ignoreBlackList` is true; otherwise
    it prefix-matches the (slash-normalised) DESTINATION path against every blackList entry
    (`stringStartWith`). A blacklisted DESTINATION folder makes the scan
    `emit errorOnFolder(destination, "Blacklisted folder", ErrorType_Folder)` -- i.e. it is
    handled by the folderError policy (Skip -> the folder, and everything under it, is NOT copied).
  * The list is keyed on the DESTINATION, not the source, and matched as a PREFIX.

WHERE THE LIST IS POPULATED -- THE DECISIVE FACT FOR TESTABILITY:

    ScanFileOrFolder::ScanFileOrFolder() populates blackList ONLY inside `#ifdef Q_OS_WIN32`:
        blackList.push_back("C:/Users/<user>/AppData/Roaming/");
        blackList.push_back("C:\\Users\\<user>\\AppData\\Roaming\\");
    There is NO non-Windows population anywhere (the only other reference,
    `//std::vector<std::string> blackList;` in ScanFileOrFolder.h, is commented out).

  => On Linux (async + io_uring) the blackList is EMPTY. `isBlackListed()` iterates 0 entries and
     ALWAYS returns false, so ignoreBlackList has NO observable on/off effect: every file is copied
     whether the option is true or false. Asserting "a file is skipped when off" on Linux would be
     a FALSE assertion (it would never be skipped) -- so this case does NOT pretend the option
     diverges on Linux. Instead it asserts the engine's ACTUAL Linux contract: the option is a
     no-op and, critically, NEVER drops files in EITHER state (a copy with ignoreBlackList=false
     must still produce the full tree -- a regression that ever populated a Linux blackList that
     dropped user data would be caught here).

  => On Windows (IOCP) the blackList has 2 REAL entries. The on/off divergence IS observable, but
     only when the DESTINATION is under `C:\Users\<user>\AppData\Roaming\`. winlane hard-codes the
     destination to `[paths] DESTINATIONWINDOWS\run-<id>\dst` and recursively deletes that sandbox
     on teardown -- so a real divergence run is possible ONLY if the operator deliberately points
     DESTINATIONWINDOWS at a throwaway dir UNDER AppData\Roaming. The IOCP lane therefore:
       - runs the real false->skip / true->copy divergence WHEN the configured Windows destination
         is under a blacklisted AppData\Roaming prefix (a genuine, non-vacuous assertion on the box);
       - otherwise SKIPs with PASS and a note (the divergence cannot be observed at the sandbox
         dest, and we must not fabricate it). This matches for_option_backends' "never vacuous when
         a host exists, but honestly skip what cannot be observed" contract.

This file asserts BOTH directions on every backend that can observe them, and proves the
Linux no-op by READING the engine (so the no-op assertion can't pass for the wrong reason: it
is anchored to the actual #ifdef-gated population, not just to "the file happened to be copied").
"""
import sys, pathlib, os
# Strip cases/ from sys.path so stdlib imports are not shadowed by a cases/*.py of the same
# name when this file is run directly (mirrors opt_perms_dates.py).
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

# The marker file we look for in the copied tree to prove "everything was copied".
MARKER_REL = "blkmarker.dat"
MARKER_DATA = b"ignoreBlackList round-trip marker " + b"Z" * 300


def _scan_source() -> pathlib.Path:
    return (pathlib.Path(__file__).resolve().parents[2] / "ScanFileOrFolder.cpp")


def _blacklist_is_windows_only() -> bool:
    """Read the engine and confirm EVERY executable `blackList.push_back(` sits inside a
    `#ifdef Q_OS_WIN32 ... #endif` guard (with no Q_OS_UNIX/else escape). This anchors the Linux
    'no-op' assertion to the real code: if a future change ever pushed a blackList entry on a
    non-Windows path, this returns False and the Linux assertion below FAILS loudly instead of
    silently passing for the wrong reason.

    We do a line-by-line preprocessor-conditional walk (a maintained stack of guard tokens) so we
    correctly handle the unrelated leading `#ifdef Q_OS_WIN32` windows.h-include block, the nested
    `#ifndef NOMINMAX`, and the `#ifdef WIDESTRING`/`#else` inside the constructor's WIN32 block.
    A push_back counts as Windows-gated iff 'Q_OS_WIN32' is on the active guard stack AND the
    nearest Q_OS_WIN32 frame has not been flipped by an `#else`."""
    txt = _scan_source().read_text(errors="replace")
    found_any = False
    # stack of dicts: {"win32": bool active-on-win32, "in_else": bool}
    stack = []

    def win32_active() -> bool:
        # active iff some frame is a Q_OS_WIN32 guard currently on its true (non-else) branch,
        # and no enclosing frame is currently on a false branch.
        any_win32 = False
        for fr in stack:
            if fr.get("dead"):
                return False
            if fr.get("win32") and not fr.get("in_else"):
                any_win32 = True
            if fr.get("win32") and fr.get("in_else"):
                # we are in the #else of a Q_OS_WIN32 guard -> NOT the windows branch
                return False
        return any_win32

    for raw in txt.splitlines():
        s = raw.strip()
        if s.startswith("#if"):
            is_win32 = ("Q_OS_WIN32" in s) and (s.startswith("#ifdef") or s.startswith("#if "))
            # a non-win32 #if that is definitely-false-on-windows is not our concern here;
            # we only track whether the WIN32 branch is the one a push_back would compile under.
            stack.append({"win32": is_win32, "in_else": False, "dead": False})
        elif s.startswith("#elif"):
            if stack:
                stack[-1]["in_else"] = True  # left the original branch
        elif s.startswith("#else"):
            if stack:
                stack[-1]["in_else"] = not stack[-1]["in_else"]
        elif s.startswith("#endif"):
            if stack:
                stack.pop()
        elif s.startswith("blackList.push_back("):
            found_any = True
            if not win32_active():
                return False
    # must have seen at least one (guards against a refactor that renamed the call and silently
    # left the Linux assertion asserting nothing).
    return found_any


def _make_src(root: str) -> str:
    """A small, fully-readable tree with a distinctive marker file. No fault markers: every entry
    MUST round-trip on Linux in BOTH option states (the option is a no-op there)."""
    import shutil
    if os.path.exists(root):
        shutil.rmtree(root)
    os.makedirs(os.path.join(root, "sub"), exist_ok=True)
    K.write_file(os.path.join(root, MARKER_REL), MARKER_DATA)
    K.write_file(os.path.join(root, "tiny.txt"), b"hello\n")
    K.write_file(os.path.join(root, "sub", "nested.bin"), b"N" * 4096)
    return root


def run(backends=None, memcheck=H.NONE) -> bool:
    src = _make_src(K.fresh_src_root("optblk_src"))
    base = os.path.basename(os.path.normpath(src))

    def one(backend):
        ok = True
        # The Linux assertion is only valid if the blackList really is Windows-only. Verify against
        # the engine source FIRST so the no-op claim can never pass for the wrong reason.
        if not _blacklist_is_windows_only():
            print("      [linux] blackList is NO LONGER Q_OS_WIN32-only -> this case's Linux "
                  "no-op assumption is invalid; the engine now blacklists on Linux and this test "
                  "must be rewritten to assert the real skip divergence. FAILING loudly.")
            return False

        # ---- ignoreBlackList = FALSE : on Linux the empty blackList means NOTHING is skipped ----
        dest_off = K.fresh_dest("optblk_dest_off")
        r_off = H.run(backend, "cp", [src], dest_off,
                      extra_options={"ignoreBlackList": "false"},
                      file_collision=H.FileCollision.OVERWRITE,
                      folder_collision=H.FolderCollision.MERGE,
                      folder_error=H.FolderError.SKIP,
                      expect_dir=src, memcheck=memcheck)
        copied_off = K.copied_root(dest_off, src)
        marker_off = os.path.join(copied_off, MARKER_REL)
        off_full = (os.path.exists(marker_off) and K.read_file(marker_off) == MARKER_DATA
                    and os.path.exists(os.path.join(copied_off, "sub", "nested.bin")))
        if not (r_off.ok and r_off.content_ok and off_full):
            print(f"      [off] not ok / file dropped on Linux (blackList must be empty -> nothing "
                  f"skipped): completed={r_off.completed} alive={r_off.stayed_alive} "
                  f"content={r_off.content_ok} full_tree={off_full} mem_errors={r_off.mem_errors}"
                  f"\n{r_off.diff_text}")
            ok = False

        # ---- ignoreBlackList = TRUE : identical result (the early-return in isBlackListed) -------
        dest_on = K.fresh_dest("optblk_dest_on")
        r_on = H.run(backend, "cp", [src], dest_on,
                     extra_options={"ignoreBlackList": "true"},
                     file_collision=H.FileCollision.OVERWRITE,
                     folder_collision=H.FolderCollision.MERGE,
                     folder_error=H.FolderError.SKIP,
                     expect_dir=src, memcheck=memcheck)
        copied_on = K.copied_root(dest_on, src)
        marker_on = os.path.join(copied_on, MARKER_REL)
        on_full = (os.path.exists(marker_on) and K.read_file(marker_on) == MARKER_DATA
                   and os.path.exists(os.path.join(copied_on, "sub", "nested.bin")))
        if not (r_on.ok and r_on.content_ok and on_full):
            print(f"      [on] not ok: completed={r_on.completed} alive={r_on.stayed_alive} "
                  f"content={r_on.content_ok} full_tree={on_full} mem_errors={r_on.mem_errors}"
                  f"\n{r_on.diff_text}")
            ok = False

        # On Linux both states are a no-op: the two destinations must be IDENTICAL and BOTH the
        # full source tree. This is the honest Linux contract (no divergence), anchored above to
        # the engine's Windows-only population so it can't be a false pass.
        if ok:
            print(f"      [{backend}] ignoreBlackList is a no-op on Linux (blackList empty per "
                  f"Q_OS_WIN32 gate); both states copied the full tree -- correct, no data dropped")
        return ok

    def iocp_one():
        """On Windows the blackList has 2 real entries keyed on the DESTINATION prefix
        C:\\Users\\<user>\\AppData\\Roaming\\. The on/off divergence is observable ONLY when the
        configured Windows destination lives under that prefix. winlane fixes the dest to
        DESTINATIONWINDOWS\\run-<id>\\dst and deletes it on teardown, so a real divergence run is
        possible only if the operator points DESTINATIONWINDOWS at a throwaway dir UNDER
        AppData\\Roaming. We run the REAL false->skip / true->copy divergence in that case, and
        SKIP=PASS (with a note) otherwise -- never fabricating a result we cannot observe."""
        from lib import winlane
        if not K.windows_host_configured():
            print("    [iocp] SKIP (no Windows host configured)")
            return True

        cfg = H.load_config()
        dest_root = cfg.get("paths", "DESTINATIONWINDOWS", fallback="").strip()
        # The destination ultracopier writes to is DESTINATIONWINDOWS\run-<id>\dst\<base>\... .
        # The blackList matches a prefix C:\Users\<user>\AppData\Roaming\ on the SLASH-normalised
        # destination, so the divergence is observable iff DESTINATIONWINDOWS is under that path.
        norm = dest_root.replace("\\", "/").lower()
        under_roaming = ("/appdata/roaming/" in norm) or norm.endswith("/appdata/roaming")
        if not under_roaming:
            print("    [iocp] SKIP=PASS: DESTINATIONWINDOWS is not under "
                  "C:\\Users\\<user>\\AppData\\Roaming\\, so the destination is NOT blacklisted "
                  "and the ignoreBlackList on/off divergence cannot be observed at the sandbox "
                  "dest. To exercise it, set [paths] DESTINATIONWINDOWS to a THROWAWAY dir under "
                  "AppData\\Roaming on the box (winlane wipes its run-<id> sandbox on teardown). "
                  "NEEDS_FAULT_INJECTION: the dest cannot be redirected from a case file alone "
                  "(run_windows hard-codes dest = DESTINATIONWINDOWS\\run-<id>\\dst).")
            return True

        # Real divergence on the box: the copy destination is under a blacklisted prefix.
        #  * ignoreBlackList=false + folderError=Skip -> the blacklisted dest folder is skipped:
        #    the marker file must be ABSENT on the box.
        #  * ignoreBlackList=true                     -> the copy proceeds: marker present + correct.
        def pv_off(box, dest, srcs):
            copied = winlane.win_join(dest, base)
            marker = winlane.win_join(copied, MARKER_REL)
            m = winlane.win_meta(box, marker)
            if m:  # file exists on the box -> it was NOT skipped -> false direction broke
                return False, ("ignoreBlackList=false did NOT skip the blacklisted dest: "
                               f"{marker} exists")
            return True, "ignoreBlackList=false skipped the blacklisted dest (marker absent) -- correct"

        def pv_on(box, dest, srcs):
            copied = winlane.win_join(dest, base)
            marker = winlane.win_join(copied, MARKER_REL)
            m = winlane.win_meta(box, marker)
            if not m:
                return False, ("ignoreBlackList=true did NOT copy into the blacklisted dest: "
                               f"{marker} missing")
            return True, "ignoreBlackList=true copied into the blacklisted dest (marker present) -- correct"

        # OFF: expect the marker to be skipped -> do NOT pass expect= (the tree is intentionally
        # partial); correctness is the post_verify (marker absent) + no crash/stay-alive.
        r_off = winlane.run_windows("cp", [src],
                                    extra_options={"ignoreBlackList": "false"},
                                    file_collision=H.FileCollision.OVERWRITE,
                                    folder_collision=H.FolderCollision.MERGE,
                                    folder_error=H.FolderError.SKIP,
                                    post_verify=pv_off)
        # ON: expect the full source tree to be reproduced AND the marker present.
        r_on = winlane.run_windows("cp", [src],
                                   extra_options={"ignoreBlackList": "true"},
                                   file_collision=H.FileCollision.OVERWRITE,
                                   folder_collision=H.FolderCollision.MERGE,
                                   folder_error=H.FolderError.SKIP,
                                   expect=src, post_verify=pv_on)
        if not r_off.ok:
            print(f"      [iocp off] FAIL: completed={r_off.completed} alive={r_off.stayed_alive} "
                  f"content={r_off.content_ok} crashes={r_off.mem_errors} notes={r_off.notes}")
        if not r_on.ok:
            print(f"      [iocp on] FAIL: completed={r_on.completed} alive={r_on.stayed_alive} "
                  f"content={r_on.content_ok} crashes={r_on.mem_errors} notes={r_on.notes}")
        return r_off.ok and r_on.ok

    return K.for_option_backends(backends, one, iocp_one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
