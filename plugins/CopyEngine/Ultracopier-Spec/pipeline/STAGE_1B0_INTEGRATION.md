# Stage 1b.0 — shadow-observer integration spec (turn-key)

Wire `OpScheduler` (pipeline/OpScheduler.h) into the io_uring/IOCP backend as a **debug-only shadow
observer** that changes NO dispatch order (diff==0 by construction) and asserts the engine's real
op-emission order is a valid topological linearization of the op DAG. Prerequisite `PathTree::parentOf()`
is **landed** (PathTree.h). Everything below is verified against current line numbers.

## Gating (keeps async + NODEBUG-release byte-identical)
- Backend: `#if defined(ULTRACOPIER_PLUGIN_IO_URING) || defined(ULTRACOPIER_PLUGIN_WINIOCP)` (idiom already
  at `ListThread.cpp:82,129`; `ListThread.h:271`).
- Debug: **`ULTRACOPIER_PLUGIN_DEBUG`** — there is **NO `ULTRACOPIER_DEBUG`** macro. It is
  `CopyEngineUltracopier-SpecVariable.h:11` inside `#ifndef ULTRACOPIER_NODEBUG` (line 10). So NODEBUG
  release ⇒ observer compiled OUT. The whole observer (members + build + hooks + checks) is gated on
  BOTH ⇒ it exists ONLY in a DEBUG io_uring/IOCP build. `OpScheduler.h` is header-only + not in
  `CopyEngine.pro` ⇒ no `.pro` change needed (included only inside the gate).

## ActionType (StructEnumDefinition_CopyEngine.h:102-111)
`MkPath=1` (mkdir → `OpKind::Mkdir`), `MovePath=2` (rmdir source, Move-only), `RealMove=3` (Move-only),
`RmSync=4` (rsync-only), `SyncDate=5` (folder date/perm after children → `OpKind::SetMetaDir`; DEFERRED via
`actionToDoListInode_afterTheTransfer` unless `size==0 || actionToDoListTransferEmpty()`). Plain cp uses
only MkPath + SyncDate.

## The three hooks (2 files) — each `if(gate){ complete(op); PLUGIN_DEBUG: check unmet==0 }`
1. **File finish** — `ListThread.cpp:292`, right after `tombstoneActionToDoTransferAt(int_for_internal_loop);`
   inside `if(foundIndex>=0)` (from `transferInodeIsClosed`, connected at `:1466`). Map
   `temp_transfer_thread->transferId` (== file engineId) → FsNode; complete open,data,close,setMetaFile in
   order, asserting each `unmet==0`. (open.unmet==0 proves parent mkdir already done.)
2. **Mkdir dispatch** — `ListThread_InodeAction.cpp:24` (`currentActionToDoInode.isRunning=true;` of
   `case ActionType_MkPath`). Map action.id → dir FsNode; complete mkdirOp, assert unmet==0.
3. **Folder-date dispatch** — `ListThread_InodeAction.cpp:70` (taken branch of `case ActionType_SyncDate`).
   Complete setMetaDirOp, assert unmet==0 — the assertion that actually exercises the folder-date-after-
   all-children edge. Requires the cp run with **keepDate on** (else no SyncDate action is emitted;
   `addToMkPath` early-returns when `inode!=0 && !keepDate && !doRightTransfer`, `ListThread.cpp:653`).

## Model build — once, at the scan→transfer boundary (`ListThread.cpp:485` region, both lists populated)
- **Dir set** = every distinct node on each live `ActionToDoTransfer`'s dest parent chain: walk
  `dstNode → pathTree.parentOf(...) → ... → PathTree::ROOT_SENTINEL(0xFFFFFFFF)`. Each interned dir node →
  one `FsNode{isDir=true,size=0}`.
- **Files** = each `ActionToDoTransfer` → `FsNode{isDir=false, size=.size, engineId=.id,
  parent=idxOf(parentOf(dstNode))}`.
- **Dir parent** = `idxOf(parentOf(dirNode))` if that node is in the dir set, else **-1** (the user's dest
  base + ancestors get no MkPath action — created by `mkpath -p`, `MkPath.cpp:172` — so treat their mkdir
  as pre-satisfied: a root op with `unmet==0` that you never complete/assert).
- **Maps** (key by id/node, NOT physical index — reorder/compaction invalidate indices):
  `fileEngineId→FsIdx` (for hook 1); `dirActionId→FsIdx` (for hooks 2/3). Build the latter by matching each
  MkPath/SyncDate action's `.destination` STRING to `resolve(dirNode)` (avoid re-`intern()`ing at run time,
  which would mutate pathTree; the strings are byte-identical scanner output). **Canonicalization gotcha:**
  `intern("/dst/a")` makes a fresh leaf, NOT the interned dir node files under `/dst/a/` share — so match by
  resolved path string, not by interning the folder action's destination.
- Construct `ucpipe::OpScheduler sched(nodes)`. Ascending `generateIdNumber()` emission + `engineId` feed ⇒
  every hook's `complete()` fires after its prerequisites ⇒ `unmet==0` holds with NO false assert on a
  clean cp. **Whole-job model is O(#files)** — fine for a DEBUG-only small tree; **windowing over the
  bounded in-flight set (`ListThread.cpp:173-201`) is REQUIRED before non-debug / ship** (SCALE rule).

## First cut = plain Copy, no-fault, no-collision, no-reorder, keepDate on
Every desync source is gated on an error/user/stall condition absent from that path: put-to-end
(`transferPutAtBottom` `:354`), skip/finalSkipNoRetry (`ListThreadActions.cpp:56`), resume/retry
(`:1192`), collision parking (`:1293`), user reorder (`ListThreadListChange.cpp`), safetyReschedule
(`:1350`). Exclude all in v1 (assert only fires on the clean path); later versions retire/rebuild window
nodes on these events. `OpScheduler::complete()` is idempotent, so an accidental double-complete is safe;
a never-completed op (skip/give-up) would make its parent's SetMetaDir falsely assert — hence the clean-path
restriction.

## Shadow test (`cases/opscheduler_shadow.py`)
Build a **DEBUG** io_uring binary (drop `CONFIG+=nodebug` ⇒ `ULTRACOPIER_PLUGIN_DEBUG` defined). Run a small
recursive cp (keepDate on, overwrite collision, no faults) and assert: process completes, `diff -rq
--no-dereference`==0, and the observer signalled **0 violations** + ran non-vacuously (emit a distinct
`OPSHADOW ok nodes=N / OPSHADOW_VIOLATION ...` line to stderr from the gated check — a debug diagnostic,
compiled out of release, so not a shipped test shim). Gate the whole step: io_uring NODEBUG full suite +
ops_integrity/ops_dependency + diff==0 + valgrind/ASan green; async byte-identical; MXE-IOCP + XP cross-builds
still compile.
