#!/usr/bin/env python3
"""optrace.py -- parse + validate the fs_preload OPS-TRACE, for the operation-decomposition tests.

The LD_PRELOAD shim (fsoverride/linux/fs_preload.c) appends one line per basic FS operation the
engine performs to UC_FS_OPTRACE_PATH:

    <seq>\\t<tid>\\t<verb>\\t<path>\\t<a1>\\t<a2>

    verb   a1      a2
    OPEN   fd      flags        READ  offset  bytes      WRITE offset bytes
    CLOSE  fd      -            MKDIR rc      mode        CHMOD mode   rc
    UTIME  rc      -            TRUNC len     rc          UNLINK/RMDIR/RENAME/SYMLINK rc -

This module turns that trace into a small model (per-file open..close lifecycles keyed by the
dest/source path) and offers three validators matching the user's requirements:

  * problems_no_duplicate() -- no double mkdir / double create-open / overlapping write of the same
    dest byte range / double set-date / double set-perm (an op done twice = wasted or, worse, a
    double copy).
  * problems_no_missing(expected_files, expected_dirs) -- every dest file got a create-open + its
    full-size writes + a close, and every needed dir got a mkdir (nothing was silently dropped).
  * problems_dependency() -- the hard FS ordering edges hold under ANY reorder the scheduler chose:
    create-open < writes < close; no data op after close; a file's perm/date only AFTER its close;
    a FOLDER's date/perm only AFTER every file (and subdir) under it is closed.

SCOPE: the shim interposes libc, so it sees the ASYNC data plane in FULL (open/read/write/close) plus
the shared metadata path (mkdir/chmod/utime) of ALL backends. The io_uring/IOCP DATA + open ops go
through the ring / completion port and are INVISIBLE here -- so on those backends only the
metadata-subset checks apply (pass check_data=False). Async is the reference backend where the whole
decomposition is validated end-to-end.
"""
import os

O_CREAT = os.O_CREAT
O_ACCMODE = os.O_ACCMODE
O_RDONLY = os.O_RDONLY
O_WRONLY = os.O_WRONLY
O_RDWR = os.O_RDWR


class Event:
    __slots__ = ("seq", "tid", "verb", "path", "a1", "a2")

    def __init__(self, seq, tid, verb, path, a1, a2):
        self.seq = seq
        self.tid = tid
        self.verb = verb
        self.path = path
        self.a1 = a1
        self.a2 = a2

    def __repr__(self):
        return f"#{self.seq} t{self.tid} {self.verb} {self.path} {self.a1} {self.a2}"


class FileLife:
    """One open..close segment on a single path (a path can have several over a run)."""
    __slots__ = ("path", "open_seq", "flags", "is_create", "accmode",
                 "is_write", "is_read", "close_seq", "writes", "reads")

    def __init__(self, path, open_seq, flags):
        self.path = path
        self.open_seq = open_seq
        self.flags = flags
        self.is_create = bool(flags & O_CREAT)
        self.accmode = flags & O_ACCMODE
        self.is_write = self.accmode in (O_WRONLY, O_RDWR)
        self.is_read = self.accmode == O_RDONLY
        self.close_seq = None
        self.writes = []   # list[(seq, off, length)]
        self.reads = []    # list[(seq, off, length)]

    def bytes_written(self):
        return sum(l for (_s, _o, l) in self.writes)


def _norm(p):
    return os.path.normpath(p)


def _under(path, root):
    """True if `path` is `root` itself or lives under it (normalized)."""
    root = _norm(root)
    p = _norm(path)
    return p == root or p.startswith(root + os.sep)


def parse(trace_path):
    """Read the trace file into a seq-ordered list[Event]. Missing file -> []."""
    events = []
    try:
        with open(trace_path, "r", errors="replace") as f:
            for line in f:
                parts = line.rstrip("\n").split("\t")
                if len(parts) != 6:
                    continue
                try:
                    seq = int(parts[0]); tid = int(parts[1])
                    a1 = int(parts[4]); a2 = int(parts[5])
                except ValueError:
                    continue
                events.append(Event(seq, tid, parts[2], parts[3], a1, a2))
    except OSError:
        return []
    events.sort(key=lambda e: e.seq)
    return events


