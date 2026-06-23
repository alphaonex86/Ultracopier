# Ultracopier-Spec CopyEngine — Test Framework Architecture

This document is a **detailed textual + ASCII description** of the test framework that
lives under `plugins/CopyEngine/Ultracopier-Spec/test/`. It is written so it can be
pasted into a diagramming model (e.g. Gemini) to reproduce the boxes, layers and arrows
faithfully. Read it together with `lib/harness.py` (the contract) and
`config.example.ini` (the configuration/secret-isolation story).

---

## 0. One-sentence mental model

> **Python test cases call a single `harness.run()` contract, which drives the REAL,
> unmodified `ultracopier` binary for one backend, surrounded by three "override planes"
> (settings/collision, filesystem/API, process-lifecycle), and then judges the run by
> diffing real folders and reading a memory checker — never by touching the C++.**

The guiding rule (from `../CLAUDE.md` and the harness docstring): **we never add test
hooks to ultracopier's C++**. Everything is observed and controlled from the outside.

---

## 1. The layered design (top to bottom)

There are four conceptual layers. Control flows **downward** (a case asks the harness to
run something, the harness launches the binary); evidence flows **upward** (the binary
mutates the real filesystem and emits memory-checker output, which the harness reads back
and returns as a `Result` to the case).

```
+===========================================================================+
|  LAYER 1 — TEST CASES (Python)                                            |
|  test/cases/*.py  +  test/all.py (runner) + test/lib/synthtree.py        |
|                                                                           |
|  Each case: build/point-to a source tree, pick a backend + collision/    |
|  error policy, call harness.run(...), assert on the returned Result.ok    |
+===========================================================================+
                | calls run(backend, mode, sources, dest, ...)
                v
+===========================================================================+
|  LAYER 2 — harness.run()  (THE CONTRACT)   test/lib/harness.py            |
|                                                                           |
|  A single function is the whole API. It owns: config loading, per-backend |
|  binary build/locate, isolated HOME+conf, env assembly (LD_PRELOAD,       |
|  offscreen, ASan), external launch under a memory cap, CPU-idle           |
|  completion detection, stay-alive assertion, external kill, content diff, |
|  memory-checker parsing -> returns a Result dataclass.                    |
+===========================================================================+
                | spawns argv = [wrapper...] [binary] mode srcs... dest
                v
+---------------------------------------------------------------------------+
|  LAYER 3 — THREE OVERRIDE PLANES wrapped around the binary                |
|                                                                           |
|   (1) SETTINGS / COLLISION PLANE  -> isolated ~/.config/ultracopier.conf  |
|       with QComboBox-INDEX keys (autoStart=true => no dialog ever shown)  |
|                                                                           |
|   (2) FS / API PLANE             -> LD_PRELOAD shim (Linux) / hook DLL    |
|       (Windows) selected by env UC_FS_SCENARIO (inject EIO/ENOSPC/slow)   |
|                                                                           |
|   (3) PROCESS-LIFECYCLE PLANE    -> external launch (offscreen, 1 GB cap, |
|       optional valgrind/ASan) + CPU-idle done-detect + stay-alive +      |
|       external kill                                                       |
+---------------------------------------------------------------------------+
                | controls / contains
                v
+===========================================================================+
|  LAYER 4 — THE REAL ULTRACOPIER BINARY (one per backend, UNMODIFIED)      |
|                                                                           |
|   argv:  ultracopier  cp|mv  <src...>  <dest>                            |
|   reads the isolated conf, runs ListThread + TransferThread, writes the  |
|   destination tree on the REAL filesystem.                               |
+===========================================================================+
                | side effects
                v
        REAL FILESYSTEM (dest tree)   +   MEM-CHECKER OUTPUT (valgrind.log / asan*)
                ^----------- read back by Layer 2 to build the Result -----------^
```

### Layer 1 — Test cases (Python)
- Live in `test/cases/` (one concern per file). They are pure orchestration: pick a
  backend, pick a collision/error policy (using the index constants in `harness.py`:
  `FileCollision`, `FolderCollision`, `FileError`, `FolderError`), build or point at a
  source tree, call `harness.run(...)`, then `assert result.ok` (or assert a specific
  field such as `content_ok`, `stayed_alive`, `mem_errors == 0`).
