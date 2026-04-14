# Hooking Dolphin's Ctrl+V to redirect copy/move to Ultracopier

## Current mechanism: Catchcopy v0002 socket protocol

Ultracopier already solves this via the **Catchcopy v0002 protocol**. A patched
Dolphin tries to connect to a UNIX socket `advanced-copier-{uid}`, sends the
copy/move command to Ultracopier, and falls back to KIO if Ultracopier isn't
running.

Patch: `dolphin-0002-add-ultracopier-advanced-copier-support-to-paste.patch`

The flow:
1. User presses Ctrl+V in Dolphin
2. Patched `pasteToUrl()` tries `QLocalSocket::connectToServer("advanced-copier-{uid}")`
3. If connected: sends file list + destination via Catchcopy protocol, Ultracopier handles the copy
4. If not connected: falls back to standard `KIO::paste()` (native behavior)

Server-side listener: `plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ServerCatchcopy.cpp`

## Alternative: LD_PRELOAD .so hook

A `.so` loaded via `LD_PRELOAD` could intercept Qt/KIO paste functions:

- **Possible targets**: `QClipboard::mimeData()`, `KIO::paste()`, `KIO::PasteJob` constructor,
  or lower-level `open()`/`write()` syscalls
- **How**: `LD_PRELOAD=/path/to/ultracopier-hook.so dolphin` -- the .so would
  `dlsym(RTLD_NEXT, ...)` the original function, check if Ultracopier's socket is
  available, redirect if yes, call original if no
- **Launch**: could be injected via a desktop file wrapper or `~/.config/environment.d/`

### Why the .so hook is worse than the current patch approach

| Aspect | LD_PRELOAD .so hook | Catchcopy patch (current) |
|--------|---------------------|---------------------------|
| **Fragility** | Breaks on KIO/Qt symbol renames, C++ name mangling changes between versions | Patches a stable high-level function |
| **C++ ABI** | Must match exact compiler, Qt version, KIO version -- one mismatch = crash | Source patch compiles against whatever KDE version the user builds |
| **Security** | `LD_PRELOAD` is blocked by some hardened distros, setuid binaries, Flatpak/Snap | Normal compiled code, no restrictions |
| **Maintenance** | Must reverse-engineer internal symbols per Dolphin release | Patch is ~50 lines against a stable public API |
| **Scope** | Hooks everything in the process, risk of unintended side-effects | Surgically modifies one function |

## Other no-patch alternatives considered

### D-Bus KIO integration

KDE has discussed a `org.freedesktop.FileManager1` D-Bus interface. Ultracopier
could register as a handler. But Dolphin doesn't use it for paste operations today.

A legacy D-Bus listener exists in `plugins-unmaintained/Listener/dbus/` (registers
`info.first-world.catchcopy` on D-Bus) but was replaced by the socket approach.

### KIO Worker plugin (formerly KIO Slave)

Write a KIO worker that Dolphin would use for `file://` operations. But Dolphin
hardcodes the `file` KIO worker; you can't override it without patching Dolphin
anyway.

## Conclusion

The `LD_PRELOAD` approach is technically possible but fragile and not worth
pursuing. The current Catchcopy socket + Dolphin patch is the correct
architecture:
- Clean and non-invasive
- Only activates when Ultracopier is running
- Falls back gracefully to native KIO
- Works across Dolphin versions without ABI concerns

The only downside is users must build a patched Dolphin, but that's inherent to
Dolphin not having a copy-engine plugin API.
