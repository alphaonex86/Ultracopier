# Ultracopier + Dolphin integration on Gentoo (KF6 / Plasma 6)

This directory packages a reproducible procedure to integrate Ultracopier with
KDE Dolphin on Gentoo, so that:

1. **Ctrl+V (paste) in Dolphin** is handed to Ultracopier when it is running
   (and falls back to native KIO paste when it is not), and
2. the right-click menu offers **Copy/Move (here) with Ultracopier**.

It survives system updates: the Dolphin paste hook is re-applied automatically on
**every** `kde-apps/dolphin` rebuild via Portage's user-patch mechanism, and the
service menus live in your home directory.

## Quick start

```bash
./install.sh
```

Run it as your normal user; it uses `sudo` only for the steps that need root
(`/etc/portage` and the Dolphin rebuild). Then start Ultracopier and test Ctrl+V
in Dolphin.

## What it installs

| Component | Destination | Scope |
|-----------|-------------|-------|
| `../dolphin-0002-...paste.patch` | `/etc/portage/patches/kde-apps/dolphin/` | system, auto re-applied every emerge |
| `servicemenus/ultracopier-*.desktop` | `~/.local/share/kio/servicemenus/` | per-user, survives all updates |

---

## 1. Dolphin Ctrl+V paste override (Portage user-patch)

Dolphin has no copy-engine plugin API, so redirecting paste requires patching its
source. On Gentoo you don't have to re-patch by hand after each upgrade: drop the
patch in `/etc/portage/patches/<category>/<package>/` and Portage applies every
`*.patch` there during `src_prepare` of that package, on every (re)build.

This works because `kde-apps/dolphin` inherits the `ecm` eclass, whose
`src_prepare` chain is `ecm_src_prepare -> cmake_src_prepare ->
default_src_prepare -> eapply_user`. `eapply_user` is what consumes
`/etc/portage/patches`. (Confirmed on Portage 3.0.79; the feature has been default
since Portage 2.3.x / EAPI 7.)

Manual equivalent of what `install.sh` does:

```bash
sudo install -d /etc/portage/patches/kde-apps/dolphin
sudo install -m644 \
  ../dolphin-0002-add-ultracopier-advanced-copier-support-to-paste.patch \
  /etc/portage/patches/kde-apps/dolphin/
sudo emerge -1 kde-apps/dolphin
```

Use the **unversioned** `kde-apps/dolphin/` directory (not
`kde-apps/dolphin-<ver>/`) so the patch applies to all future versions.

### How the hook works

The patch rewrites `DolphinView::pasteToUrl()` (`src/views/dolphinview.cpp`) to:

1. Connect to the UNIX socket `advanced-copier-{uid}` (Catchcopy v0002 protocol).
2. If connected: send `protocol 0002`, then the `cp`/`mv` order with the source
   URLs + destination, and let Ultracopier perform the transfer.
3. If **not** connected (Ultracopier not running): fall back to the original
   `KIO::paste()` — native behavior, no regression.

It also adds `Qt6::Network` to Dolphin's CMake link (already available via
`dev-qt/qtbase[network]`).

Server side that receives it:
`plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ServerCatchcopy.cpp`.

### Qt6 fix baked into the patch

The Catchcopy v0002 frame is `[uint32 size][uint32 orderId][QStringList]`, with a
**4-byte** length header. The original send code wrote the header as:

```cpp
out << int(0);          // placeholder
...
out << block.size();    // real size, written over the placeholder
```

Under Qt5 `QByteArray::size()` returns `int` (4 bytes) — correct. Under **Qt6**
it returns `qsizetype` (`qint64`, 8 bytes), so `QDataStream` writes **8 bytes**,
clobbering the header and the orderId. The server then reads `size = 0`, computes
`0 - sizeof(int)` (unsigned underflow ≈ 4 GB), and aborts with:

```
Error during the reception of the copy/move list
Reply size is >64MB, seam corrupted
```

The patch fixes this by forcing a 4-byte header:

```cpp
out << (quint32)0;
...
out << (quint32)block.size();
```

> Note: the in-tree reference client
> `plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ClientCatchcopy.cpp`
> (around lines 89–93) has the **same** Qt6 bug and should get the same
> `(quint32)` cast. The server (`ServerCatchcopy.cpp`) is correct — it reads a
> `uint32_t` — so only senders need the fix.

---

## 2. KF6 service menus (right-click Copy/Move with Ultracopier)

The old menus used the KF5/Konqueror format (`Type=Service` +
`ServiceTypes=KonqPopupMenu/Plugin`), which **KF6 ignores** — that's why the
entries disappear after upgrading to Plasma 6. The files in `servicemenus/` are
the converted **KF6** format (`Type=Application` + `Actions=` + `[Desktop Action]`,
with a required top-level `Name`, and the literal `?` argument quoted as `"?"` to
satisfy the desktop-entry spec). All four pass `desktop-file-validate`.

They are installed per-user so package updates never touch them:

```bash
install -d ~/.local/share/kio/servicemenus
install -m755 servicemenus/ultracopier-*.desktop ~/.local/share/kio/servicemenus/
kbuildsycoca6   # refresh the menu cache
```

| File | Menu entry | Command |
|------|-----------|---------|
| `ultracopier-cp.desktop` | Copy with Ultracopier | `ultracopier cp %U "?"` |
| `ultracopier-mv.desktop` | Move with Ultracopier | `ultracopier mv %U "?"` |
| `ultracopier-cp-clipboard.desktop` | Copy here with Ultracopier | `ultracopier CBcp %U` |
| `ultracopier-mv-clipboard.desktop` | Move here with Ultracopier | `ultracopier CBmv %U` |

Old root-owned KF5 files under `/usr/share/kio/servicemenus/` are harmless
(ignored by KF6) but can be removed; the per-user files override them by filename
regardless.

---

## Troubleshooting

| Symptom | Cause / fix |
|---------|-------------|
| `Reply size is >64MB, seam corrupted` | Qt6 8-byte header bug — make sure you deployed the **fixed** patch, then re-emerge Dolphin. |
| Ctrl+V uses native copy, no Ultracopier | Ultracopier isn't running (expected fallback), or Dolphin wasn't rebuilt with the patch. |
| A patch hunk is rejected during emerge | Dolphin source drifted; `eapply` allows no fuzz. Grab the `*.rej` from the build dir and rebase the patch. |
| Compile error: `KUrlMimeData` undeclared | Add `#include <KUrlMimeData>` to `src/views/dolphinview.cpp` in the patch. |
| Right-click entries missing | Old KF5-format menus; install the KF6 ones here and run `kbuildsycoca6`. |