- `test/all.py` is the aggregate runner that iterates cases across the available
  backend lanes and prints pass/fail.
- The **synthetic tree generator** (referenced by `config.example.ini` and used by CI /
  `all.py`) produces a source tree exercising many shapes: files of many sizes, nested
  dirs, symlinks, a **dangling** symlink, unicode names, a 0-byte file, and a
  **> blockSize multi-chunk** file. It leaks no private data.

### Layer 2 — `harness.run()` (the contract)
This is the only entry point a case touches. Signature (abridged):

```
run(backend, mode, sources, dest, *,
    file_collision, folder_collision, file_error, folder_error,
    keep_date, do_right, expect_dir, memcheck,
    fs_preload, mem_limit_mb, stay_alive_seconds) -> Result
```

Responsibilities, in order:
1. `load_config()` — merge `config.example.ini` (defaults) then `config.ini` (real
   overrides) via `ConfigParser`.
2. `binary_for(backend)` — return a prebuilt path from `[binaries]`, else build into
   `build/test-<backend>[-asan]/` with `qmake6` + `make` (`CONFIG+=release nodebug`,
   so DebugModel never inflates RAM).
3. `write_config(home)` — write the **isolated** conf (plane 1).
4. Assemble `env` and `argv` (planes 2 and 3), launch, monitor, kill, diff, memcheck.
5. Return a `Result`.

The returned **`Result`** dataclass is the upward evidence packet:

| field          | meaning |
|----------------|---------|
| `backend`      | which lane produced it |
| `completed`    | copy reached CPU-idle within `copy_timeout_seconds` |
| `stayed_alive` | still running `stay_alive_seconds` after completion (no teardown crash) |
| `content_ok`   | `diff -rq --no-dereference` == 0 vs `expect_dir` |
| `peak_rss_mb`  | peak resident memory observed via `/proc/<pid>/status` |
| `oom_killed`   | exceeded the memory cap (SIGKILL) |
| `mem_errors`   | valgrind/ASan errors whose stack touches our code |
| `exit_code`    | exit code if it died on its own (`None` if we killed it) |
| `diff_text`    | the diff output for debugging |
| `.ok`          | `completed and stayed_alive and content_ok and not oom_killed and mem_errors==0` |

---

## 2. The three override planes (the heart of "don't touch the C++")

```
                          +-------------------------------------+
                          |        harness.run() assembles       |
                          |        conf + env + argv             |
                          +-------------------------------------+
                            |              |              |
            (1) settings    |     (2) FS   |   (3) process|
                conf        |       env    |    wrapper   |
                            v              v              v
   +------------------+  +---------------------+  +---------------------------+
   | ISOLATED CONF    |  | LD_PRELOAD shim     |  | EXTERNAL LIFECYCLE        |
   | $HOME=throwaway  |  |  (.so) / hook DLL   |  |  systemd-run --scope      |
   | ~/.config/       |  |  + UC_FS_SCENARIO   |  |   -pMemoryMax=1024M       |
   |  ultracopier.conf|  |                     |  |   (ulimit -v fallback)    |
   |                  |  | intercepts libc     |  |  QT_QPA_PLATFORM=offscreen|
   | fileCollision=2  |  | open/read/write/... |  |  optional valgrind / ASan |
   | folderCollision=1|  | to inject EIO,      |  |  CPU-idle done detection  |
   | autoStart=true   |  | ENOSPC, latency,    |  |  stay-alive assertion     |
   | (NO DIALOG)      |  | partial writes      |  |  EXTERNAL pkill teardown  |
   +--------+---------+  +----------+----------+  +-------------+-------------+
            |                       |                           |
            +-----------+----------+------------+--------------+
                        v                        v
                +-------------------------------------------+
                |     REAL ultracopier binary (Layer 4)      |
                +-------------------------------------------+
```

### Plane 1 — Settings / Collision plane (isolated conf, combo-index keys, no dialog)
- `write_config()` creates a **throwaway `$HOME`** (`tempfile.mkdtemp`) and writes
  `~/.config/ultracopier.conf` there. The user's real `~/.config/ultracopier.conf` is
  never read or written.
