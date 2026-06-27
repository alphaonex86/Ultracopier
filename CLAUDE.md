# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Ultracopier** is a Qt-based file copy/move utility replacement that intercepts system copy/move operations. It's a modular, plugin-based application written in C++17 with multi-threaded transfer operations.

- **Version**: 3.0.x (C++11 to C++23)
- **Website**: https://ultracopier.herman-brule.com/
- **Repository**: https://github.com/alphaonex86/Ultracopier
- **License**: GPLv3
- **Language**: C++ with Qt framework (supports Qt5 and Qt6)
- **Build Systems**: CMake (modern), qmake (.pro files), Qt's automatic UI compilation

## Building and Running

### Prerequisites
```bash
# Debian/Ubuntu-based systems
sudo apt install make gcc build-essential libssl-dev qt6-base-dev qtchooser qmake6 qt6-base-dev-tools qt6-tools-dev-tools
```

### Build (CMake - NOT Recommended)
```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run
./build/Ultracopier
```

### Build (qmake - Recommended)
```bash
# Update translations (if needed)
find ./ -name '*.ts' -exec lrelease {} \;

# Build
qmake ultracopier.pro
make -j$(nproc)

# Run
./ultracopier
```

### Build with Optional Features
- **KDE KIO Support** (for sftp, smb, ftp protocols): `-DULTRACOPIER_PLUGIN_KIO=ON`
- **io_uring Support** (high-performance async I/O): Define `ULTRACOPIER_PLUGIN_IO_URING` in Variable.h (Linux only)
- **Portable Version**: Define `ULTRACOPIER_VERSION_PORTABLE` in Variable.h

### Testing
The project has basic test utilities in `/test/` and `/tools/unit-tester/` but no comprehensive unit test framework. Tests are primarily integration-level.

## Architecture

### High-Level Flow

1. **main.cpp** → **EventDispatcher** → **Core** (manages copy/move operations)
   - EventDispatcher is the central event routing hub
   - Multiple Core instances can exist (one per copy/move window)
   - Core delegates to copy engines and themes via plugins

2. **Plugin System** (5 plugin types):
   - **CopyEngine** - Performs actual file transfers (e.g., Ultracopier-Spec, Rsync, Random)
   - **Listener** - Intercepts system copy/move (e.g., CatchCopy protocol v0002 for Windows Explorer integration)
   - **Themes** - UI rendering (e.g., Oxygen theme)
   - **PluginLoader** - Windows Explorer context menu integration
   - **SessionLoader** - Persistent state (Windows only)

### Core Components

#### Manager Classes (Singleton Pattern)
- **EventDispatcher** - Main event loop coordinator; initializes all managers
- **PluginsManager** - Loads/unloads plugins dynamically; indexes by type
- **ThemesManager** - Theme selection and lifecycle
- **CopyEngineManager** - Copy engine selection based on protocols
- **LanguagesManager** - Translation system
- **OptionEngine** - Settings persistence (QSettings wrapper)
- **ResourcesManager** - File path resolution (read/write paths for portable vs. installed)

#### Copy Operation Flow
```
EventDispatcher::initFunction()
  ↓
Core::newCopy/newMove() [slots receive from listeners/CLI]
  ↓
CopyEngineManager::getCopyEngine() [select engine by protocol]
  ↓
CopyEngine plugin instantiation (Ultracopier-Spec is default)
  ↓
ListThread [scans sources, builds transfer list]
  ↓
TransferThread [transfers files: read/write/collision handling]
  ↓
ThemesManager [displays progress UI]
  ↓
Core::periodicSynchronization() [timer-based sync between engine & UI]
```

### Critical Classes

#### **Core** (`Core.h/cpp`, 9.7KB / 63.5KB)
- Manages multiple simultaneous copy/move operations (one CopyInstance per window)
- Maintains state for each transfer: files, progress, errors, paused state
- Coordinates signals between CopyEngine plugins and UI themes
- Calculates transfer speed and remaining time using two algorithms (Traditional vs. Logarithmic)
- **Key insight**: Works with plugin instances via interfaces, not direct dependencies

