#!/usr/bin/env bash
# Install GrimLedger for the current user (~/.local).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_ROOT="${XDG_DATA_HOME:-$HOME/.local/share}/GrimLedger"
APP_DIR="$INSTALL_ROOT/app"
EXT_DIR="$INSTALL_ROOT/browser-extension"
BIN_DIR="$HOME/.local/bin"
DESKTOP_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"

if [[ ! -x "$SCRIPT_DIR/GrimLedger" ]]; then
    echo "GrimLedger binary not found. Run installer/build-linux.sh first." >&2
    exit 1
fi

echo "Installing GrimLedger to $APP_DIR ..."
mkdir -p "$APP_DIR" "$EXT_DIR" "$BIN_DIR" "$DESKTOP_DIR"

if [[ -d "$SCRIPT_DIR/lib" || -d "$SCRIPT_DIR/plugins" ]]; then
    rsync -a --delete "$SCRIPT_DIR/" "$APP_DIR/" \
        --exclude browser-extension --exclude native-host \
        --exclude install.sh --exclude README.txt --exclude GrimLedger.desktop
else
    cp "$SCRIPT_DIR/GrimLedger" "$APP_DIR/"
    cp "$SCRIPT_DIR/grimledger_host" "$APP_DIR/"
    chmod 755 "$APP_DIR/GrimLedger" "$APP_DIR/grimledger_host"
fi

cp -r "$SCRIPT_DIR/browser-extension/." "$EXT_DIR/"
mkdir -p "$APP_DIR/native-host"
cp -r "$SCRIPT_DIR/native-host/." "$APP_DIR/native-host/"

ln -sf "$APP_DIR/GrimLedger" "$BIN_DIR/grimledger"

cat > "$DESKTOP_DIR/grimledger.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=GrimLedger
Comment=Encrypted markdown vault and password manager
Exec=$APP_DIR/GrimLedger
Icon=grimledger
Terminal=false
Categories=Office;Security;
StartupWMClass=GrimLedger
EOF

echo ""
echo "Desktop app installed."
echo "  App:        $APP_DIR"
echo "  CLI:        grimledger  (via $BIN_DIR)"
echo "  Extension:  $EXT_DIR"
echo ""
echo "Next steps:"
echo "  1. Load unpacked extension from: $EXT_DIR"
echo "  2. Enable browser bridge in GrimLedger Settings"
echo "  3. Register native host (replace EXT_ID from chrome://extensions):"
echo "     $APP_DIR/native-host/install-linux.sh --extension-id EXT_ID --host-exe $APP_DIR/grimledger_host"
echo ""
echo "Vault location: ~/.local/share/GrimLedger/vault.grim"