- It is a QSettings INI under `[CopyEngine-Ultracopier Spec]`. **Crucially, the engine
  reads the QComboBox INDEX, not the enum value** — so the collision/error policy is
  expressed as the integer index the UI combo would have selected. The constants in
  `harness.py` encode exactly those indices:
  - `FileCollision`: ASK=0, SKIP=1, OVERWRITE=2, OVERWRITE_IF_NOT_SAME_SIZE_AND_DATE=3,
    OVERWRITE_IF_NEWER=4, OVERWRITE_IF_OLDER=5, RENAME=6, OVERWRITE_IF_NOT_SAME_SIZE=7,
    OVERWRITE_IF_MDATE_DIFFERS=8.
  - `FolderCollision`: ASK=0, MERGE=1, SKIP=2, RENAME=3.
  - `FileError`: ASK=0, SKIP=1, PUT_TO_END=2.  `FolderError`: ASK=0, SKIP=1.
- `autoStart=true`, `checkDiskSpace=false`, and the window-grouping keys are set so the
  copy starts immediately and **no interactive dialog is ever shown** — the chosen
  non-ASK index makes collisions resolve deterministically without UI.

### Plane 2 — FS / API plane (LD_PRELOAD shim / hook DLL, driven by `UC_FS_SCENARIO`)
- On **Linux**, the harness can prepend an `LD_PRELOAD=<shim.so>` to the child env and set
  `UC_FS_SCENARIO=<name>`. The shim interposes libc syscalls (open/read/write/etc.) so a
  case can inject `EIO`, `ENOSPC`, latency, or partial writes **without modifying
  ultracopier**. This exercises the engine's error/collision policy code paths against
  realistic faults.
- On **Windows**, the equivalent is a **hook DLL** (same `UC_FS_SCENARIO` selector).
- **IMPORTANT LIMITATION:** this plane only works where I/O goes through **libc**. It does
  NOT reach the **io_uring data plane** (see §3) — io_uring submits read/write SQEs
  directly to the kernel, bypassing the libc symbols the shim overrides. So FS-fault
  injection is meaningful for the **async** (and IOCP, via its DLL) lanes, but the
  io_uring data-plane reads/writes are not interceptable by the preload shim.

### Plane 3 — Process-lifecycle plane (external launch, cap, idle-detect, stay-alive, kill)
- **Launch is fully external.** `QT_QPA_PLATFORM=offscreen` runs the GUI headless. The
  binary is wrapped:
  - **Memory cap:** `systemd-run --user --scope -pMemoryMax=1024M -pMemorySwapMax=0`
    kills on overshoot; if `systemd-run` is unavailable, a `bash -c 'ulimit -v ...; exec'`
    fallback is used. The default cap is `1024 MB` (`[limits] mem_limit_mb`).
  - **Optional memory checker:** `valgrind --tool=memcheck --leak-check=full
    --track-origins=yes` (writes `valgrind.log`), or an **ASan** build selected via
    `memcheck=sanitize` with `ASAN_OPTIONS=...:log_path=.../asan`. (valgrind and the
    systemd cap are mutually exclusive in the wrapper logic.)
- **Completion detection is CPU-idle based,** not a code signal: the harness polls
  `/proc/<pid>/stat` utime+stime every 2 s; when CPU stops growing for ~8 s the copy is
  declared finished (robust to IOCP file pre-sizing and to the engine sitting idle in the
  tray). Peak RSS is sampled throughout from `/proc/<pid>/status`.
- **Stay-alive assertion:** after completion the process must remain alive for
  `stay_alive_seconds` (default 10). Surviving this window proves there was **no
  teardown/exit crash** — important because there is deliberately no "exit after copy"
  shortcut in the binary.
- **External kill:** the harness ALWAYS terminates the resident process with an external
  `pkill -x ultracopier` (exact name, never `-f`) and also clears the per-UID
  single-instance socket in `/tmp/ultracopier-<uid>*` before the next run.

---

## 3. The three backend lanes

