# Ultracopier / Supercopier — Frequently Asked Questions

This FAQ is compiled from real user support questions and the answers given by
the developer (BRULE Herman / alpha_one_x86). Ultracopier and Supercopier share
the same engine; answers below apply to both unless noted.

- Website: https://ultracopier.herman-brule.com/
- Source & docs: https://github.com/alphaonex86/Ultracopier
- Documentation: https://github.com/alphaonex86/Ultracopier/tree/master/docs

---

## Licensing & the "Ultimate" version

### Why isn't the latest (major) version free? I had a key for an older version.

Major version updates are paid in order to fund the project. Ultracopier is
maintained by a single developer, with no outside help, who also keeps the
website running, fights piracy/hacking attempts, and provides user support
(historically fixing bugs in under two hours).

For context:
- The **1.x** series received free updates for more than 5 years.
- The **2.x** series received free updates for more than 7 years.

A license is tied to a major version. A **v2 key does not unlock v3** — v3 is a
new major version. Buying is essentially a way to support continued development.

### I don't want to pay — can I keep using the version I have?

Yes. You can keep using your existing (older) version indefinitely, and simply
**disable update detection** in the options so it stops prompting you to upgrade.

### Is buying mandatory to use Ultracopier?

No. Purchasing funds the project. The free version is fully usable; the paid
("Ultimate") key removes update nagging and supports development. You are also
welcome to promote the project and encourage others to buy if they want to help.

### What payment methods are accepted?

Stripe (credit card) and PayPal only.

### Can I get a refund?

Sales are **non-refundable** — taxes on the sale are already paid, and
the intended process is to **test the free version first** and only buy once
you've validated it works for you. That said, the developer will still do his
best to find and fix any problem you hit.

### I lost my key / need it re-sent / want to back it up.

Keep your key safe (e.g. `UC...`). If you bought a license and lost it, contact
support at the address below with your purchase details so it can be re-issued.

---

## Download & installation

### The installer fails / "Install error" on Windows.

The most common cause is that **Ultracopier is already running**. Close it first
(including the system-tray icon near the clock), then run the installer again.
Re-trying the install after fully closing the app resolves most install errors.

### Which Windows versions are supported?

Windows XP and later (including Windows 10/11). On a freshly installed Windows 11
the known slowdowns/issues are not present.

---

## Antivirus & security warnings

### My antivirus / SmartScreen flags Ultracopier as unsafe.

The official binaries are **not code-signed**, so Windows SmartScreen and some
antivirus products show a warning or report it as untrusted. This is a false
positive caused by the missing signature, not malware. You can trust the file as
long as you **download it from the official website**:

```
https://ultracopier.herman-brule.com/
```

If you prefer, you can also build from source (see below).

---

## Usage & settings

### Ultracopier doesn't intercept copy/move — the standard system copy is still used.

Ultracopier replaces the system copy via its listener integration. Make sure the
"copy/move interception" (listener) option is enabled, and that Ultracopier is
actually running in the system tray. On Windows the Explorer integration must be
active for it to take over standard copy/paste.

### When a file already exists, what does Ultracopier do?

By default it **asks you what to do** (skip, overwrite, rename, etc.) for each
collision.

### My chosen collision action (skip / overwrite / "put to bottom") isn't remembered.

Configure a default action so you aren't asked every time:
**Right-click the system-tray icon (near the clock) → Options**, and set
**"always perform this action"** for the collision type you want automated.

### What does the "OS Buffer" option do?

It lets Ultracopier use the operating system's write buffer. Writes become
faster, but `close()` at the end of each file takes longer. Practically: the
progress bar moves quickly during the copy and then appears to pause/slow down
right at the end of each file while the OS flushes the buffer.

### What is "native OS copy" and when should I use it?

It hands the copy off to the OS's own copy routine instead of Ultracopier's
read/write loop. Try enabling it (in the performance options) if you experience
slow transfers — slowness is often a device-speed limitation, and native copy
can perform better in those cases.

---

## Performance

### Copies are slow / slower than expected.

- The bottleneck is frequently the **device speed** (slow USB drive, NAS, SD
  card), not Ultracopier itself.
- Try the **"native OS copy"** option in the performance settings.
- The **OS Buffer** option also affects perceived speed (see above).

---

## File integrity & permissions

### Does Ultracopier verify copied files (checksum)?

Yes — a basic **checksum verification** is available in the options. Note the
inherent limitation of any checksum-on-copy: if the disk reports a correct value
back while having physically written a wrong value, verification can still pass.

### Are file permissions/rights preserved?

Yes. The original source file's rights are copied to the destination.

---

## Platform notes

### Linux

- Install from your distribution's package or build from source (Qt 6.7+).
  On Arch-based systems such as Manjaro, build/install from source if no package
  is available.
- File-manager integration (e.g. Nemo on Cinnamon, Dolphin on KDE) is available
  via the matching integration; see the project docs.
- Optional protocol support (sftp/smb/ftp) is available through KDE KIO when
  built with that plugin.

### macOS

- Official macOS binaries are no longer reliably provided, and a given macOS
  build may **not be compatible** with your system/version.
- The recommended path on macOS is to **compile from source with Qt** and run it
  from there (which also makes it possible to debug issues).
- There is no Finder/Explorer-style integration on macOS.

### Windows

- Full Explorer integration (intercepting copy/move, drag-and-drop into the
  Ultracopier window) is supported.
- Remember to close the running instance before installing/updating.

---

## Troubleshooting & crashes

### Ultracopier crashes randomly, even without copying.

- Note which version **does** work for you, and whether the crash is systematic
  or random (does it also happen with a single file?).
- Random crashes with no copy in progress can indicate **faulty RAM** — test your
  memory with a tool like **memtest86+**.
- The developer tests on both real hardware and virtual machines without
  reproducing such crashes, so environment-specific factors matter.

### I get an error creating folders on a NAS.

Errors should occur on the **destination**, not the source — Ultracopier creates
folders on the destination only. Some NAS devices behave awkwardly here; mounting
the NAS as a **network drive** instead of accessing it directly often avoids the
problem. Testing against a Linux/Debian NAS works without issue.

### How do I report a hard-to-reproduce bug?

Provide: the exact version, whether it's reproducible (and with a single file),
the source/destination types (local, USB, NAS…), and — when possible — the
**downloaded installer file** or a debug build's output. For deep issues,
building from source and running under a debugger is the fastest path to a fix.

---

## Building from source

The most reliable way to diagnose platform issues (especially on macOS/Linux) is
to compile from source:

1. Install Qt (6.7+) and a C++17 toolchain.
2. Clone https://github.com/alphaonex86/Ultracopier
3. Build with qmake (recommended) or CMake:
   ```bash
   qmake ultracopier.pro
   make -j$(nproc)
   ./ultracopier
   ```
4. See `docs/` and `CLAUDE.md`/`README` for optional features (KIO, io_uring,
   portable mode).

---

## Documentation & support

- Documentation: https://github.com/alphaonex86/Ultracopier/tree/master/docs
- Issues / bug tracker: https://github.com/alphaonex86/Ultracopier/issues
- Support contact: `ultracopier@herman-brule.com`

> Ultracopier/Supercopier is developed and maintained by a single person. Please
> test the free version before purchasing, and include as much detail as possible
> when reporting a problem so it can be fixed quickly.
