#!/bin/bash
#
# Gentoo integration installer for Ultracopier + Dolphin (KF6 / Plasma 6).
#
# Installs two things:
#   1. A Portage user-patch that makes Dolphin's Ctrl+V (paste) hand the
#      copy/move to Ultracopier when it is running, falling back to native
#      KIO paste otherwise. Re-applied automatically on every Dolphin emerge.
#   2. The KF6 service-menu files (right-click "Copy/Move with Ultracopier").
#
# Run as your normal user (it calls sudo where root is needed):
#   ./install.sh
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PATCH="$SCRIPT_DIR/../dolphin-0002-add-ultracopier-advanced-copier-support-to-paste.patch"
PATCH_DEST="/etc/portage/patches/kde-apps/dolphin"
MENU_DEST="$HOME/.local/share/kio/servicemenus"

if [ ! -f "$PATCH" ]; then
    echo "ERROR: patch not found: $PATCH" >&2
    exit 1
fi

echo ">> [1/4] Installing Dolphin paste patch -> $PATCH_DEST/"
sudo install -d "$PATCH_DEST"
sudo install -m644 "$PATCH" "$PATCH_DEST/"

echo ">> [2/4] Installing KF6 service menus -> $MENU_DEST/"
install -d "$MENU_DEST"
install -m755 "$SCRIPT_DIR"/servicemenus/ultracopier-*.desktop "$MENU_DEST/"

echo ">> [3/4] Rebuilding kde-apps/dolphin (applies the patch now;"
echo "         Portage re-applies it automatically on every future upgrade)"
sudo emerge --oneshot --quiet-build=n kde-apps/dolphin

echo ">> [4/4] Refreshing KDE service cache"
command -v kbuildsycoca6 >/dev/null 2>&1 && kbuildsycoca6 >/dev/null 2>&1 || true

cat <<'EOF'

>> Done.

Notes:
  * Stale KF5-format menus may still exist (they are IGNORED by KF6, and the
    per-user files above override them by filename). To remove them:
        sudo rm -f /usr/share/kio/servicemenus/ultracopier-{cp,cp-clipboard,mv,mv-clipboard}.desktop
  * The paste override only activates while Ultracopier is running; otherwise
    Dolphin uses its native copy. Start Ultracopier, then test Ctrl+V.
EOF