#### **CopyEngine (Ultracopier-Spec)** (`plugins/CopyEngine/Ultracopier-Spec/`)
- **ListThread** - Parallel thread scanning sources, building transfer list, handling collisions
  - Sub-modules: ListThreadScan, ListThreadNew, ListThreadActions, ListThreadOptions, ListThreadMedia
  - Handles file/folder collision detection with user interaction
- **TransferThread** - Parallel thread performing actual file I/O
  - Two implementations: `async/` (pthreads + select) or `uring/` (Linux io_uring)
  - Manages pre/post operations (timestamps, permissions, filters)
  - Emits signals for progress updates, errors, completion
- **Factory Pattern**: CopyEngineFactory creates instances for concurrent transfers

#### **Themes (Oxygen)** (`plugins/Themes/Oxygen/`)
- Implements PluginInterface_Themes interface
- Qt UI rendering (interface.ui, interface.cpp)
- TransferModel - Qt model for QTreeView/progress display
- Options UI for theme customization

#### **Listener (CatchCopy v0002)** (`plugins/Listener/catchcopy-v0002/`)
- Protocol handler for Windows Explorer drag-drop integration
- ServerCatchcopy / ClientCatchcopy - IPC via sockets
- Converts explorer operations into Core::newCopy/newMove calls

### Data Flow: Signals and Slots

Plugins communicate via **Qt signals/slots** across thread boundaries:
```
CopyEngine::pushFileProgression → Core::periodicSynchronization() → UI update
CopyEngine::actionInProgess → Core/Interface sync
CopyEngine::error → Error dialogs
CopyEngine::canBeDeleted → Cleanup & window close
```

Each transfer window has separate signal connections, avoiding crosstalk.

## File Structure

```
/sources/
├── main.cpp                           # Entry point
├── EventDispatcher.[h/cpp]            # Central event router
├── Core.[h/cpp]                       # Transfer session manager
├── *Manager.[h/cpp]                   # Singleton managers (Plugins, Themes, Options, etc.)
├── StructEnumDefinition.h             # Core enums (CopyMode, DebugLevel, CopyType, etc.)
├── interface/                         # Plugin interfaces (abstract)
│   ├── PluginInterface_CopyEngine.h
│   ├── PluginInterface_Themes.h
│   ├── PluginInterface_Listener.h
│   └── PluginInterface_*.h
├── plugins/
│   ├── CopyEngine/
│   │   └── Ultracopier-Spec/         # Default copy engine (listthread + transferthread)
│   ├── Listener/
│   │   └── catchcopy-v0002/          # Windows Explorer integration
│   ├── Themes/
│   │   ├── Oxygen/                   # Default theme (Qt UI)
│   │   ├── Oxygen2/
│   │   └── Supercopier/
│   └── Languages/                     # .ts translation files
├── resources/                         # Qt resource files (.qrc)
└── other-pro/                         # Build configuration files
```

## Key Development Notes

### Threading Model
- **Qt Event Loop** (main thread): EventDispatcher, themes, dialogs
- **ListThread** (copy engine): File scanning, collision handling
- **TransferThread** (copy engine): I/O operations (pthreads or io_uring)
- **Communication**: Qt signals/slots with queued connections across threads

### Plugin Architecture
- **All-in-one build** (`ULTRACOPIER_PLUGIN_ALL_IN_ONE`): Plugins compiled into main binary
- **Dynamic load** (without flag): Plugins loaded from `plugins/*/libPluginName.so`
- **Factory pattern**: Each plugin type has a Factory interface for instantiation
- **Interface versioning**: Interfaces use Qt's `Q_DECLARE_INTERFACE` macro

### Options/Settings
- **OptionEngine** stores settings in platform-specific locations:
  - Linux: `~/.config/ultracopier.conf` or `~/.ultracopier.conf`
  - Windows: Registry
  - Portable: Alongside executable (`Variable.h` defines path behavior)
- Options registered at startup in `main.cpp::registerTheOptions()`

