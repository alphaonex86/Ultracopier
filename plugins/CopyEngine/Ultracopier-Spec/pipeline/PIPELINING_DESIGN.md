# Pipelining design — the operation-decomposition scheduler (io_uring / IOCP)

Status: **design + guardrail landed; scheduler build staged below.** Async is untouched (stable
reference). Read together with `../CLAUDE.md` → "The pipelining design".

## 1. Why

Small-file throughput. With one op-chain per file per inode thread, the per-file
`open→read→write→close→chmod→utime` **serializes** inside each thread; multi-threading only roughly
halved the IOCP small-file gap vs robocopy (io_uring 2.5×→1.4× rsync). The cheap, latency-bound
**metadata** ops (open/close/mkdir/chmod/utime) and the bandwidth-bound **data** ops (read/write) want
*different* amounts of parallelism, but today they are welded together in one thread. Decoupling them —
deep-queue the metadata, bound the data — is the actual fix.

## 2. What the trace shows today (async reference, `test/lib/optrace.py`)

A clean copy of a small tree already exhibits two avoidable inefficiencies the scheduler should drop:

- **Redundant per-file parent `mkdir`.** MkPath re-`mkdir`s each file's parent (gets `EEXIST`) instead
  of remembering dirs it created. → the "reuse inode cache" the user asked for: keep a created-dir set;
  on `open` `ENOENT`, `mkdir -p` the parent then retry, and never re-`mkdir` a known dir.
- **Folder metadata set twice** — once at folder creation (before its files), once at the end (after
  all files close). Only the *final* one is load-bearing (the earlier is overwritten). The scheduler
  should set folder date/perm **once**, at the end, satisfying the dependency edge directly.

Both are *optimizations*, not correctness bugs; async stays as-is. The pipelined scheduler simply
does it right from the start. (Files already get exactly one create-open + full writes + close + one
chmod + one utime, in valid order — that part is the model to keep.)

## 3. The operation model

Decompose every ListThread action into typed ops with explicit dependencies:

| op        | class    | depends on                                              |
|-----------|----------|---------------------------------------------------------|
| `MKDIR`   | inode    | parent dir exists                                       |
| `OPEN_R`  | inode    | (source exists)                                         |
| `OPEN_W`  | inode    | dest parent dir exists (MKDIR)                           |
| `PREALLOC`| inode    | OPEN_W + final size known (best-effort; skip if unsupported) |
| `READ`    | data     | OPEN_R of same file                                     |
| `WRITE`   | data     | OPEN_W (+ PREALLOC if used) + the READ that produced the bytes |
| `CLOSE`   | inode    | all READ/WRITE of that fd done                          |
| `SETPERM` | inode    | CLOSE of that file                                      |
| `SETDATE` | inode    | CLOSE of that file (file); all contained files CLOSE (folder) |
| `UNLINK`  | inode    | (move) dest of that file fully verified                 |
| `RMDIR`   | inode    | (move) source folder empty                              |

The **hard edges** are exactly what `ops_dependency.py` asserts. Everything else is free to reorder
and to run on any pool. (`PREALLOC` is one of those free ops: best-effort and **performance-only** — NOT
a data-safety edge, NOT asserted by `ops_dependency.py`, correctness-neutral if skipped or reordered; its
`PREALLOC`→`WRITE` arrow is a *preference*, not a hard edge, and there is no prealloc verb in the trace's
tracked set.) Ops carry a small state; the *payload* (paths) is resolved lazily from the existing
transfer-list / PathTree — **no per-file struct is materialised for all files** (SCALE rule).

## 4. Tiered pools + device scaling (targets; autotuned)

*Scope: this whole tiered + device-adaptive + preallocation scheme is **io_uring / IOCP only**. The async
backend is already stable and is **not** changed — see §6.*

**HDD baseline** (seek-bound; the widths to start from):
- **inode pool** — `MKDIR/OPEN/CLOSE/PREALLOC/SETPERM/SETDATE`: ~**8× cores** (deep queue hides syscall
  latency; the small-file win).
- **data pool, medium** (file < 4 KB): ~**cores**.
- **data pool, small** (file < 64 KB): ~**cores / 4, min 2**.
- **data, large** (≥ 64 KB): **serial** (concurrent large reads on a spindle = seek storms).