```
                 harness.run(backend=...)            run_windows(...) [IOCP lane]
                        |                                    |
       +----------------+----------------+                   |
       v                                 v                   v
+----------------+              +------------------+   +---------------------------+
| LANE A: async  |              | LANE B: io_uring |   | LANE C: IOCP (Windows)    |
| Linux, libc    |              | Linux            |   | over SSH to the laptop    |
|                |              |                  |   |                           |
| build:         |              | build:           |   | host = user@ip (config)   |
|  hide liburing |              |  DEFINES+=        |   | exe = C:\...ultracopier   |
|  via empty     |              |  ULTRACOPIER_    |   |  .exe (pre-deployed +     |
|  PKG_CONFIG    |              |  PLUGIN_IO_URING |   |  Qt6 DLLs)                |
|                |              |                  |   |                           |
| FS plane:      |              | FS plane:        |   | mem cap: Windows          |
|  LD_PRELOAD    |              |  data plane NOT  |   |  Job Object               |
|  WORKS         |              |  libc-           |   | crash check: Windows      |
|                |              |  interceptable   |   |  Event log Event ID 1000  |
| memcheck:      |              |  (note!)         |   | empty host => IOCP cases  |
|  valgrind/ASan |              |                  |   |  SKIPPED (Linux still runs)|
+----------------+              +------------------+   +---------------------------+
```

- **Lane A — async (libc):** the default backend. Forced even on a liburing-capable box
  by pointing `PKG_CONFIG_LIBDIR` at an empty dir so qmake cannot find liburing. All three
  override planes apply; the **LD_PRELOAD FS shim works** because I/O goes through libc.
  Memchecking via valgrind or ASan is fully supported here.
- **Lane B — io_uring (Linux):** built with `DEFINES+=ULTRACOPIER_PLUGIN_IO_URING`.
  Settings and process-lifecycle planes apply normally, but **the io_uring data plane is
  NOT libc-interceptable** — the preload FS shim cannot inject faults into the actual
  read/write SQEs. FS-fault cases therefore do not cover io_uring's data path; correctness
  is still verified by the folder diff and memory by valgrind (≥3.27 understands io_uring)
  / ASan.
- **Lane C — IOCP (Windows):** does **not** run through `harness.run()` (which asserts
  `backend in (ASYNC, IO_URING)`); it runs via a separate **`run_windows`** path that
  drives the binary **over SSH** to the Windows 10 laptop configured in `[windows] host`.
  The synthetic tree (or a real `SOURCEWINDOWS`) is pushed to the box, the copy targets
  `DESTINATIONWINDOWS`. Memory is capped with a **Windows Job Object**; a crash is detected
  by scanning the **Windows Event log for Event ID 1000** (application crash). If
  `[windows] host` is empty, **all IOCP cases are SKIPPED** while the Linux async/io_uring
  lanes still run.

---

## 4. Data flow of one case (end to end)

```
 cases/foo.py
    |
    | (1) synthtree.build()  ──►  SOURCE tree (sizes, nested dirs, symlinks,
    |                              dangling symlink, unicode, 0-byte, multi-chunk)
    v
 harness.run(backend, 'cp', [src], dest, file_collision=OVERWRITE, expect_dir=src, memcheck=VALGRIND)
    |
    | (2) write_config(throwaway $HOME)  ──►  ISOLATED ultracopier.conf (combo-index keys, autoStart)
    |
    | (3) build/locate per-backend binary (qmake6 + make, release/nodebug)
    |
    | (4) _kill_all_ultracopier()  (clear resident instance + /tmp socket)
    |
    | (5) LAUNCH:  [systemd-run -pMemoryMax=1024M | ulimit -v]  [valgrind?]  ultracopier cp src dest
    |              env: HOME=throwaway, QT_QPA_PLATFORM=offscreen, LD_PRELOAD=shim?, UC_FS_SCENARIO?
    v
 real ultracopier  ── ListThread + TransferThread ──►  writes DEST tree on real FS
    |
    | (6) MONITOR loop (every 2s):  peak RSS from /proc/<pid>/status
    |                               CPU ticks from /proc/<pid>/stat (utime+stime)
    |        CPU flat ~8s  ───────────────────────────►  completed = True
    |        process gone   ──►  exit_code = poll(), oom = (killed by SIGKILL?)
    v
 (7) STAY-ALIVE:  wait stay_alive_seconds, sample RSS  ──►  stayed_alive = process still present?
    |
 (8) EXTERNAL KILL:  pkill -x ultracopier  (+ clear /tmp/ultracopier-<uid>*)
    |
 (9) DIFF:    diff -rq --no-dereference  expect_dir  vs  dest/<basename(src)>  ──► content_ok, diff_text
    |
 (10) MEMCHECK: parse valgrind.log (errors touching our code) OR asan* logs  ──► mem_errors
    |
 (11) cleanup throwaway $HOME
    v
 Result(backend, completed, stayed_alive, content_ok, peak_rss_mb, oom_killed,
        mem_errors, exit_code, diff_text)   ──►  case asserts result.ok
```

