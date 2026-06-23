#!/usr/bin/env python3
"""Synthetic source-tree builder for the Ultracopier-Spec test suite.

`make_tree(root, profile)` builds a deterministic (seeded) tree on disk and returns
its absolute path. It is idempotent: the root is wiped first, so re-running a case
leaks nothing and always diffs reproducibly.

Profiles:
  * 'default'  -- a handful of files exercising the common edge cases:
        - a 0-byte file
        - a tiny text file
        - a ~few-KB file
        - a file just over 1 MiB (forces a multi-chunk pipeline read)
        - a nested directory  a/b/c  with a file at the bottom
        - a unicode-named file 'ünïcödé.txt'
  * 'symlinks' -- 'default' plus:
        - a relative symlink to a sibling file
        - a directory symlink
        - a DANGLING symlink pointing at /nonexistent
        - a symlink whose target escapes the tree ('../escape')
  * 'large'    -- N files totalling ~total_mb across nested dirs (memory case).
  * 'comprehensive' (alias 'all') -- EVERY kind and size of file: a size matrix straddling
        sector/page/64KiB/1MiB boundaries (0,1,17,511/512/513,4095/4096/4097,…,16MiB) + a
        configurable big multi-chunk file (big_mb), all-zeros / all-0xFF / CRLF content, a
        SPARSE file (8MiB hole), a HARDLINK pair, every symlink shape, 12-deep nesting, an
        empty dir + empty file, long/unicode/hostile names, and 0755/0444/0741 permission modes.
  * 'weird_names' -- files whose NAMES contain hostile-but-valid bytes (control
        chars, invalid UTF-8, embedded space/tab/newline, trailing dot/space,
        leading '-', shell metachars, a >200-char name). All valid on ext4 (only
        '/' and NUL are forbidden); must be copied AS-IS.
  * 'long_paths' -- length extremes the PathTree must store/resolve without truncation:
        a 255-byte (ext4 NAME_MAX) file name, a 255-byte directory name, a path nested
        deep enough that the cumulative length far exceeds 255 bytes (and 1 KiB) with
        every component <=255, and a 255-byte name at the bottom of that deep path.

Helper:
  * make_faulty_tree(root) -> (root, good_paths, bad_substr): a 'default' tree plus
        1-2 files whose names contain 'BADSECTOR', designated for read-fault
        injection by the LD_PRELOAD shim (failing-disk backup case).

All content is generated from a fixed seed so `diff` is byte-for-byte stable.
"""
from __future__ import annotations
import os, shutil, pathlib, random


def _wipe(root: str) -> None:
    p = pathlib.Path(root)
    # Defensive: only ever wipe an absolute path that looks like a scratch tree.
    assert p.is_absolute(), f"synthtree root must be absolute, got {root!r}"
    if p.exists() or p.is_symlink():
        if p.is_dir() and not p.is_symlink():
            shutil.rmtree(p)
        else:
            p.unlink()


def _seeded_bytes(seed: int, n: int) -> bytes:
    rnd = random.Random(seed)
    return bytes(rnd.getrandbits(8) for _ in range(n))


def _seeded_bytes_fast(seed: int, n: int) -> bytes:
    """Cheap deterministic large blob (random.Random.randbytes, seeded)."""
    return random.Random(seed).randbytes(n)


