#!/usr/bin/env python3
r"""Option verification: include / exclude FILTERS (the conf keys includeStrings,
includeOptions, excludeStrings, excludeOptions).

What the engine does with these (verified against the shipping code, not guessed):

  * CopyEngineFactory::resetOptions() (CopyEngineFactory.cpp:393-412) runs at startup via
    setResources(), reads each conf string and turns it into a stringlist with
    stringtostringlist() (cpp11addition.cpp:561 -- COMMA-separated, a literal comma escaped
    as ",,"). It hands them to Filters::setFilters(); getInstance() (line 147) then calls
    realObject->set_setFilters(...) so the rules reach the running ListThread/ScanFileOrFolder.
    => writing the four conf keys is enough to drive filters headlessly (no prod change),
       same mechanism as the other options (project_headless_options_take_effect).

  * Filters::setFilters / convertToRegex (Filters.cpp:19-81,264-311) builds ONE std::regex
    per rule. The per-rule "options" string (the includeOptions/excludeOptions entry, itself a
    ';'-separated token set) selects the search TYPE and scope:
        SearchType_rawText (default) | SearchType_simpleRegex | SearchType_perlRegex
        ApplyOn_fileAndFolder (default) | ApplyOn_file | ApplyOn_folder
    For SearchType_simpleRegex the search text is escaped and every '*' becomes "[^\\/]*",
    so a wildcard like "*.log" compiles to the regex  [^\\/]*\.log .

  * ScanFileOrFolder applies the rules with std::regex_match (FULL-string match) against the
    bare BASENAME of each scanned entry -- d_name from readdir / FindFirstFile, NOT a path
    (ScanFileOrFolder.cpp:743-826; d_name set at TransferThread.cpp:1523/1561). An EXCLUDE
    match drops the entry; with any INCLUDE rule present a file is kept ONLY if it matches an
    include rule (default-excluded). The top-level source folder itself is never filtered --
    only its contents are -- so the dest is dest/<basename(src)>/<surviving entries>.

We use SearchType_simpleRegex wildcards so the rule compiles to a regex that FULL-matches the
basename (a perlRegex like "\.log$" would NOT, because regex_match anchors both ends). Each
rule is a single token with no comma, so the ",,"-escaping of the stringlist encoding is not
in play and the conf value is exactly the rule text.

Observable on/off divergence (asserted in BOTH directions, never vacuous):
  * EXCLUDE "*.log": with the filter ON the .log files are ABSENT from the dest and every
    non-.log file is PRESENT; with the filter OFF (no keys) the SAME .log files ARE present.
    (Asserting "keep.txt exists" alone would pass regardless of the option -- so we assert the
    exact present-set AND the exact absent-set, and flip both between on and off.)
  * INCLUDE "*.keep": with the filter ON only the TOP-LEVEL .keep files survive and everything
    else is ABSENT -- including the nested sub/inner.keep, because the 'sub' folder itself does
    NOT match the include rule and so is never descended (a folder is kept only if its own name
    passes the include filter). With the filter OFF every file is present. Both sets checked both ways.

Runs on async + io_uring (in-process harness) and on IOCP via winlane (the filter is a
file-name regex on the basename -- fully applicable on NTFS, so iocp_one is wired, not None).
The IOCP lane builds the EXPECTED filtered tree locally and passes it as winlane expect=, with
the identical extra_options, so the box's copied tree is hash-compared against the filtered
expectation (a mismatch -- e.g. a .log leaking through, or a .keep dropped -- fails content_ok).
"""
import sys, pathlib, os
# Strip cases/ from sys.path so stdlib imports are not shadowed by sibling cases/*.py
# when this file is run directly (mirrors opt_perms_dates.py).
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

# ---- the source tree: a flat set of distinctive basenames + one nested dir ---------------
# Files chosen so each filter (below) cleanly partitions them into match / no-match sets.
SRC_FILES = {
    "app.log":          b"LOG-app\n",
    "server.log":       b"LOG-server\n",          # excluded by *.log ; not included by *.keep
    "notes.txt":        b"text notes content\n",
    "config.keep":      b"KEEP-config\n",         # included by *.keep ; not excluded by *.log
    "data.keep":        b"KEEP-data\n",
    "readme.md":        b"# readme\n",
    os.path.join("sub", "inner.txt"): b"nested non-matching\n",
    os.path.join("sub", "inner.keep"): b"KEEP-nested\n",
    os.path.join("sub", "trace.log"): b"LOG-nested\n",
}
# A multi-chunk (> blockSize) non-matching file, so a real read/write transfer is exercised
# for a SURVIVING file under both rules (proves filtering does not break normal copying).
BIG_REL = "payload.bin"
BIG_DATA = (b"BIGPAYLOAD" * 200000)   # ~2 MiB, has neither .log nor .keep -> survives exclude,
                                      # dropped by include (asserted accordingly)