**Scale to the device** (faster media want MORE concurrency, not less):
- **SSD** (SATA): raise the data pools; large files can run ~2–4 in parallel.
- **NVMe** (deep HW queues): raise further — many concurrent data ops, large-file content parallelised,
  wider inode pool.
Detect as a hint (Linux `/sys/block/<dev>/queue/rotational`=1 → HDD; NVMe via device node); confirm by
measurement.

**Match source ↔ destination — the more-serial side wins.** Effective data parallelism is bounded by the
*less* capable of the two devices (NVMe→HDD, local→network/USB ⇒ use the HDD/network limit). Metadata
parallelism follows the **destination** (where `mkdir`/`open`/`setmeta` land).

**Preallocate the dest (anti-fragmentation). [LANDED — io_uring + IOCP.]** On `PREALLOC` (FRESH file,
size ≥ `PREALLOCATE_MIN_SIZE`=1 MiB) reserve the full extent before the writes: io_uring
`fallocate(FALLOC_FL_KEEP_SIZE)` — reserves the BLOCKS *without* changing `st_size`, so an interrupted
copy keeps its partial-size semantics (preferred over `mode 0`, which would set the size to full); Windows
`SetFileInformationByHandle(FileAllocationInfo)` — reserves the clusters WITHOUT moving the logical EOF
(the Windows KEEP_SIZE equivalent). **Fresh file only** (never a resume prefix), **size-gated** (< 1 MiB
skipped — small files don't fragment and it'd only add syscalls to the small-file fast path),
**best-effort** — on `EOPNOTSUPP`/`ENOSYS` (FAT, network FS) or `ENOSPC` **skip**, never error (the real
ENOSPC still surfaces at write time). Do NOT use glibc `posix_fallocate` — it silently zero-writes the
whole file on an unsupported FS (O(filesize) *data* I/O); NOT `SetFileValidData` (needs
`SE_MANAGE_VOLUME_NAME`, leaks stale disk bytes); and **NOT `SetEndOfFile`** — it sets `st_size` to full
immediately, so a HARD crash/power-loss (bypassing the graceful unlink) would leave a full-size zero-tailed
file that a later size-only `fileCollision` "overwrite if not same size" run silently SKIPS as complete =
**silent data corruption** (caught by the adversarial review). Both backends keep `st_size` == bytes
written, so an interrupted dest is always SHORT (correctly re-copied); the base also UNLINKS a
not-fully-copied dest on graceful cancel/skip/error (`destinationIsOursToRemove()`). Verified: io_uring
suite + `preallocate.py` (the `FALLOC` op trace proves it engages ≥1 MiB and skips below) + valgrind; IOCP `iocp_parity` on the
laptop. Implemented in each backend's `openDestFile` (not a shared hook — the fd/HANDLE + primitive differ).

**Autotune.** Widths + thresholds are starting points; sample achieved MB/s and adjust data concurrency
up while it climbs, back off on plateau/regression, converging toward the matched source/dest limit.
Simple + observable (log the chosen widths).

## 5. Memory

