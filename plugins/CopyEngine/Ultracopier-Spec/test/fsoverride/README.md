# fsoverride -- filesystem-condition override layer for the Ultracopier tests

This directory provides a thin **API-override layer** that lets the test suite
simulate filesystem conditions (I/O errors, denied opens, slow I/O, short writes,
metadata-access failures) against the **real, unmodified** ultracopier binary.

It honours the project rule "override the API with minimal code changes": we never
patch ultracopier's C++ to make it testable. Instead we interpose the OS file APIs
*around* the running process:

* **Linux:** `linux/fs_preload.c` -> built into `build/fs_preload.so`, injected via
  `LD_PRELOAD`. The test harness (`../lib/harness.py`) passes it as `fs_preload=`.
* **Windows:** `windows/fs_hook.cpp` -> an injected hook DLL for the IOCP lane
  (SKELETON; see *Windows* below).

Both lanes are driven by the **same** environment variable, **`UC_FS_SCENARIO`**,
with the same grammar, so a test case expresses one scenario and it works on either
platform.

---

## Build

### Linux (LD_PRELOAD shim)

```bash
cd plugins/CopyEngine/Ultracopier-Spec/test/fsoverride
make                # -> build/fs_preload.so
make clean
```

Produces `build/fs_preload.so` (gcc `-shared -fPIC -ldl`). That path is exactly
what `harness.run(..., fs_preload="...../build/fs_preload.so")` expects.

### Windows (hook DLL)

`windows/fs_hook.cpp` is a **documented skeleton** -- the structure, hook table,
scenario parser and DllMain are complete, but the actual inline-hooking is left as
clearly marked `TODO`s. To finish it:

1. Vendor **MinHook** (MIT) and link `minhook.lib`.
2. Replace the `installHooks()` / `removeHooks()` TODO blocks with
   `MH_Initialize` / `MH_CreateHookApi` / `MH_EnableHook`.
3. Build it as a DLL and inject it into a freshly spawned, *suspended* ultracopier
   process via `CreateRemoteThread` + `LoadLibraryW` (recommended) or Detours'
   `DetourCreateProcessWithDllEx`. Pass `UC_FS_SCENARIO` in that process's
   environment.

It intentionally does **not** compile yet (no MinHook dependency committed); the
design is complete enough to finish later.

---

## `UC_FS_SCENARIO` grammar

A **comma-separated** list of rules. Each rule is `verb:arg`. Unknown verbs are
ignored. When the variable is **unset or empty the layer is a pure pass-through**
(zero behaviour change) -- so non-error tests are unaffected.

