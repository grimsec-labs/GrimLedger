#!/usr/bin/env bash
# Install GrimLedger to ~/Applications for the current user.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DEST="$HOME/Applications/GrimLedger.app"
SUPPORT_DIR="$HOME/Library/Application Support/GrimLedger"
EXT_DIR="$SUPPORT_DIR/browser-extension"
HOST_DIR="$SUPPORT_DIR/native-host"

if [[ ! -d "$SCRIPT_DIR/GrimLedger.app" ]]; then
    echo "GrimLedger.app not found. Run installer/build-macos.sh first." >&2
    exit 1
fi

echo "Installing GrimLedger to $APP_DEST ..."
rm -rf "$APP_DEST"
cp -R "$SCRIPT_DIR/GrimLedger.app" "$APP_DEST"

mkdir -p "$EXT_DIR" "$HOST_DIR"
cp -r "$SCRIPT_DIR/browser-extension/." "$EXT_DIR/"
cp "$SCRIPT_DIR/grimledger_host" "$HOST_DIR/"
chmod 700 "$HOST_DIR/grimledger_host"
cp -r "$SCRIPT_DIR/native-host/." "$HOST_DIR/"

echo ""
echo "Desktop app installed."
echo "  App:        $APP_DEST"
echo "  Extension:  $EXT_DIR"
echo "  Native host: $HOST_DIR/grimledger_host"
echo ""
echo "Next steps:"
echo "  1. Open GrimLedger from Applications or: open \"$APP_DEST\""
echo "  2. Load unpacked extension from: $EXT_DIR"
echo "  3. Enable browser bridge in GrimLedger Settings"
echo "  4. Register native host (replace EXT_ID from chrome://extensions):"
echo "     $HOST_DIR/install-macos.sh --extension-id EXT_ID --host-exe $HOST_DIR/grimledger_host"
echo ""
echo "Vault location: ~/Library/Application Support/GrimLedger/vault.grim"
