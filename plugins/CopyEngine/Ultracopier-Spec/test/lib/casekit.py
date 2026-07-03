#!/usr/bin/env python3
"""Shared helpers for the test cases under ../cases/.

Every case:
  * builds a synthetic source via synthtree.make_tree()
  * gets a fresh, wiped scratch destination from the harness config
    ([paths] DESTINATIONLINUX), optionally pre-populated for collision cases
  * iterates the requested backends and asserts on the harness Result.

These helpers keep each case file small and readable.
"""
from __future__ import annotations
import os, shutil, subprocess, pathlib
from . import harness as H

DEFAULT_BACKENDS = [H.ASYNC, H.IO_URING]

_FSOVERRIDE = pathlib.Path(__file__).resolve().parents[1] / "fsoverride"


def fs_so() -> str:
    """Build (if needed) the LD_PRELOAD filesystem-condition shim and return the
    absolute path to fsoverride/build/fs_preload.so.

    A case that wants to inject a filesystem condition does:

        os.environ["UC_FS_SCENARIO"] = "readfail:BADSECTOR"
        r = H.run(H.ASYNC, "cp", [src], dest,
                  file_error=H.FileError.PUT_TO_END,
                  fs_preload=K.fs_so(), ...)

    The harness sets LD_PRELOAD=<so> and forwards UC_FS_SCENARIO from os.environ,
    so the running ultracopier sees the injected source/dest I/O faults. Remember
    to clear/reset UC_FS_SCENARIO between sub-runs (it persists in os.environ)."""
    out = _FSOVERRIDE / "build" / "fs_preload.so"
    # Always run make: it is a no-op when the .so is already up to date, and it
    # rebuilds after an edit to linux/fs_preload.c.
    subprocess.run(["make", "-C", str(_FSOVERRIDE)], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    if not out.exists():
        raise RuntimeError(f"fs_preload.so was not built at {out}")
    return str(out)


def with_scenario(scenario: str) -> None:
    """Set UC_FS_SCENARIO in os.environ (the harness forwards it to the binary).
    Pass '' to disable. Convenience so cases don't poke os.environ directly."""
    os.environ["UC_FS_SCENARIO"] = scenario


def _scratch_root() -> pathlib.Path:
    cfg = H.load_config()
    root = cfg.get("paths", "DESTINATIONLINUX", fallback="/tmp/ultracopier-spec-test")
    p = pathlib.Path(os.path.abspath(root))
    assert p.is_absolute()
    return p


def _force_rmtree(path):
    """rmtree robust against HOSTILE-MODE leftovers (e.g. an unenterable mode-644 directory a
    previous run's engine left at the dest -- seen when a dir/file TYPE-mismatch collision gets
    the source FILE's permissions stamped onto a kept dest DIRECTORY): chmod the blocker and its
    parent to 0755 and retry, instead of failing the NEXT run's setup with PermissionError."""
    def _fix(p):
        try:
            os.chmod(os.path.dirname(p), 0o755)
        except OSError:
            pass
        try:
            os.chmod(p, 0o755)
        except OSError:
            pass
    def onexc(func, p, exc):
        _fix(p)
        func(p)          # retry once; a second failure propagates (real problem, surface it)
    try:
        shutil.rmtree(path, onexc=onexc)          # Python >= 3.12
    except TypeError:
        shutil.rmtree(path, onerror=lambda f, p, e: (_fix(p), f(p)))


def fresh_dest(name: str) -> str:
    """Return a wiped, freshly-created scratch destination directory (absolute)."""
    dest = _scratch_root() / name
    if dest.exists() or dest.is_symlink():
        if dest.is_dir() and not dest.is_symlink():
            _force_rmtree(dest)
        else:
            dest.unlink()
    dest.mkdir(parents=True, exist_ok=True)
    return str(dest)


def fresh_src_root(name: str) -> str:
    """Absolute path under the scratch root to hold a generated source tree."""
    return str(_scratch_root() / name)


def copied_root(dest: str, src: str) -> str:
    """ultracopier copies the source FOLDER into dest -> dest/<basename(src)>."""
    base = os.path.basename(os.path.normpath(src))
    cand = os.path.join(dest, base)
    return cand if os.path.isdir(cand) else dest


def write_file(path: str, data: bytes) -> None:
    p = pathlib.Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_bytes(data)


def read_file(path: str) -> bytes:
    return pathlib.Path(path).read_bytes()


# ---------------------------------------------------------------------------
# Metadata verification (perms + dates + symlink-ness), NOT just content.
#
# `diff -rq --no-dereference` (what Result.content_ok uses) checks file CONTENT and
# symlink TARGETS but is blind to permission bits and timestamps -- so metadata
# corruption (wrong mode, lost/garbled date) is invisible to it. These helpers close
# that gap for the per-option cases (doRightTransfer -> perms; the date-always-copied
# invariant; keepDate fault behaviour).
#
# IMPORTANT semantics (verified against the engine + the 3.0 baseline, see
# project_keepdate_semantics): the file DATE is ALWAYS copied for a regular file on all
# backends; `keepDate` only escalates a date *failure* to an error. So `mtime` should
# match the source by default regardless of keepDate. Permissions are copied ONLY when
# doRightTransfer is on; with it off the dest gets the process umask default.
# ---------------------------------------------------------------------------
MTIME_TOL_S = 2  # async uses second-granularity utimbuf; allow a small slop


def stat_meta(path: str) -> dict:
    """lstat-based metadata snapshot (does NOT follow symlinks)."""
    st = os.lstat(path)
    import stat as _stat
    m = {
        "mode": st.st_mode & 0o7777,
        "mtime": int(st.st_mtime),
        "mtime_ns": st.st_mtime_ns,
        "size": st.st_size,
        "is_symlink": _stat.S_ISLNK(st.st_mode),
        "is_dir": _stat.S_ISDIR(st.st_mode),
        "uid": st.st_uid,
        "gid": st.st_gid,
    }
    m["link_target"] = os.readlink(path) if m["is_symlink"] else None
    return m


def meta_mismatch(src: str, dst: str, *, check_mode=False, check_mtime=False,
                  expect_mode=None, mtime_tol_s=MTIME_TOL_S):
    """Return a human string describing the FIRST metadata mismatch between two paths,
    or None if they match under the requested checks. Symlinks: mode is never compared
    (lchmod is unportable); mtime is never compared (utime can't set a symlink's own
    time, and the engine intentionally skips date transfer for symlinks)."""
    s = stat_meta(src)
    d = stat_meta(dst)
    if s["is_symlink"] != d["is_symlink"]:
        return f"symlink-ness differs: src={s['is_symlink']} dst={d['is_symlink']}"
    if s["is_symlink"]:
        if s["link_target"] != d["link_target"]:
            return f"symlink target differs: src={s['link_target']!r} dst={d['link_target']!r}"
        return None
    if check_mode:
        want = s["mode"] if expect_mode is None else expect_mode
        if d["mode"] != want:
            return f"mode differs: dst={oct(d['mode'])} want={oct(want)} (src={oct(s['mode'])})"
    if check_mtime and not s["is_dir"]:
        if abs(d["mtime"] - s["mtime"]) > mtime_tol_s:
            return f"mtime differs: src={s['mtime']} dst={d['mtime']} (tol={mtime_tol_s}s)"
    return None


def verify_tree_meta(src_root: str, dst_root: str, *, check_mode=False,
                     check_mtime=False, expect_mode=None) -> list:
    """Walk src_root and compare each corresponding entry under dst_root. Returns a
    list of '<relpath>: <reason>' strings (empty == all good). Missing dst entries are
    reported too. Content correctness stays the harness's job (diff -rq); this is purely
    the metadata overlay."""
    problems = []
    for dirpath, dirnames, filenames in os.walk(src_root, followlinks=False):
        rel = os.path.relpath(dirpath, src_root)
        for name in list(dirnames) + filenames:
            srel = os.path.normpath(os.path.join(rel, name))
            sp = os.path.join(src_root, srel)
            dp = os.path.join(dst_root, srel)
            if not (os.path.exists(dp) or os.path.islink(dp)):
                problems.append(f"{srel}: MISSING in destination")
            else:
                why = meta_mismatch(sp, dp, check_mode=check_mode, check_mtime=check_mtime,
                                    expect_mode=expect_mode)
                if why:
                    problems.append(f"{srel}: {why}")
    return problems


def make_meta_tree(root: str) -> dict:
    """Create a small source tree with DISTINCTIVE perms + an OLD mtime on every entry,
    plus a symlink and a nested dir, so per-option metadata transfer is observable.
    Returns {relpath: mode} for the regular files (the modes a doRightTransfer=true copy
    must reproduce). All files get mtime 2001-02-03 04:05:06 UTC (981173106)."""
    import shutil as _sh
    if os.path.exists(root):
        _sh.rmtree(root)
    os.makedirs(os.path.join(root, "sub"), exist_ok=True)
    old = 981173106
    files = {
        "exec.sh": 0o755,
        "secret.dat": 0o600,
        "group.txt": 0o640,
        "odd.bin": 0o741,
        "sub/nested.dat": 0o604,
    }
    for rel, mode in files.items():
        p = os.path.join(root, rel)
        with open(p, "wb") as fh:
            fh.write(b"meta-" + rel.encode() + b"-" + b"Z" * 200)
        os.chmod(p, mode)
    # a relative symlink (its OWN metadata is not transferred; target must round-trip)
    link = os.path.join(root, "link_to_exec")
    if os.path.lexists(link):
        os.unlink(link)
    os.symlink("exec.sh", link)
    # set the old mtime on files + dirs (deepest first so dir mtimes stick)
    for rel in list(files) + ["sub", "."]:
        p = os.path.join(root, rel)
        try:
            os.utime(p, (old, old), follow_symlinks=True)
        except OSError:
            pass
    return files


def make_optrace_tree(root: str) -> tuple:
    """Create a representative source tree for the operation-decomposition tests (ops_integrity /
    ops_dependency): nested dirs, several small files, a 0-byte file and a MULTI-CHUNK file (so the
    write path emits several extents), plus a symlink. Returns (files, dirs):
      files: {relpath: size}   -- every regular file and the exact byte count it must write
      dirs : [relpath, ...]    -- every directory that must be mkdir'd on the destination
    Deterministic content (no randomness) so the trace is reproducible."""
    import shutil as _sh
    if os.path.exists(root):
        _sh.rmtree(root)
    dirs = ["sub", "sub/deep", "sub2"]
    for d in dirs:
        os.makedirs(os.path.join(root, d), exist_ok=True)
    files = {
        "a.txt": 6,
        "empty.dat": 0,
        "big.dat": 300 * 1024,          # multi-chunk: several 64 KiB write extents
        "sub/b.txt": 14,
        "sub/deep/c.txt": 21,
        "sub2/d.txt": 9,
    }
    payload = {
        "a.txt": b"hello\n",
        "empty.dat": b"",
        "big.dat": bytes(300 * 1024),
        "sub/b.txt": b"world of files",
        "sub/deep/c.txt": b"deeply nested content",
        "sub2/d.txt": b"sibling d",
    }
    for rel, data in payload.items():
        with open(os.path.join(root, rel), "wb") as fh:
            fh.write(data)
    # a relative symlink (target must round-trip; its own metadata is not transferred)
    link = os.path.join(root, "sub", "link_to_a")
    if os.path.lexists(link):
        os.unlink(link)
    os.symlink("../a.txt", link)
    return files, dirs


def run_traced(backend, mode, src, dest, files, dirs, **run_kwargs):
    """Run one cp/mv under the OPS-TRACE (LD_PRELOAD shim logs every basic FS op) and return
    (Result, optrace.Trace, expected_files, expected_dirs). `files`/`dirs` are the source-relative
    outputs of make_optrace_tree; they are mapped to their destination paths (dest/<basename(src)>/rel).
    The trace is written to a throwaway file; UC_FS_OPTRACE_PATH is set only around the run."""
    import tempfile
    from lib import optrace as OT
    trace = tempfile.mktemp(prefix="uc-optrace-", suffix=".txt")
    os.environ["UC_FS_OPTRACE_PATH"] = trace
    try:
        r = H.run(backend, mode, [src], dest, fs_preload=fs_so(), **run_kwargs)
    finally:
        os.environ.pop("UC_FS_OPTRACE_PATH", None)
    copied = copied_root(dest, src)
    expected_files = {os.path.join(copied, rel): size for rel, size in files.items()}
    expected_dirs = [copied] + [os.path.join(copied, d) for d in dirs]
    t = OT.Trace(trace, dest, src)
    return r, t, expected_files, expected_dirs


def windows_host_configured() -> bool:
    """True iff config.ini has a [windows] host -> the IOCP lane can actually run."""
    cfg = H.load_config()
    return cfg.has_section("windows") and bool(cfg.get("windows", "host", fallback="").strip())


def for_option_backends(backends, linux_one, iocp_one=None) -> bool:
    """Like for_backends, but ALSO drives IOCP so an option case can assert the option is
    honored on async/thread + io_uring + IOCP (the user's explicit requirement). For
    async/io_uring it calls linux_one(backend) -> bool. For IOCP it calls iocp_one() ->
    bool (which should run via winlane.run_windows and verify the option on the box). If
    iocp_one is None the option genuinely does not apply on Windows (POSIX-only) and IOCP
    is recorded as SKIP=PASS; a wired iocp_one self-skips (PASS) when no [windows] host is
    configured. Never weaken: a wired option must really run on the laptop when available."""
    backends = backends or DEFAULT_BACKENDS
    ok = True
    for b in backends:
        if b == H.IOCP:
            if iocp_one is None:
                print(f"    [{b}] SKIP (option is POSIX-only; not applicable on IOCP/NTFS)")
            else:
                try:
                    res = iocp_one()
                except Exception as e:
                    print(f"    [{b}] EXCEPTION: {e!r}")
                    ok = False
                else:
                    print(f"    [{b}] {'PASS' if res else 'FAIL'}")
                    ok = ok and bool(res)
        else:
            try:
                res = linux_one(b)
            except Exception as e:
                print(f"    [{b}] EXCEPTION: {e!r}")
                ok = False
            else:
                print(f"    [{b}] {'PASS' if res else 'FAIL'}")
                ok = ok and bool(res)
    return ok


def for_backends(backends, fn) -> bool:
    """Run fn(backend) for each backend; return True only if all return True.
    Prints a per-backend line so individual cases are debuggable on their own.

    IOCP is SKIPPED here (printed as SKIP, counts as PASS) because the async/io_uring cases
    drive the in-process harness.run(), which only supports async/io_uring. IOCP is covered
    end-to-end by cases/iocp_parity.py via the Windows SSH lane (winlane.run_windows)."""
    backends = backends or DEFAULT_BACKENDS
    ok = True
    for b in backends:
        if b == H.IOCP:
            print(f"    [{b}] SKIP (IOCP is covered by iocp_parity.py via the Windows lane)")
            continue
        try:
            res = fn(b)
        except Exception as e:           # a thrown assertion/exception == failure
            print(f"    [{b}] EXCEPTION: {e!r}")
            ok = False
        else:
            print(f"    [{b}] {'PASS' if res else 'FAIL'}")
            ok = ok and bool(res)
    return ok