def _write(path: pathlib.Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def _build_default(root: pathlib.Path) -> None:
    root.mkdir(parents=True, exist_ok=True)
    # 0-byte file
    _write(root / "empty.bin", b"")
    # tiny text file
    _write(root / "tiny.txt", b"hello ultracopier\n")
    # a few-KB file (deterministic)
    _write(root / "few_kb.dat", _seeded_bytes(1, 4 * 1024))
    # a file just over 1 MiB -> multi-chunk pipeline read
    _write(root / "over_1mib.dat", _seeded_bytes_fast(2, 1024 * 1024 + 1234))
    # nested directory a/b/c with a file at the bottom
    _write(root / "a" / "b" / "c" / "deep.dat", _seeded_bytes(3, 777))
    # unicode-named file
    _write(root / "ünïcödé.txt", "café résumé déjà vu — ünïcödé\n".encode("utf-8"))


def _build_symlinks(root: pathlib.Path) -> None:
    _build_default(root)
    links = root / "links"
    links.mkdir(parents=True, exist_ok=True)
    # relative symlink to a sibling file (../tiny.txt is inside the tree)
    os.symlink("../tiny.txt", links / "rel_to_tiny")
    # directory symlink (points to the nested 'a' dir, relative)
    os.symlink("../a", links / "dir_link")
    # DANGLING symlink to a nonexistent absolute path
    os.symlink("/nonexistent/definitely/missing", links / "dangling")
    # symlink escaping the tree (target '../escape' resolves outside root)
    os.symlink("../../escape", links / "escape_link")


def _weird_name_bytes() -> list[bytes]:
    """Return a deterministic list of hostile-but-VALID ext4 filename byte strings.

    On a POSIX filesystem the ONLY bytes forbidden inside a name are '/' (0x2f) and
    NUL (0x00). Every other byte is storable and Ultracopier must copy it AS-IS,
    byte-for-byte. We therefore exercise: control chars, invalid/high UTF-8 bytes,
    embedded whitespace (space/tab/newline), trailing dot + trailing space, a leading
    '-', shell metacharacters, and a very long (>200 char) name. (Embedded NUL is NOT
    a storable on-disk case -- it is covered as an internal data-length invariant in
    weird_names.py, not as a created file.)
    """
    names = [
        # control chars 0x01..0x1f (skip 0x00; include them all in one name + singles)
        b"ctrl_" + bytes(range(0x01, 0x20)) + b"_end.dat",
        b"\x01\x02\x03leadctrl.dat",
        # high / invalid UTF-8 bytes 0x80..0xff (not valid UTF-8 -> must still round-trip)
        b"high_" + bytes(range(0x80, 0x100)) + b".dat",
        b"\xff\xfe\x80invalidutf8.dat",
        # embedded space and tab
        b"name with space.dat",
        b"name\twith\ttab.dat",
        # trailing dot and trailing space (Windows would strip these; ext4 keeps them)
        b"trailing_dot.",
        b"trailing_space ",
        # leading dash (looks like a CLI option)
        b"-leadingdash.dat",
        # shell metacharacters
        b"meta;&$'\"()|*?.dat",
        # a newline embedded in the name
        b"line1\nline2.dat",
        # a very long name (>200 chars, under the 255-byte ext4 limit)
        b"L" * 230 + b".dat",
    ]
    return names


def _build_weird_names(root: pathlib.Path) -> None:
    root.mkdir(parents=True, exist_ok=True)
    # A couple of normal files so the tree is not pathological-only.
    _write(root / "normal.txt", b"a normal sibling file\n")
    for i, nb in enumerate(_weird_name_bytes()):
        # Build the path at the bytes level: encode the parent, join with b'/', and
        # write via an os-level fd so Python never re-encodes/normalises the name.
        parent = os.fsencode(str(root))
        full = parent + b"/" + nb
        data = _seeded_bytes(5000 + i, 64 + i)   # deterministic, distinct per file
        fd = os.open(full, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
        try:
            os.write(fd, data)
        finally:
            os.close(fd)


# Sizes chosen to straddle every common boundary: 0, ±1 around sector (512), page (4 KiB),
# 64 KiB buffer, and the 1 MiB pipeline chunk, plus a few multi-chunk sizes.
_SIZES = [0, 1, 17, 511, 512, 513, 4095, 4096, 4097,
          65535, 65536, 65537,
          1024 * 1024 - 1, 1024 * 1024, 1024 * 1024 + 1,
          4 * 1024 * 1024, 16 * 1024 * 1024]


def _add_symlink_variety(dirp: pathlib.Path) -> None:
    """Self-contained set of every symlink shape into `dirp` (creates its own targets)."""
    dirp.mkdir(parents=True, exist_ok=True)
    _write(dirp / "target.txt", b"symlink target\n")
    (dirp / "subdir").mkdir(exist_ok=True); _write(dirp / "subdir" / "f.dat", b"x")
    os.symlink("target.txt", dirp / "rel_to_sibling")            # valid relative file
    os.symlink("subdir", dirp / "dir_link")                      # valid relative dir
    os.symlink("/etc/hostname", dirp / "abs_link")               # absolute (likely valid)
    os.symlink("/nonexistent/definitely/missing", dirp / "dangling")  # dangling
    os.symlink("../../escape", dirp / "escape_link")             # escapes the tree


def _build_comprehensive(root: pathlib.Path, *, big_mb: int = 48) -> None:
    """ANY kind and size of file -- the most complete profile. Exercises the full matrix:
    boundary-straddling sizes, content kinds, a sparse file, a hardlink pair, every symlink
    shape, deep nesting, an empty dir + empty file, long/unicode names, and permission modes."""
    root.mkdir(parents=True, exist_ok=True)

    # (1) SIZE MATRIX -- every boundary-straddling size, deterministic content.
    sizes = root / "sizes"
    for i, n in enumerate(_SIZES):
        _write(sizes / f"sz_{n}.dat", _seeded_bytes_fast(7000 + i, n) if n else b"")
    # one configurable big multi-chunk file (default ~48 MiB; raise for a stress run).
    _write(root / "big" / f"big_{big_mb}mib.bin",
           _seeded_bytes_fast(7777, big_mb * 1024 * 1024 + 123))

    # (2) CONTENT KINDS.
    kinds = root / "kinds"
    _write(kinds / "all_zeros_1mib.bin", b"\x00" * (1024 * 1024 + 7))
    _write(kinds / "all_ff_256k.bin", b"\xff" * (256 * 1024))
    _write(kinds / "text_crlf.txt", b"line1\r\nline2\r\nno final newline")
    # sparse file: an 8 MiB hole then 1 byte (du << apparent size; tests hole handling).
    sp = kinds / "sparse_8mib.bin"; sp.parent.mkdir(parents=True, exist_ok=True)
    with open(sp, "wb") as f:
        f.truncate(8 * 1024 * 1024); f.seek(8 * 1024 * 1024 - 1); f.write(b"Z")

    # (3) HARDLINK: two names, one inode.
    hl = root / "hardlinks"; hl.mkdir(parents=True, exist_ok=True)
    _write(hl / "original.dat", _seeded_bytes(8001, 5000))
    os.link(hl / "original.dat", hl / "hardlink_to_original.dat")

    # (4) SYMLINK variety.
    _add_symlink_variety(root / "links")

    # (5) STRUCTURE: deep nesting, empty dir, empty file.
    deep = root
    for d in range(12):
        deep = deep / f"lvl{d}"
    _write(deep / "bottom.dat", _seeded_bytes(8100, 321))
    (root / "empty_dir").mkdir(parents=True, exist_ok=True)
    _write(root / "empty_file.dat", b"")

    # (6) NAMES: long, unicode, one hostile-byte name.
    _write(root / ("L" * 230 + ".dat"), _seeded_bytes(8200, 99))
    _write(root / "ünïcödé_文件_😀.txt", "unicode name\n".encode("utf-8"))
    fd = os.open(os.fsencode(str(root)) + b"/name with space\tand tab.dat",
                 os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
    try: os.write(fd, b"hostile name\n")
    finally: os.close(fd)

    # (7) PERMISSION modes.
    perms = root / "perms"; perms.mkdir(parents=True, exist_ok=True)
    _write(perms / "exec.sh", b"#!/bin/sh\necho hi\n"); os.chmod(perms / "exec.sh", 0o755)
    _write(perms / "readonly.dat", _seeded_bytes(8300, 222)); os.chmod(perms / "readonly.dat", 0o444)
    _write(perms / "weird_mode.dat", _seeded_bytes(8301, 111)); os.chmod(perms / "weird_mode.dat", 0o741)


# A POSIX path COMPONENT (single name) maxes out at 255 bytes on ext4 (NAME_MAX); the
# full path maxes at ~4096 (PATH_MAX). The engine must store/resolve both extremes through
# the PathTree without truncation. We therefore build: a 255-byte (max) file name; a path
# nested DEEP enough that the cumulative path far exceeds 255 bytes (and 1 KiB) while every
# component stays <=255; and a 255-byte name AT the bottom of that deep path. Sizes are kept
# small (this is a path/length test, not a data-volume test). Names use only [A-Za-z0-9_]
# so they round-trip trivially and the focus stays on LENGTH, not hostile bytes (those are
# weird_names' job). The 255 limit is the ext4 max; "more than 255" for a single component
# is not filesystem-storable (ENAMETOOLONG) -- that internal case lives in the PathTree unit
# test, mirroring how the embedded-NUL invariant is split out of weird_names.
NAME_MAX_BYTES = 255


def _build_long_paths(root: pathlib.Path) -> None:
    root.mkdir(parents=True, exist_ok=True)
    # a couple of ordinary siblings so the tree is not pathological-only
    _write(root / "normal.txt", b"a normal sibling\n")

    # (1) a MAX-LENGTH (255-byte) file name at the root
    name255 = ("n" * (NAME_MAX_BYTES - len(".dat"))) + ".dat"
    assert len(name255.encode()) == NAME_MAX_BYTES
    _write(root / name255, _seeded_bytes(6001, 321))

    # (2) a MAX-LENGTH (255-byte) DIRECTORY name with a file inside it
    dir255 = "d" * NAME_MAX_BYTES
    assert len(dir255.encode()) == NAME_MAX_BYTES
    _write(root / dir255 / "inside.dat", _seeded_bytes(6002, 222))

    # (3) a DEEP path whose cumulative length far exceeds 255 (and 1 KiB) -- 30 levels of
    #     40-byte component names ~= 1230 bytes, every component well under NAME_MAX. Keep
    #     the destination under PATH_MAX: dest_root(~40) + base(~9) + this(~1230) << 4096.
    deep = root
    for lvl in range(30):
        comp = f"lvl{lvl:02d}_" + ("x" * 34)        # 5 + 34 = 39 bytes, <=255
        assert len(comp.encode()) <= NAME_MAX_BYTES
        deep = deep / comp
    # (4) ...and a 255-byte name AT the bottom of that deep path (deep path + max name)
    _write(deep / name255, _seeded_bytes(6003, 159))
    _write(deep / "bottom.dat", _seeded_bytes(6004, 77))


def _build_iocp(root: pathlib.Path) -> None:
    """IOCP/Windows lane profile: regular files only, ASCII names, that stage cleanly over scp to a
    Windows box (NO symlinks -> can't scp; NO >MAX_PATH names; NO unicode/control-byte names -> scp
    mangles them and Windows can't open them). Still exercises the things the PathTree + Core
    list-refresh changes care about: many files in nested dirs (list churn), 0-byte/tiny/few-KB/
    multi-chunk sizes (the >1 MiB pipeline path), and a deeper tree. Content is seeded -> byte-stable."""
    root.mkdir(parents=True, exist_ok=True)
    _write(root / "empty.bin", b"")
    _write(root / "tiny.txt", b"hello ultracopier\n")
    _write(root / "few_kb.dat", _seeded_bytes(1, 4 * 1024))
    _write(root / "over_1mib.dat", _seeded_bytes_fast(2, 1024 * 1024 + 1234))  # multi-chunk pipeline
    _write(root / "boundary_64k.dat", _seeded_bytes_fast(3, 65536 + 1))
    # a handful of nested dirs with several small files each -> the structural list view churns,
    # which is exactly what the Core 100ms list-refresh cap paces.
    for d in range(6):
        for f in range(8):
            _write(root / f"dir{d:02d}" / f"file{f:02d}.dat", _seeded_bytes(100 + d * 10 + f, 64 + f))
    _write(root / "a" / "b" / "c" / "deep.dat", _seeded_bytes(9, 777))


# Bytes Windows forbids in a FILENAME (Win32 namespace): the 0x01-0x1f control range plus
# " < > : | ? * and backslash/slash/NUL. NTFS can store most of them but the Win32 API Ultracopier
# uses (CreateFileW) cannot open them, so a round-trip through Windows is impossible -> they are
# POSIX-only hostile names and excluded from the IOCP parity profile.
_WIN_FORBIDDEN = set(range(0x00, 0x20)) | {ord(c) for c in '<>:"|?*\\/'}


def _win_safe_weird_name_bytes() -> list[bytes]:
    """Hostile-but-WINDOWS-STORABLE filename byte strings for the IOCP parity profile.

    Windows stores filenames as UTF-16, so a name MUST be valid UTF-8/UTF-16-translatable.
    Excluded (genuinely unstorable on NTFS, NOT a Ultracopier bug): the raw 0x80-0xff byte
    range (invalid UTF-8 -- cannot be decoded to a UTF-16 name), control chars 0x01-0x1f,
    tab/newline, and < > : " | ? * \\ / . What IS kept and must round-trip byte-for-byte:
    a leading dash, shell metacharacters that are Windows-legal (; & $ ' ( )), a trailing
    dot (NTFS keeps it; Win32 strips on write -- the engine must preserve the source name),
    a long (200-byte, under MAX_PATH-component 255) name, and embedded space."""
    names = [
        # embedded space (no tab -- Windows forbids it)
        b"name with space.dat",
        # trailing dot (NTFS keeps; Win32 would strip -- engine must preserve the source name)
        b"trailing_dot.",
        # leading dash (looks like a CLI option)
        b"-leadingdash.dat",
        # shell metacharacters that are Windows-legal
        b"meta;&$'()dat",
        # a long name (200 bytes, under the 255 component limit and far under MAX_PATH)
        b"W" * 200 + b".dat",
    ]
    # defensive: make sure none slipped through a Windows-forbidden byte
    for nb in names:
        assert not any(b in _WIN_FORBIDDEN for b in nb), f"win-safe name has forbidden byte: {nb!r}"
    return names


def _build_comprehensive_win(root: pathlib.Path, *, big_mb: int = 16) -> None:
    """The Windows-compatible COMPREHENSIVE profile for the IOCP feature-parity case.

    Mirrors _build_comprehensive but keeps ONLY things Windows can store + tar can stage:
      - the size matrix (every boundary-straddling size) + a big multi-chunk file
      - content kinds (zeros, 0xff, CRLF text, sparse)
      - hardlink pair (NTFS supports hardlinks)
      - symlink variety (Windows 10 supports symlinks; IOCP trySymlinkCopy recreates them)
      - deep nesting + empty dir + empty file
      - long + unicode + Windows-safe hostile-byte names (NO tab/newline/control/< > : " | ? *)
      - permission modes (Windows maps readonly; the rest are POSIX-only but harmless)

    The big file is smaller (16 MiB) than the Linux comprehensive (48) because it crosses
    a slow SSH link to stage. Content is seeded -> byte-stable for hash diff."""
    root.mkdir(parents=True, exist_ok=True)

    # (1) SIZE MATRIX -- every boundary-straddling size, deterministic content.
    sizes = root / "sizes"
    for i, n in enumerate(_SIZES):
        _write(sizes / f"sz_{n}.dat", _seeded_bytes_fast(7000 + i, n) if n else b"")
    _write(root / "big" / f"big_{big_mb}mib.bin",
           _seeded_bytes_fast(7777, big_mb * 1024 * 1024 + 123))

    # (2) CONTENT KINDS.
    kinds = root / "kinds"
    _write(kinds / "all_zeros_1mib.bin", b"\x00" * (1024 * 1024 + 7))
    _write(kinds / "all_ff_256k.bin", b"\xff" * (256 * 1024))
    _write(kinds / "text_crlf.txt", b"line1\r\nline2\r\nno final newline")
    sp = kinds / "sparse_8mib.bin"; sp.parent.mkdir(parents=True, exist_ok=True)
    with open(sp, "wb") as f:
        f.truncate(8 * 1024 * 1024); f.seek(8 * 1024 * 1024 - 1); f.write(b"Z")

    # (3) HARDLINK: two names, one inode (NTFS supports hardlinks).
    hl = root / "hardlinks"; hl.mkdir(parents=True, exist_ok=True)
    _write(hl / "original.dat", _seeded_bytes(8001, 5000))
    os.link(hl / "original.dat", hl / "hardlink_to_original.dat")

    # (4) SYMLINK variety. Windows 10 (developer mode or admin) can create symlinks; IOCP's
    # trySymlinkCopy recreates them. Use RELATIVE targets only (abs Linux paths are meaningless
    # on Windows and a dangling abs link would not round-trip its target string cleanly).
    links = root / "links"; links.mkdir(parents=True, exist_ok=True)
    _write(links / "target.txt", b"symlink target\n")
    (links / "subdir").mkdir(exist_ok=True); _write(links / "subdir" / "f.dat", b"x")
    os.symlink("target.txt", links / "rel_to_sibling")
    os.symlink("subdir", links / "dir_link")
    os.symlink("nonexistent_missing", links / "dangling")   # relative dangling (target string preserved)

    # (5) STRUCTURE: deep nesting (under MAX_PATH), empty dir, empty file.
    deep = root
    for d in range(8):
        deep = deep / f"lvl{d}"
    _write(deep / "bottom.dat", _seeded_bytes(8100, 321))
    (root / "empty_dir").mkdir(parents=True, exist_ok=True)
    _write(root / "empty_file.dat", b"")

    # (6) NAMES: long (under 255-component), unicode, Windows-safe hostile bytes.
    _write(root / ("W" * 200 + ".dat"), _seeded_bytes(8200, 99))
    _write(root / "ünïcödé_文件_😀.txt", "unicode name\n".encode("utf-8"))
    for i, nb in enumerate(_win_safe_weird_name_bytes()):
        parent = os.fsencode(str(root))
        full = parent + b"/" + nb
        fd = os.open(full, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
        try: os.write(fd, _seeded_bytes(8300 + i, 64 + i))
        finally: os.close(fd)

    # (7) PERMISSION modes (readonly maps to Windows ReadOnly attribute; exec/weird are POSIX-only
    #     but harmless -- Windows ignores the POSIX mode bits on copy).
    perms = root / "perms"; perms.mkdir(parents=True, exist_ok=True)
    _write(perms / "exec.sh", b"#!/bin/sh\necho hi\n"); os.chmod(perms / "exec.sh", 0o755)
    _write(perms / "readonly.dat", _seeded_bytes(8300, 222)); os.chmod(perms / "readonly.dat", 0o444)
    _write(perms / "weird_mode.dat", _seeded_bytes(8301, 111)); os.chmod(perms / "weird_mode.dat", 0o741)


def _build_large(root: pathlib.Path, total_mb: int) -> None:
    root.mkdir(parents=True, exist_ok=True)
    # Spread ~total_mb across nested dirs in fixed-size chunks so peak RSS is bounded
    # by the engine's pipeline, not by any single huge file.
    per_file_mb = 8
    n_files = max(1, (total_mb + per_file_mb - 1) // per_file_mb)
    for i in range(n_files):
        sub = root / f"d{i // 8}"      # 8 files per sub-directory
        path = sub / f"chunk_{i:04d}.dat"
        _write(path, _seeded_bytes_fast(1000 + i, per_file_mb * 1024 * 1024))


def make_tree(root: str, profile: str = "default", *, total_mb: int = 400, big_mb: int = 48) -> str:
    """Build the synthetic source tree at `root`; return the absolute root path."""
    root = os.path.abspath(root)
    _wipe(root)
    rp = pathlib.Path(root)
    if profile == "default":
        _build_default(rp)
    elif profile == "symlinks":
        _build_symlinks(rp)
    elif profile == "large":
        _build_large(rp, total_mb)
    elif profile == "weird_names":
        _build_weird_names(rp)
    elif profile == "long_paths":
        _build_long_paths(rp)
    elif profile == "iocp":
        _build_iocp(rp)
    elif profile == "comprehensive_win":
        _build_comprehensive_win(rp, big_mb=big_mb)
    elif profile in ("comprehensive", "all"):
        _build_comprehensive(rp, big_mb=big_mb)
    else:
        raise ValueError(f"unknown synthtree profile: {profile!r}")
    return root


def make_faulty_tree(root: str):
    """Build a healthy 'default' tree PLUS a small number of files designated for
    fault injection (their names contain the substring 'BADSECTOR'), and return:

        (root, good_paths, bad_substr)

    * root        -- absolute path of the built tree (same as make_tree).
    * good_paths  -- list of absolute paths of the GOOD (fully readable) files; these
                     must be backed up byte-for-byte even when the bad files fault.
    * bad_substr  -- the substring ('BADSECTOR') a UC_FS_SCENARIO read-fault verb keys
                     off (e.g. 'readfail:BADSECTOR' or 'eio_after:BADSECTOR:65536').

    Two designated files are created so a scenario can target one (readfail) and one
    partway-failure (eio_after) while the rest of the tree stays healthy. The bad
    files have real, deterministic content on disk; the FAULT is injected by the
    LD_PRELOAD shim at read() time, not by writing corrupt bytes here.
    """
    root = os.path.abspath(root)
    _wipe(root)
    rp = pathlib.Path(root)
    _build_default(rp)
    bad_substr = "BADSECTOR"
    # A fully-dead file (readfail) and a part-readable file (eio_after) -- both >1 MiB
    # so the failure happens mid-pipeline, not on a single tiny read.
    bad1 = rp / "movie_BADSECTOR_dead.bin"
    bad2 = rp / "archive_BADSECTOR_partial.bin"
    _write(bad1, _seeded_bytes_fast(9001, 1024 * 1024 + 4096))
    _write(bad2, _seeded_bytes_fast(9002, 1024 * 1024 + 4096))

    # Enumerate the GOOD files = everything we just built that is NOT a bad file.
    good_paths = []
    for dirpath, _dirs, files in os.walk(root):
        for fn in files:
            p = os.path.join(dirpath, fn)
            if bad_substr not in fn:
                good_paths.append(p)
    good_paths.sort()
    return root, good_paths, bad_substr