O(in-flight ops), never O(#files). The op queue is a **bounded window** drained as files complete;
buffers stay the existing `8 × blockSize` per active data op. Backed by the transfer-list/PathTree so
paths are stored once and derived lazily. Enforced by the suite's 1 GB RSS cap.

## 6. Backends

Async: untouched. Build/verify on **io_uring first** (Linux, `ULTRACOPIER_PLUGIN_IO_URING`, valgrind
≥3.27 + TSAN). Port to **IOCP** after (laptop-verified per the HARD RULE). The scheduler lives in shared
code (a new `pipeline/` class alongside `TransferThreadPipelined`); backends provide the same async I/O
hooks they already do.

## 7. Staged plan (each stage gated: `ops_integrity` + `ops_dependency` + full suite + TSAN + diff==0)

0. **[DONE] Guardrail.** OPS-TRACE in the shim + `lib/optrace.py` + `ops_integrity.py` +
   `ops_dependency.py`. Baseline green on async; metadata-subset on io_uring.
1. **Op model + single-thread scheduler.** Emit the typed-op DAG for a subtree; execute it on ONE
   inode thread, correctness-first (no reorder yet). Must stay diff==0 and pass the guardrail. This
   isolates the decomposition from the parallelism (so a later regression is provably threads, per the
   user's "use TSAN to know if the regression is due to multi-thread"). Split into:
   - **1a [DONE].** The scheduler CORE as a dependency-free, unit-tested component: `pipeline/OpScheduler.h`
     (typed ops Mkdir/Open/Data/Close/SetMetaFile/SetMetaDir over `FsNode` INDICES — not paths, per the
     SCALE rule — with the dependency EDGES and a readiness engine + tiered-pool `classOf`) +
     `test/unit/opscheduler_test.cpp` (`cases/opscheduler_unit.py`). Proves, at the plan level, the same
     invariants `ops_dependency.py` asserts on the engine: no dup / no missing, every edge respected under
     any completion order, the transitive folder-date-after-every-contained-file-closed rule, pool
     classification. ZERO engine risk (no I/O, not yet wired in).
   - **1b [next — design validated, observer-first].** Wire `OpScheduler` into the io_uring backend
     WITHOUT changing dispatch order yet. All gated `#if defined(ULTRACOPIER_PLUGIN_IO_URING) ||
     defined(ULTRACOPIER_PLUGIN_WINIOCP)` (the idiom already at `ListThread.cpp:82/129/547/595/…`); the
     header is dependency-free so the async TU stays byte-identical with no `.pro` change.
     - **1b.0 SHADOW OBSERVER [IMPLEMENTED — first cut].** Spec: `pipeline/STAGE_1B0_INTEGRATION.md`;
       prerequisite `PathTree::parentOf()` LANDED (behaviour-neutral, async compiles it, never calls it).
       As landed: gated `(IO_URING||WINIOCP) && ULTRACOPIER_PLUGIN_DEBUG` (NOT "ULTRACOPIER_DEBUG" —
       that macro does not exist), members + `opShadowBuild()` + 3 hook methods in `ListThread`
       (`opShadowCompleteFile` after the tombstone in `transferInodeIsClosed`; `opShadowCompleteMkdir` /
       `opShadowCompleteSetMetaDir` at the MkPath/SyncDate launches in `ListThread_InodeAction.cpp`).
       Model built ONCE at `autoStartAndCheckSpace()` (scan finished, lists fully populated, nothing
       dispatched): file FsNodes from `ActionToDoTransfer` (engineId=.id, size=.size, parent via
       `parentOf(dstNode)`), dir FsNodes from each file's dest-parent chain — **bounded by the
       modeled-dir set** (the MkPath/SyncDate destinations): the climb STOPS at the first ancestor with
       no action (the user's dest base + everything above is pre-existing, `parent=-1`); an unbounded
       climb walked into `/mnt/...` and false-flagged (caught + fixed in the debug loop). Dir actions
       matched to nodes by RESOLVED-path string (never re-`intern()` — the canonicalization gotcha).
       VIOLATIONS ARE LOGGED, never abort (`OPSHADOW_VIOLATION` lines + an `OPSHADOW summary:
       nodes/checks/violations` line at job-Idle — logged at Idle, not the dtor, because pkill/SIGTERM
       skips destructors). First cut = plain Copy + keepDate/doRightTransfer, no fault/collision/
       reorder/Move: any other shape sets `opShadowActive=false` (silently declines — no false alarms).
       WHOLE-JOB model: O(#files) — acceptable ONLY because it exists solely in a DEBUG build; the
       windowed O(in-flight) model is still required before any non-debug use (SCALE rule).
       Verified: 5-file nested tree → `nodes=28 checks=28 violations=0`; `comprehensive` synthtree →
       `nodes=196 checks=196 violations=0`, diff==0 both; Move job correctly declines (no OPSHADOW
       lines, move itself correct); NODEBUG async + io_uring rebuild clean with **zero**
       `opShadow`/`OPSHADOW` strings in either binary (observer fully compiled out).
       Test: `cases/opscheduler_shadow.py` (builds its OWN debug io_uring binary — the harness's
       `build/test-io_uring` stays NODEBUG — asserts engaged + checks==nodes (non-vacuous) +
       violations==0 + diff==0). **Do NOT jump to the driver from here** — `readyOps()`
       yields ops in FsNode-index order, ≠ the engine's `generateIdNumber()` order, so driving would
       silently perturb collision/put-to-end/resume (real diff≠0 risk); the driver is 1b.1 with the
       lowest-engineId pick.
     - **1b.1 CONSTRAINED DRIVER** (only after 1b.0 green). Replace the id pick at `ListThread.cpp:1026`
       with `readyOps()` → map the chosen op back to its whole-file hand-off (unchanged
       `setFiles()`/`startTheTransfer`), mark the four file-ops complete on thread-done. Still single
       inode thread, still whole-file. **diff==0 requires** carrying each action's engine id into
       `FsNode` and yielding the lowest-engine-id ready op (reproduce the ascending tie-break). Proves
       the scheduler can DRIVE without regression — without yet cashing parallelism.
       *(The tie-break primitive is DONE: `FsNode.engineId` + `OpScheduler::nextReadyByEngineId()`,
       unit-proven in `opscheduler_test.cpp` under ascending + scrambled engineId assignments — the pick
       always yields the lowest-engineId ready op while respecting every edge. What remains for 1b is the
       LIVE wiring: build the FsNode window model from ListThread's `ActionToDoInode`/`ActionToDoTransfer`/
       `PathTree` state and drive `complete()`/the pick at the real dispatch points — the deep, gated,
       full-multi-build-gated integration.)*
     Every 1b step gates on: full io_uring suite + `ops_integrity`/`ops_dependency` + diff==0 +
     valgrind/ASan, **async byte-identical**, and the MXE-IOCP + XP/mingw492 cross-builds still compile.
     Preserve-all-options audit (collision parking, fileError skip/put-to-end/retry+finalSkipNoRetry,
     resume must re-drive the SAME thread's `retryAfterError`, MOVE per-file-delete + rmdir-after-empty,
     checksum between Close and SetMetaFile, progress/ETA once-per-node, `stopIt`/`putInPause`, cancel
     drain/join ordering) is part of the gate. (Unlink/Rmdir/Prealloc become explicit op kinds when the
     move + preallocation paths are wired.)
2. **Inode-cache + drop the redundancies.** Created-dir set (no re-`mkdir`); folder date/perm once at
   end; `open`-ENOENT→`mkdir -p`→retry. Guardrail proves no dup/missing + folder-date-after-closes.
3. **Split pools + tiered parallelism.** Route ops to the inode pool and the size-bucketed data pools.
   TSAN clean. Bench small-file throughput vs stage 1.
4. **Autotune.** Feedback loop on measured MB/s. Log decisions; never let it violate an edge or the mem
   cap.
5. **Port to IOCP.** Same scheduler; laptop verification + `iocp_parity`.

Data safety, integrity, and honoring the options gate every stage — a stage that regresses any of them
does not land.

## 8. Landed I/O micro-optimizations (self-contained, io_uring/IOCP-only, async untouched)

Independent of the scheduler stages — each removes/avoids a per-file cost on the small-file critical path,
each `#if`-gated to a pipelined backend, each verified content-byte-identical:

- **Destination preallocation** — §4 above (the `PREALLOC` op). io_uring `fallocate(KEEP_SIZE)` / IOCP
  `SetFileInformationByHandle(FileAllocationInfo)`, ≥1 MiB, best-effort. [io_uring suite + valgrind + laptop]
- **io_uring: removed the dead `lseek(destFd,0)`** in `openDestFile` — every dest I/O uses an explicit
  `io_uring_prep_write` offset, so the seek was a dead per-file blocking syscall. [12/12 io_uring]
- **IOCP: `FILE_FLAG_SEQUENTIAL_SCAN`** on the dest handle (gated on `os_spec_flags`, mirroring the source
  hint). [iocp_parity 9/9]
- **io_uring: fire-and-forget close** — `closeFiles()` still SUBMITS the close SQEs (mandatory, else the
  last file's fd leaks) but no longer `io_uring_wait_cqe`-blocks; the ≤2 unreaped close CQEs are swept by
  the next file's open discard-drains (which now log any close error) / `io_uring_cq_advance` / queue_exit.
  Removes one `io_uring_enter(GETEVENTS)`/file. Design validated by the `design-fire-and-forget-close`
  workflow (GO-with-changes). [20/20 io_uring suite + fd-leak (3000 files → peak 60 fds) + valgrind 3/3]

Next candidate levers (from the `pick-next-pipelining-increment` workflow): coalesce the two `openat`
submits into one (io_uring, high-risk); IOCP `FILE_SKIP_COMPLETION_PORT_ON_SUCCESS` (medium); then the
tiered op-scheduler proper (stages 1–5).