| Rule                  | Effect                                                                                  | Matches on |
|-----------------------|-----------------------------------------------------------------------------------------|------------|
| `efail:<path-substr>` | `write`/`pwrite` (and `utimensat`/`chmod`) to a path containing `<substr>` fail `EIO`.   | path       |
| `openfail:<substr>`   | `open`/`open64`/`openat`/`mkdir` of a path containing `<substr>` fails `EACCES`.         | path       |
| `slow:<ms>`           | every `read`/`write`/`pread`/`pwrite` sleeps `<ms>` ms before the real call.             | global     |
| `shortwrite:<substr>` | `write`/`pwrite` to a path containing `<substr>` returns ~half the bytes (>=1).          | path       |
| `wflip:<substr>:<off>` | `write`/`pwrite` to a matching path SILENTLY bit-flips the byte at cumulative offset `<off>` but returns the FULL count — a CONTENT corruption (the dest looks complete). With `checksum` on, the verify re-read must DETECT it and the engine REMOVE the corrupt dest (`cases/write_corruption_async.py`, the #25 fix). ASYNC/libc only; io_uring uses the writable FUSE `UC_FUSE_WFAULT`. | path + offset |
| `statfail:<substr>`   | `stat`/`lstat`/`fstat`/`statx`/`readlink` of a matching path fails `EACCES`.             | path       |
| `readfail:<substr>`   | `read`/`pread` of a path containing `<substr>` fails `EIO` (whole SOURCE file unreadable). | path     |
| `eio_after:<substr>:<bytes>` | `read`/`pread` of a matching fd succeed until `<bytes>` cumulative bytes, then fail `EIO` (bad sector PARTWAY through a SOURCE file; the readable prefix is delivered first). | path + per-fd counter |
| `flaky:<substr>:<n>`  | the first `<n>` read attempts on a matching SOURCE path fail `EIO`, then succeed (a transient/recoverable sector). Budget tracked PER PATH, so close()+reopen()+retry keeps draining it. | path + per-path counter |
| `datefail:<substr>`   | ONLY the date-set (`utimensat`/`futimens`/`utime`/`utimes`) on a matching path fails `EPERM`; the data `write`s SUCCEED. Isolates the `keepDate` failure path: with `keepDate=true` the file errors out ("Unable to change the date"), with `keepDate=false` the copy proceeds (best-effort). Reaches async (`utime`) and io_uring (`futimens` on the open destFd); IOCP uses `SetFileTime` (Windows hook). | path |
| `statmut:<substr>:<n>`| `stat`/`lstat`/`statx` of a matching SOURCE path returns a MUTATED `st_size` (`*2+4096`) and bumped `st_mtime` from the **(n+1)th** call onward (calls `1..n` are real). With `n=1` the scan's first stat is genuine and is what `coalesceSourceStat` caches; a later RE-stat (cache miss) would see the mutated values. Models "source data changed while cached". | path + per-path counter |

Path matching is a plain substring test (case-insensitive on Windows). For the
multi-field verbs (`eio_after`, `flaky`, `statmut`, `disconnect`, `wdisconnect`) the
trailing number(s) are split off the **last** colon(s), so a path-substr may itself contain `:`.

### Accounting (independent of `UC_FS_SCENARIO`)

Two objective counters, each enabled by its own env pair and flushed periodically (so the
value survives the harness's `pkill` SIGTERM, which skips `atexit`/destructors):

| Env pair | Counts |
|----------|--------|
| `UC_FS_READLOG_MATCH=<substr>` + `UC_FS_READLOG_PATH=<file>` | total BYTES successfully `read`/`pread` from matching paths (resume-vs-restart: total ≈ size if resumed, size+prefix if restarted). |
| `UC_FS_STATLOG_MATCH=<substr>` + `UC_FS_STATLOG_PATH=<file>` | number of `stat`/`lstat`/`statx`/`fstat` CALLS on matching paths (lower with `coalesceSourceStat` on than off — the objective proof the cache is honored). |

### Examples

```bash
SO=plugins/CopyEngine/Ultracopier-Spec/test/fsoverride/build/fs_preload.so

# Force a write error on everything copied under /tmp/dst/
LD_PRELOAD=$SO UC_FS_SCENARIO="efail:/tmp/dst/" ./ultracopier cp /tmp/src /tmp/dst

# Deny opening a specific source file
UC_FS_SCENARIO="openfail:secret.bin"

# Slow I/O (5ms per read/write) + partial writes to the destination
UC_FS_SCENARIO="slow:5,shortwrite:/tmp/dst/"

# Deny metadata access on a path (exercise the scan-error path)
UC_FS_SCENARIO="statfail:/tmp/src/locked"

# Failing-disk SOURCE faults (back up a dying HDD):
UC_FS_SCENARIO="readfail:/src/movie.iso"        # whole file is a dead-sector run (EIO)
UC_FS_SCENARIO="eio_after:/src/archive:65536"   # first 64 KiB readable, then a bad sector
UC_FS_SCENARIO="flaky:/src/photo.jpg:3"         # 3 EIO read attempts, then it recovers
```

From a test case (via the harness):

```python
import sys, pathlib; sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1])); from lib import harness as H

so = str(pathlib.Path(__file__).resolve().parents[1] / "fsoverride" / "build" / "fs_preload.so")
os.environ["UC_FS_SCENARIO"] = "efail:/dst/"
r = H.run(H.ASYNC, "cp", [src], dst, fs_preload=so, file_error=H.FileError.SKIP, expect_dir=...)
```

(The harness copies the current environment, so set `UC_FS_SCENARIO` in
`os.environ` before calling `run`, and pass `fs_preload=` to the `.so`.)

---

## What it catches -- and the io_uring limitation

The shim interposes **libc** symbols. That covers two things:

1. **The ASYNC (pthread) data plane** -- its file I/O is plain libc
   `open`/`read`/`write`/`pread`/`pwrite`/`close`, all interposed. So `efail`,
   `slow`, and `shortwrite` fire on the actual copied bytes.

2. **The shared scan / metadata path used by ALL backends** -- directory and file
   handles (`open`/`openat`), `lstat`/`stat`/`statx`, `readlink`, `mkdir`,
   `unlink`, `rename`, `symlink`, `utimensat`, `chmod`. So `openfail` and
   `statfail` fire regardless of backend.

> **io_uring limitation (read this).** The **io_uring data plane does NOT go
> through libc `read`/`write`.** It submits SQEs and calls `io_uring_enter(2)`
> directly, so `efail` / `slow` / `shortwrite` on the **transferred bytes** will
> **not** trigger for io_uring transfers. The io_uring backend's *metadata/scan*
> calls still use libc, so `openfail` / `statfail` **do** apply to it. To fault-
> inject io_uring *data* I/O you need a real fault-injecting layer below the
> kernel I/O path (e.g. a FUSE filesystem, `dm-flakey`, or an error-returning
> block device) -- that is out of scope for this LD_PRELOAD shim.

On **Windows** there is no io_uring; the IOCP hook (skeleton) targets overlapped
`ReadFile`/`WriteFile` on the completion port plus the scan/metadata calls. Note
that short-writing / failing an **overlapped** (IOCP) operation mid-flight may also
require hooking the completion (`GetQueuedCompletionStatus`) -- see the TODOs in
`windows/fs_hook.cpp`.

---

## Failing-disk / backup use case

A primary reason people reach for Ultracopier is **salvaging a dying HDD**: copy
everything still readable, work *around* bad sectors, and **never abort the whole job,
hang, or crash** because one file (or one sector) is unreadable. The expected behaviour
is *"try, work around, inform"*: copy every readable file in full, **SKIP / defer** the
unreadable ones (surfaced via the interactive `fileErrorDialog`, or headlessly via the
`fileError` policy index `1`=Skip / `2`=Put-to-end), and degrade to a **resilient partial
backup** rather than an all-or-nothing failure.

The `readfail` / `eio_after` / `flaky` verbs exist to test exactly this. Unlike the
dest-side write verbs (`efail`, `shortwrite`, ENOSPC, slow), **these fault the SOURCE
reads** -- they model the dying disk being *read from*:

| verb        | models | proves |
|-------------|--------|--------|
| `readfail`  | a file sitting entirely on dead sectors (unreadable from byte 0) | the file is skipped/deferred; the rest of the backup still completes |
| `eio_after` | a bad sector **partway** through a file (readable prefix, then EIO) | the readable prefix is delivered; the file is never written with wrong/garbage bytes; the job continues |
| `flaky`     | a sector that reads on **retry** (transient defect) | the engine's Retry path eventually reads the file fully and copies it correctly |

A failing-disk test therefore injects one of these on the source, picks a non-`Ask`
`fileError` index, and asserts the resilient outcome: the process **stays alive** and the
job **completes** (no hang/crash -- `pkill` still does the killing); every **readable**
file is byte-identical (`diff -rq --no-dereference` of the readable subset == 0); the bad
file is absent or at most a **faithful prefix** of the source (never trailing garbage);
and memory stays under the cap with **zero** mem-errors (a storm of read errors must not
leak buffers/handles or free an in-flight I/O buffer). See `test/cases/faulty_hdd.py`
(readfail/eio_after) and `test/cases/transient_sector.py` (flaky).

> **Backend scope.** The **async** ReadThread reads the source through libc
> `::read`/`::pread`, so these verbs fire for the async backend. The **io_uring** DATA
> plane reads via `io_uring_enter(2)` and bypasses libc, so its transferred bytes
> **cannot** be source-faulted this way (only its metadata/scan path is libc and thus
> affected). On **Windows/IOCP** the source `ReadFile` faults are injected by the hook DLL
> (skeleton).
```