class Trace:
    """Parsed + filtered op trace, with the per-path lifecycle model built once."""

    def __init__(self, trace_path, dest_root, src_root=None):
        self.dest_root = _norm(dest_root)
        self.src_root = _norm(src_root) if src_root else None
        allev = parse(trace_path)
        # Keep only ops we can attribute to the copy: paths under dest (writes/mkdir/meta) or
        # source (reads). Everything else (Qt conf, fonts, /proc, the trace file itself) is noise.
        self.events = [e for e in allev if self._relevant(e.path)]
        self._build()

    def _relevant(self, path):
        if _under(path, self.dest_root):
            return True
        if self.src_root is not None and _under(path, self.src_root):
            return True
        return False

    def _is_dest(self, path):
        return _under(path, self.dest_root)

    def _build(self):
        self.segments = []               # all FileLife (open..close), in open order
        self._fd_open = {}               # fd -> current FileLife
        self._open_by_path = {}          # path -> [FileLife,...] currently open (stack)
        self.orphan_data = []            # (Event) read/write with no active open for its path
        self.mkdirs = []                 # (Event) MKDIR
        self.chmods = []                 # (Event) CHMOD
        self.utimes = []                 # (Event) UTIME
        self.truncs = []
        self.unlinks = []
        self.renames = []
        self.symlinks = []
        self.falloc = []                 # (Event) FALLOC (best-effort destination preallocation)
        for e in self.events:
            if e.verb == "OPEN":
                seg = FileLife(e.path, e.seq, e.a2)
                self._fd_open[e.a1] = seg
                self._open_by_path.setdefault(e.path, []).append(seg)
                self.segments.append(seg)
            elif e.verb == "CLOSE":
                seg = self._fd_open.pop(e.a1, None)
                if seg is not None:
                    seg.close_seq = e.seq
                    stk = self._open_by_path.get(seg.path)
                    if stk and seg in stk:
                        stk.remove(seg)
            elif e.verb in ("READ", "WRITE"):
                stk = self._open_by_path.get(e.path)
                if stk:
                    seg = stk[-1]
                    (seg.writes if e.verb == "WRITE" else seg.reads).append((e.seq, e.a1, e.a2))
                else:
                    self.orphan_data.append(e)   # data op with NO open in effect -> after close / never opened
            elif e.verb == "MKDIR":
                self.mkdirs.append(e)
            elif e.verb == "CHMOD":
                self.chmods.append(e)
            elif e.verb == "UTIME":
                self.utimes.append(e)
            elif e.verb == "TRUNC":
                self.truncs.append(e)
            elif e.verb == "UNLINK":
                self.unlinks.append(e)
            elif e.verb == "RENAME":
                self.renames.append(e)
            elif e.verb == "SYMLINK":
                self.symlinks.append(e)
            elif e.verb == "FALLOC":
                self.falloc.append(e)

    # ---- derived views ---------------------------------------------------------------

    def dest_create_segments(self):
        """FileLife segments that CREATED a dest file (O_CREAT + writable)."""
        return [s for s in self.segments if self._is_dest(s.path) and s.is_create and s.is_write]

    def dest_write_segments_by_path(self):
        d = {}
        for s in self.segments:
            if self._is_dest(s.path) and s.is_write:
                d.setdefault(s.path, []).append(s)
        return d

    def prealloc_paths(self):
        """dest paths that received a best-effort preallocation (FALLOC op) -> {path: (len, rc)}.
        len == a1 (bytes reserved), rc == a2 (0 ok; nonzero = the FS skipped it, still best-effort).
        io_uring's fallocate is libc-visible; io_uring/IOCP ring reads/writes are not."""
        out = {}
        for e in self.falloc:
            if self._is_dest(e.path):
                out[e.path] = (e.a1, e.a2)
        return out

    def successful_mkdir_paths(self):
        """dest dir paths with at least one rc==0 mkdir (a1==rc)."""
        return {e.path for e in self.mkdirs if self._is_dest(e.path) and e.a1 == 0}

    def dest_file_paths(self):
        """dest paths that look like regular files (had a create-write open)."""
        return {s.path for s in self.dest_create_segments()}

    def last_close_seq(self, path):
        cs = [s.close_seq for s in self.segments if s.path == path and s.close_seq is not None]
        return max(cs) if cs else None

    # ---- validators ------------------------------------------------------------------

    def problems_no_duplicate(self, check_data=True):
        """Duplicate == the SAME logical work done twice, i.e. wasted or (worse) a double copy:
        a dir created twice, a file copied (create-opened / written to the same range) twice, a
        FILE's perm/date set twice. NB a FOLDER's date/perm being set more than once is NOT a
        duplicate here -- the engine legitimately touches a folder at creation and re-asserts it
        after its files finish (see problems_dependency, which checks the FINAL touch is correct)."""
        probs = []
        # 1. no dest dir created (rc==0 mkdir) more than once. rc!=0 (EEXIST re-probe) is the
        #    per-file "ensure parent exists" MkPath does and is NOT a duplicate create.
        seen = {}
        for e in self.mkdirs:
            if self._is_dest(e.path) and e.a1 == 0:
                seen[e.path] = seen.get(e.path, 0) + 1
        for p, n in seen.items():
            if n > 1:
                probs.append(f"duplicate mkdir ({n}x) of {p}")
        # 2. no dest file create-opened more than once (would mean a double copy)
        create_count = {}
        for s in self.dest_create_segments():
            create_count[s.path] = create_count.get(s.path, 0) + 1
        for p, n in create_count.items():
            if n > 1:
                probs.append(f"duplicate create-open ({n}x) of dest file {p}")
        # 3. a FILE's set-date / set-perm applied more than once (files get exactly one of each;
        #    folders are exempt -- see the docstring).
        file_paths = self.dest_file_paths()
        ut = {}
        for e in self.utimes:
            if e.path in file_paths and e.a1 == 0:
                ut[e.path] = ut.get(e.path, 0) + 1
        for p, n in ut.items():
            if n > 1:
                probs.append(f"duplicate set-date ({n}x) of file {p}")
        cm = {}
        for e in self.chmods:
            if e.path in file_paths and e.a2 == 0:
                cm[e.path] = cm.get(e.path, 0) + 1
        for p, n in cm.items():
            if n > 1:
                probs.append(f"duplicate set-perm ({n}x) of file {p}")
        # 4. (data plane) no overlapping write of the same dest byte range (a double write)
        if check_data:
            for p, segs in self.dest_write_segments_by_path().items():
                extents = []
                for s in segs:
                    for (_seq, off, length) in s.writes:
                        if length > 0:
                            extents.append((off, off + length))
                extents.sort()
                prev_end = -1
                for (a, b) in extents:
                    if a < prev_end:
                        probs.append(f"overlapping write of {p}: [{a},{b}) overlaps a prior write "
                                     f"ending at {prev_end}")
                    if b > prev_end:
                        prev_end = b
        return probs

    def renamed_dest_paths(self):
        """dest paths produced by a successful rename (the same-FS MOVE fast-path: rename replaces
        copy+delete, so such a file has NO open/write/close -- it must still not be 'forgotten')."""
        return {_norm(e.path) for e in self.renames if self._is_dest(e.path) and e.a1 == 0}

    def problems_no_missing(self, expected_files, expected_dirs, check_data=True):
        """expected_files: {dest_abs_path: size}. expected_dirs: iterable of dest_abs_path.
        On check_data=False (io_uring/IOCP) only the dir-mkdir presence is asserted (files' open/
        write/close are invisible through the ring). A same-FS move may `rename()` a file into place
        instead of copying it -- that file is produced (not forgotten) but has no byte writes, so it
        is accepted via the rename set and its size is not re-checked (rename is atomic)."""
        probs = []
        made = self.successful_mkdir_paths()
        for d in expected_dirs:
            if _norm(d) not in made:
                probs.append(f"missing mkdir for expected dir {d}")
        if check_data:
            by_path = self.dest_write_segments_by_path()
            renamed = self.renamed_dest_paths()
            for f, size in expected_files.items():
                fn = _norm(f)
                segs = by_path.get(fn) or by_path.get(f)
                if not segs:
                    if fn in renamed:
                        continue   # produced by the rename fast-path -- present, atomic, size N/A
                    probs.append(f"missing dest file (no create/write, no rename) {f}")
                    continue
                created = any(s.is_create for s in segs)
                if not created:
                    probs.append(f"dest file {f} written but never create-opened")
                total = sum(s.bytes_written() for s in segs)
                if total != size:
                    probs.append(f"dest file {f}: wrote {total} bytes, expected {size}")
                if not any(s.close_seq is not None for s in segs):
                    probs.append(f"dest file {f} never closed")
        return probs

    def problems_dependency(self, check_data=True):
        probs = []
        # (a) no data op outside an open..close window (orphans == after-close / never-opened)
        if check_data:
            for e in self.orphan_data:
                probs.append(f"{e.verb} of {e.path} at #{e.seq} with no open in effect "
                             f"(read/write after close or before open)")
            # writes/reads are attached only while open, so open<op<close holds by construction;
            # assert close ordering explicitly for safety.
            for s in self.segments:
                if s.close_seq is None:
                    continue
                for (seq, _o, _l) in s.writes + s.reads:
                    if not (s.open_seq < seq < s.close_seq):
                        probs.append(f"data op #{seq} on {s.path} outside its open..close "
                                     f"(#{s.open_seq}..#{s.close_seq})")
        # Build: file paths (regular files) and their last close; dir paths and their meta ops.
        file_paths = self.dest_file_paths()
        # (b) a FILE's set-perm / set-date only AFTER that file's last close
        for e in self.utimes + self.chmods:
            if not self._is_dest(e.path):
                continue
            if e.path in file_paths:
                lc = self.last_close_seq(e.path)
                if lc is not None and e.seq < lc:
                    probs.append(f"{e.verb} of file {e.path} at #{e.seq} BEFORE its close #{lc}")
        # (c) a FOLDER's date/perm: the FINAL (last) touch must land AFTER every file under it is
        #     closed. `close` bumps the parent dir mtime on some FS/OS, so whatever folder-date runs
        #     LAST wins -- it must be after all contained files. An EARLY touch is fine as long as a
        #     later correct one exists (the async reference touches a folder at creation and re-asserts
        #     it at the end; only that final assertion's ordering matters for the stored mtime).
        made_dirs = self.successful_mkdir_paths()
        dir_paths = set(made_dirs)
        for e in self.utimes + self.chmods:
            if self._is_dest(e.path) and e.path not in file_paths:
                dir_paths.add(_norm(e.path))
        # last set-date and last set-perm seq per folder
        last_utime = {}
        last_chmod = {}
        for e in self.utimes:
            dp = _norm(e.path)
            if dp in dir_paths:
                last_utime[dp] = max(last_utime.get(dp, -1), e.seq)
        for e in self.chmods:
            dp = _norm(e.path)
            if dp in dir_paths:
                last_chmod[dp] = max(last_chmod.get(dp, -1), e.seq)
        # max close_seq of any dest file strictly under each folder
        contained_last_close = {}
        for s in self.segments:
            if s.close_seq is None or not self._is_dest(s.path) or s.path not in file_paths:
                continue
            for dp in dir_paths:
                if _under(s.path, dp) and _norm(s.path) != dp:
                    if s.close_seq > contained_last_close.get(dp, -1):
                        contained_last_close[dp] = s.close_seq
        for dp, last_close in contained_last_close.items():
            fu = last_utime.get(dp)
            if fu is not None and fu < last_close:
                probs.append(f"final set-date of folder {dp} at #{fu} is BEFORE a contained file "
                             f"closed at #{last_close} (folder mtime will be wrong)")
            fc = last_chmod.get(dp)
            if fc is not None and fc < last_close:
                probs.append(f"final set-perm of folder {dp} at #{fc} is BEFORE a contained file "
                             f"closed at #{last_close}")
        return probs


def summarize(trace_path, dest_root, src_root=None):
    """Human-readable digest of a trace -- used while calibrating the cases against the engine."""
    t = Trace(trace_path, dest_root, src_root)
    lines = [f"events(relevant)={len(t.events)} segments={len(t.segments)} "
             f"mkdir={len(t.mkdirs)} chmod={len(t.chmods)} utime={len(t.utimes)} "
             f"trunc={len(t.truncs)} unlink={len(t.unlinks)} rename={len(t.renames)} "
             f"symlink={len(t.symlinks)} orphan_data={len(t.orphan_data)}"]
    return "\n".join(lines), t


if __name__ == "__main__":
    import sys
    if len(sys.argv) < 3:
        print("usage: optrace.py <trace_file> <dest_root> [src_root]")
        sys.exit(2)
    s, _t = summarize(*sys.argv[1:4])
    print(s)