Key points reflected in the code:
- The destination basename rule: ultracopier copies the source **folder** into dest, so
  the harness compares against `dest/<basename(src)>` when that subdir exists
  (`_copied_tree`).
- `diff -rq --no-dereference` makes symlink **targets** part of correctness (not just
  their resolved content) — that is why the synthetic tree deliberately includes a
  dangling symlink.
- Memory-checker parsing is **filtered to our code**: only valgrind/ASan blocks whose
  stack mentions `Ultracopier-Spec`, `TransferThread`, or `ListThread` count, so Qt/glib
  one-time noise does not fail a case.

---

## 5. Config & secret-isolation story

```
 config.example.ini  (committed, defaults)        config.ini  (gitignored, real/private)
          \                                              /
           \___________ ConfigParser.read([...]) ________/   (config.ini overrides example)
                                  |
                                  v
                   [paths]   SOURCE*/DESTINATION* per OS
                   [windows] host (empty => IOCP skipped), exe
                   [binaries] async / io_uring / async_asan (empty => build on demand)
                   [limits]  mem_limit_mb=1024, stay_alive_seconds=10, copy_timeout_seconds=1800
```

- `config.ini` is **gitignored**. Real (possibly private) source/destination paths and the
  Windows box host live only there, so secret folders never enter git while still being
  testable against real data.
- `config.example.ini` holds only safe defaults and documentation.
- **Synthetic-tree fallback:** if `SOURCELINUX` / `SOURCEWINDOWS` is empty, the harness
  builds the synthetic source tree itself. This is what CI and `all.py` use by default —
  it leaks nothing and still exercises the hard shapes (multi-chunk files, symlinks,
  unicode, 0-byte, dangling symlink).
- Throwaway `$HOME` per run (plane 1) guarantees the user's real ultracopier settings are
  never read or mutated.

---

## 6. Directory map of `test/`

```
plugins/CopyEngine/Ultracopier-Spec/test/
├── ARCHITECTURE.md          # this document
├── config.example.ini       # committed defaults + docs (paths/windows/binaries/limits)
├── config.ini               # GITIGNORED real config (you create it; private paths/host)
├── lib/
│   ├── harness.py           # THE CONTRACT: run() + Result + build/conf/launch/diff/memcheck
│   ├── synthtree.py         # synthetic source-tree generator (CI/all.py default source)
│   └── fsshim/              # LD_PRELOAD .so (Linux) / hook DLL (Windows) for the FS plane
│                            #   selected at runtime by env UC_FS_SCENARIO
├── cases/                   # one Python test case per concern; each calls harness.run()
│                            #   e.g. collisions, errors, symlinks, large/multi-chunk,
│                            #   keep-date, mv vs cp, per backend
├── all.py                   # aggregate runner: iterates cases across available lanes
└── build artifacts (out-of-tree):
    └── ../../../../../build/test-<backend>[-asan]/ultracopier   # qmake6/make output (release/nodebug)
```

> Note: at the time of writing, the on-disk tree contains `config.example.ini` and
> `lib/harness.py`; the other entries (`cases/`, `all.py`, `synthtree.py`, `lib/fsshim/`,
> `config.ini`) are the structure the harness expects/references and are created as the
> suite is populated. The build output lives **out of tree** under `sources/build/`.

---

## 7. Legend for the diagrammer

- **Boxes** = components/processes. **Double-lined boxes** (`+===+`) = the main layers
  (cases / contract / binary). **Single-lined boxes** (`+---+`) = sub-components and the
  three override planes / three backend lanes.
- **Downward arrows** = control (a case drives the harness; the harness launches/controls
  the binary).
- **Upward arrows** = evidence (real filesystem mutations + memory-checker logs read back
  into the `Result`).
- The **three override planes** all clamp around the **single** real binary box; the
  **three backend lanes** are mutually exclusive selections of which binary that box is.