def _build_src(root: str) -> None:
    import shutil
    if os.path.exists(root):
        shutil.rmtree(root)
    os.makedirs(os.path.join(root, "sub"), exist_ok=True)
    for rel, data in SRC_FILES.items():
        K.write_file(os.path.join(root, rel), data)
    K.write_file(os.path.join(root, BIG_REL), BIG_DATA)


def _all_rels() -> set:
    s = set(SRC_FILES.keys())
    s.add(BIG_REL)
    return s


# Basename-regex (simpleRegex wildcard) partitions of the file set ------------------------
def _matches_log(rel: str) -> bool:
    return os.path.basename(rel).endswith(".log")


def _matches_keep(rel: str) -> bool:
    return os.path.basename(rel).endswith(".keep")


def _dest_file_rels(copied_root: str) -> set:
    """Every regular FILE present under the copied tree, as src-relative POSIX paths."""
    out = set()
    for dirpath, _dirs, files in os.walk(copied_root):
        for nm in files:
            rel = os.path.relpath(os.path.join(dirpath, nm), copied_root)
            out.add(rel)
    return out


# Filter conf encodings (single rule each, no comma -> conf value == rule text verbatim).
EXCLUDE_OPTS = {
    "excludeStrings": "*.log",
    "excludeOptions": "SearchType_simpleRegex",
}
INCLUDE_OPTS = {
    "includeStrings": "*.keep",
    "includeOptions": "SearchType_simpleRegex",
}


def _expected_present(rule: str) -> set:
    """The src-relative file set that must survive `rule` ('exclude_log' | 'include_keep')."""
    allr = _all_rels()
    if rule == "exclude_log":
        return {r for r in allr if not _matches_log(r)}
    elif rule == "include_keep":
        # With an INCLUDE rule and the default ApplyOn_fileAndFolder, a sub-FOLDER is kept
        # only if ITS OWN name matches the include regex (ScanFileOrFolder.cpp:744-792).
        # 'sub' does NOT match '*.keep', so it is never descended and sub/inner.keep is never
        # even scanned -> only the TOP-LEVEL .keep files survive (nested .keep is ABSENT).
        return {r for r in allr if _matches_keep(r) and ("/" not in r and os.sep not in r)}
    elif rule == "off":
        return set(allr)
    raise ValueError(rule)


def _make_expect_tree(name: str, present_rels: set) -> str:
    """Materialise a local tree containing ONLY `present_rels` (with the source bytes), so the
    IOCP winlane lane can hash-compare the box's copied tree against the filtered expectation."""
    import shutil
    root = K.fresh_src_root(name)
    if os.path.exists(root):
        shutil.rmtree(root)
    os.makedirs(root, exist_ok=True)
    for rel in present_rels:
        data = BIG_DATA if rel == BIG_REL else SRC_FILES[rel]
        K.write_file(os.path.join(root, rel), data)
    return root