### Important Build Defines
- `ULTRACOPIER_DEBUG` - Debug logging
- `ULTRACOPIER_NODEBUG` - Force disable debug
- `ULTRACOPIER_PLUGIN_ALL_IN_ONE` - Compile plugins into main binary
- `ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT` - Inline plugin compilation (no separate compilation)
- `ULTRACOPIER_VERSION_PORTABLE` - Portable installation mode
- `ULTRACOPIER_INTERNET_SUPPORT` - Update checking, product key validation
- `ULTRACOPIER_PLUGIN_KIO` - KDE KIO protocol support
- `ULTRACOPIER_PLUGIN_IO_URING` - Use io_uring instead of pthreads for I/O

### Collision Handling
- FileExistsDialog, FolderExistsDialog, FileIsSameDialog
- User chooses: Skip, Overwrite, Rename, Cancel
- Actions queued in ListThread and applied by TransferThread
- Filters system allows regex-based automatic collision resolution

## Coding Style

- **Prefer nested `if`/`else` over `continue`.** Inside loops, do not use guard `continue;` statements to skip iterations -- wrap the rest of the body in `if(...) { ... }` (with `else` for failure logging when needed). Yes, this nests deeply; that is the style here.

## Git history (read-only reference)

- **When you need history, use the canonical repo at `/mnt/data/perso/progs/ultracopier/git/` — READ-ONLY.** It holds the full git history (the working tree under `…/sources/` may not). Use only read-only commands: `git -C /mnt/data/perso/progs/ultracopier/git log`, `git … show <sha>:<path>`, `git … blame`, `git … log -S'<string>'`, `git … log -G'<regex>'`, `git … diff <a>..<b>`. **NEVER** mutate it — no `checkout`/`switch`/`reset`/`stash`/`commit`/`clean`/`rebase`, nothing that touches its worktree or refs.
- **Current cycle is 3.1.** Tag **`3.1.0.0`** (2026-06-20) marks where 3.1 work started; **`3.0.2.4`** (2026-06-16) is the last stable 3.0 and the async reference point. To see what 3.1 changed vs the stable baseline: `git -C /mnt/data/perso/progs/ultracopier/git diff 3.1.0.0 -- <path>` (or `3.0.2.4`). List tags with `git … tag --sort=version:refname`.

## The async/thread backend is STABLE — do NOT modify it

- The **async (pthread) backend** (`plugins/CopyEngine/Ultracopier-Spec/async/` — `ReadThread`, `WriteThread`, `TransferThreadAsync`) was **perfectly working and stable at version 3.0** and has been **manually tested across ALL cases and ALL features**. Treat it as a **read-only reference implementation**.
- **Never change async code for a new feature unless the user EXPLICITLY asks for an async change.** New behaviour (e.g. resume-at-offset on media reconnect) is **ported FROM async TO io_uring/IOCP** to reach feature parity — async is the source of truth, not the thing to edit. If a task seems to need an async edit, stop and ask first.
- **You CAN change core code (including async/shared) when it is the ONLY way or the OBVIOUS way to fix a real correctness/data-loss/hang bug** (not a new feature). When you do: make the change minimal + targeted, then gate it HARD — re-run the affected case(s) red→green, the full suite (the `move_destination_error` / `file_error_dialog` skip/completion-race tripwires especially), ASan, and the XP/IOCP cross-builds — and if the fix introduces a new failure, first TRY to fix that, and only revert if that fails.

## Build Portability (required — Ultracopier must ALWAYS build)

Ultracopier ships to a **very large range of OS, compilers and platforms** — Windows XP through Windows 11+,
Linux (old kernels through current), macOS, multiple architectures, and toolchains from **gcc/mingw 4.9.2**
up to the latest. **It must always compile on every one of them.** A feature that is unavailable on some
target (IOCP needs Vista+ APIs; io_uring needs liburing + Linux 5.1+) must **gracefully fall back** to the
base async/thread backend — **never** `#error`, never break the build. Concretely:

- **Gate optional backends in the `.pro`, not the C preprocessor.** Use qmake predicates
  (`win32:greaterThan(QT_MAJOR_VERSION, 5)`, `linux:packagesExist(liburing)`) so the unsupported `.cpp`
  is simply not compiled and the async sources are selected instead. Do **not** use `#error`,
  `__has_include` (gcc 5+ only — breaks mingw 4.9.2), or `#include <sdkddkver.h>` tricks to detect
  capability; those either fail the build or have ordering hazards (`sdkddkver.h` locked `_WIN32_WINNT`
  low and broke the MXE build). Stay **mingw 4.9.2 / C++11-compatible** in anything that can reach an old
  toolchain.