def run(backends=None, memcheck=H.NONE) -> bool:
    src = K.fresh_src_root("optfilters_src")
    _build_src(src)
    allr = _all_rels()

    def _check(copied_root: str, expect_present: set, label: str) -> bool:
        got = _dest_file_rels(copied_root)
        # normalise os.sep so the comparison is platform-neutral on the Linux side
        got = {p.replace("\\", "/") for p in got}
        want_present = {p.replace("\\", "/") for p in expect_present}
        want_absent = {p.replace("\\", "/") for p in (allr - expect_present)}
        missing = want_present - got                 # should have survived but did not
        leaked = want_absent & got                   # should have been filtered but is present
        ok = (not missing) and (not leaked)
        if not ok:
            print(f"      [{label}] filter set wrong:")
            if missing:
                print(f"        MISSING (should be present): {sorted(missing)}")
            if leaked:
                print(f"        LEAKED  (should be absent):  {sorted(leaked)}")
            print(f"        present-on-disk={sorted(got)}")
        return ok

    def one(backend):
        ok = True

        # ---- EXCLUDE "*.log" ON : .log files ABSENT, everything else PRESENT --------------
        dest = K.fresh_dest("optfilters_excl_on")
        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  expect_dir=None, memcheck=memcheck,
                  extra_options=dict(EXCLUDE_OPTS))
        copied = K.copied_root(dest, src)
        if not (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0):
            print(f"      [excl on] run not ok: completed={r.completed} alive={r.stayed_alive} "
                  f"oom={r.oom_killed} mem_errors={r.mem_errors}\n{r.notes}")
            ok = False
        if not _check(copied, _expected_present("exclude_log"), "excl on"):
            ok = False

        # ---- EXCLUDE OFF (no filter keys) : the SAME .log files ARE present ---------------
        dest_off = K.fresh_dest("optfilters_excl_off")
        r2 = H.run(backend, "cp", [src], dest_off,
                   file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.MERGE,
                   expect_dir=src, memcheck=memcheck)   # off: dest must equal source exactly
        copied_off = K.copied_root(dest_off, src)
        if not (r2.ok and r2.content_ok):
            print(f"      [excl off] run not ok: completed={r2.completed} alive={r2.stayed_alive} "
                  f"content={r2.content_ok} mem_errors={r2.mem_errors}\n{r2.diff_text}")
            ok = False
        if not _check(copied_off, _expected_present("off"), "excl off"):
            ok = False

        # ---- INCLUDE "*.keep" ON : ONLY .keep files PRESENT, everything else ABSENT -------
        dest_in = K.fresh_dest("optfilters_incl_on")
        r3 = H.run(backend, "cp", [src], dest_in,
                   file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.MERGE,
                   expect_dir=None, memcheck=memcheck,
                   extra_options=dict(INCLUDE_OPTS))
        copied_in = K.copied_root(dest_in, src)
        if not (r3.completed and r3.stayed_alive and not r3.oom_killed and r3.mem_errors == 0):
            print(f"      [incl on] run not ok: completed={r3.completed} alive={r3.stayed_alive} "
                  f"oom={r3.oom_killed} mem_errors={r3.mem_errors}\n{r3.notes}")
            ok = False
        if not _check(copied_in, _expected_present("include_keep"), "incl on"):
            ok = False

        # ---- INCLUDE OFF (no filter keys) : every file present (full source) --------------
        dest_in_off = K.fresh_dest("optfilters_incl_off")
        r4 = H.run(backend, "cp", [src], dest_in_off,
                   file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.MERGE,
                   expect_dir=src, memcheck=memcheck)
        copied_in_off = K.copied_root(dest_in_off, src)
        if not (r4.ok and r4.content_ok):
            print(f"      [incl off] run not ok: completed={r4.completed} alive={r4.stayed_alive} "
                  f"content={r4.content_ok} mem_errors={r4.mem_errors}\n{r4.diff_text}")
            ok = False
        if not _check(copied_in_off, _expected_present("off"), "incl off"):
            ok = False

        return ok

    def iocp_one():
        """IOCP: filters are a file-name regex on the BASENAME -> fully applicable on NTFS.
        Build the expected FILTERED trees locally and hash-compare them on the box (winlane
        expect=), once with the exclude rule ON and once with the include rule ON, both with
        the SAME extra_options the Linux lane uses. The OFF direction is the full-source diff
        already covered by other IOCP cases; here the on-state filtered expectation is the
        non-vacuous check (a leaked .log or a dropped .keep makes content_ok False)."""
        from lib import winlane
        if not K.windows_host_configured():
            print("    [iocp] SKIP (no Windows host configured)")
            return True

        ok = True
        # EXCLUDE *.log ON: expect only the non-.log files on the box.
        excl_expect = _make_expect_tree("optfilters_excl_expect",
                                        _expected_present("exclude_log"))
        re = winlane.run_windows("cp", [src],
                                 file_collision=H.FileCollision.OVERWRITE,
                                 folder_collision=H.FolderCollision.MERGE,
                                 expect=excl_expect, extra_options=dict(EXCLUDE_OPTS))
        if not re.ok:
            print(f"      [iocp excl] FAIL: completed={re.completed} alive={re.stayed_alive} "
                  f"content={re.content_ok} crashes={re.mem_errors} notes={re.notes}\n{re.diff_text}")
            ok = False

        # INCLUDE *.keep ON: expect only the .keep files on the box.
        incl_expect = _make_expect_tree("optfilters_incl_expect",
                                        _expected_present("include_keep"))
        ri = winlane.run_windows("cp", [src],
                                 file_collision=H.FileCollision.OVERWRITE,
                                 folder_collision=H.FolderCollision.MERGE,
                                 expect=incl_expect, extra_options=dict(INCLUDE_OPTS))
        if not ri.ok:
            print(f"      [iocp incl] FAIL: completed={ri.completed} alive={ri.stayed_alive} "
                  f"content={ri.content_ok} crashes={ri.mem_errors} notes={ri.notes}\n{ri.diff_text}")
            ok = False
        return ok

    return K.for_option_backends(backends, one, iocp_one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