- **Every change must keep ALL of these building, fallback included:** IOCP (MXE Qt6 / Windows),
  io_uring (Linux gcc/Qt6 + liburing), async fallback (Linux without liburing — simulate with
  `PKG_CONFIG_LIBDIR=/tmp/empty-pc qmake6 …`), and async on **Qt5 / mingw 4.9.2** (Windows XP). When you
  change backend selection or shared code, compile-verify the affected ones from scratch (let the `.pro`
  gate pick the backend; confirm via the generated Makefile which backend `.cpp` is actually compiled).
- **Always check the real packaging/build environment at `/mnt/data/perso/progs/ultracopier/to-pack/to-send`
  will still compile with your changes.** In particular `./3-compil-wineXP.sh` is the canonical
  Windows-XP/Qt5.6.3/mingw492 build (its `compil` fn rsyncs the live `sources/` into TEMP_PATH and builds
  there). Success markers: `compilation... done` per target and **absent** `plugins not created` /
  `application not created` (wine `make` output goes to `/dev/null` and intermediates are deleted, so judge
  by the produced `.exe`/`.dll` + those markers, not by grepping compile lines). **Do NOT run
  `2-pre-send.sh` (uploads via `rsync … root@…`) or `sync.sh` (`git push origin`) autonomously** — those
  publish; `3-compil-wineXP.sh` alone is enough to verify the XP build.

## Memory Safety & Verification (required after every change)

This is a mature, widely-used project: it must stay stable and bug-free. After **every** code change, before considering it done:

- **Run a memory-safety tool on the code paths you touched** — check for uninitialised variables, use-after-free, double-free, out-of-bounds, leaks, etc. On Linux use **valgrind** (`valgrind --leak-check=full --track-origins=yes`), or a sanitizer build (`-fsanitize=address,undefined`). Read the findings that touch your changed files and fix them; don't dismiss them as noise without checking the origin trace.
- **Exercise the actual code path the change affects, not just a build:**
  - If it impacts the **io_uring** backend, build with `DEFINES+=ULTRACOPIER_PLUGIN_IO_URING` and run a real copy through that path (valgrind 3.27 handles io_uring).
  - If it impacts the **IOCP / Windows** backend, build via MXE and run a real copy on the Windows test box (valgrind can't run PE binaries — rely on a focused static memory review + the runtime copy + content verification there).
  - If it impacts the **shared base / async (thread) backend**, run a copy through the default async build and valgrind it on Linux.
- **Verify content correctness** after a copy on the affected backend: `diff -rq --no-dereference SRC DST` = 0 (content + symlink targets), plus size/mtime/perm spot-checks.
- New members must be initialised in the constructor; new heap allocations must have a matching free on every path (including error/early-return), and async I/O buffers must not be freed while an operation referencing them is still in flight.

## Testing Discipline (never mutate the codebase for a test)

- **Do NOT change the code just to make a test easier.** The shipping behavior must be what you test.
  For example, Ultracopier is tray-resident and intentionally does NOT exit after a copy — so to end a
  headless test run, drive it from OUTSIDE the process (`pkill -x ultracopier`, `kill <pid>`, a timeout,
  polling `du` for completion), never by adding an `exit()`/`quit()`/auto-close into the code. Temporary
  "just for my test" edits (auto-close, hardcoded paths, skipped dialogs, disabled features) corrupt what
  you are validating and have historically been forgotten and shipped. Use external tooling, CLI args that
  already exist, env vars, and separate scratch dirs instead. If a test genuinely cannot be done without a
  code change, stop and ask first.

## Command Safety

- **Recursive/forced deletes have already destroyed real data here — treat them as dangerous.** Do not delete recursively unless you are certain the target holds only disposable data and nothing important will be lost.
- **`rm -Rf <relative-path>` (a relative path) is FORBIDDEN.** A relative path depends on the current directory and can wipe the wrong tree. Recursive force-deletes must use a fully-qualified ABSOLUTE path, e.g. `rm -rf /mnt/data/.../scratch` — never `rm -rf ./build` or `rm -rf sub/`.
- **Before ANY recursive `rm`, enumerate first with `find`.** Run `find <abs-path>` (the same absolute path you are about to delete, recursively listing every file/folder) and actually read the output — confirm it contains only what you expect to destroy and nothing important. Only after that review run the recursive delete. The `find` is a mandatory dry-run, not optional: it shows the full recursive blast radius before you commit to it.
- **Double-check the absolute path before running:** no empty/unset variables (a blank `$VAR` makes `rm -rf $VAR/x` become `rm -rf /x`), no stray `/`, no glob that could expand wider than intended. When unsure, `ls -d <abs-path>` first and confirm it is exactly what you mean.
- Prefer non-destructive cleanup when possible (`make clean`, overwrite in place, or write scratch into a fresh uniquely-named dir). If you are not sure whether the data matters, ask before deleting.

## Common Tasks

### Adding a New Copy Engine Plugin
1. Create `/plugins/CopyEngine/YourEngine/`
2. Implement `PluginInterface_CopyEngine` and `PluginInterface_CopyEngineFactory`
3. Add `informations.xml` metadata
4. Create `.pro` file for qmake or `CMakeLists.txt`
5. Register in `PluginsManager` (automatic if placed in plugins/ directory)

### Modifying the UI Theme
- Edit `plugins/Themes/Oxygen/interface.ui` (Qt Designer format)
- Modify `interface.cpp` for custom rendering
- Register options in `options.ui` and `ThemesFactory.cpp`
- Rebuild: themes are embedded in binary if all-in-one

### Debugging Transfer Operations
- Enable debug in `Variable.h`: uncomment `#define ULTRACOPIER_DEBUG`
- Rebuild with CMake or qmake
- Debug window shows ListThread and TransferThread operations
- Check DebugEngine for log output

### Adding CLI Arguments
- Modify `CliParser.[h/cpp]`
- Hook into `EventDispatcher::initFunction()` for processing
- Examples: `--help`, `--version`, transfer lists

## Compilation Warnings
- Some deprecated Qt functions are intentionally used for Qt5 compatibility
- Unused return values in error paths are normal (explicit intent)
- Static analysis may flag thread-safety; singletonic patterns are verified at runtime

## Platform-Specific Notes

### Windows
- Uses Windows API for native file operations (volume detection, timestamps)
- CatchCopy plugin enables Explorer integration (drag-drop to ultracopier window)
- SessionLoader plugin saves/restores session across reboots
- PluginLoader adds context menu to Explorer

### Linux/Unix
- Uses POSIX APIs (stat, open, read, write)
- No session saving (SessionLoader is Windows-only)
- KIO support available for remote protocols (sftp, smb, ftp)
- io_uring optional for high-performance async I/O

### macOS
- Similar to Unix; official binaries no longer provided (compile yourself)
- No Explorer integration (not applicable)

## External Dependencies
- **Qt 6.7+** (Core, Gui, Widgets, Network, Xml modules)
- **OpenSSL** (for HTTPS update checking)
- **POSIX/Win32 APIs** (for file operations)
- **liburing** (optional, Linux only, for io_uring mode)
- **KF6/KF5** (optional, for KIO protocol support)

## Relevant Files for Common Changes

| Task | Files |
|------|-------|
| Add command-line argument | `CliParser.h/cpp`, `EventDispatcher::initFunction()` |
| Add new manager | `main.cpp::registerTheOptions()`, new Manager singleton |
| Modify copy engine behavior | `CopyEngine.h/cpp`, `ListThread.h/cpp`, `TransferThread.h/cpp` |
| Change UI layout | `plugins/Themes/Oxygen/interface.ui`, `interface.cpp` |
| Add new option | `OptionEngine`, `OptionDialog.ui` |
| Handle new protocol | `CopyEngineManager::getCopyEngine()`, plugin protocol arrays |
| Collision logic | `FileExistsDialog.cpp`, `CopyEngine-collision-and-error.cpp` |

