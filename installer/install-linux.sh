#!/usr/bin/env bash
# Install GrimLedger for the current user (~/.local).
set -euo pipefail

if [[ "${EUID:-$(id -u)}" -eq 0 ]] || [[ "${HOME:-}" == "/root" ]]; then
    echo "Do not run as root or with sudo. GrimLedger installs per-user under ~/.local." >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_ROOT="${XDG_DATA_HOME:-$HOME/.local/share}/GrimLedger"
APP_DIR="$INSTALL_ROOT/app"
EXT_DIR="$INSTALL_ROOT/browser-extension"
BIN_DIR="$HOME/.local/bin"
DESKTOP_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
DEV_LINK="$HOME/GrimLedger-browser-extension"

EXTENSION_ID=""
BROWSER="all"
SKIP_NATIVE_HOST=0
DEV_EXTENSION_LINK=1

usage() {
    cat <<'EOF'
Usage: install.sh [options]

Options:
  --extension-id ID     Chrome extension ID (32 chars a-p); prompts if omitted
  --browser TARGET      chrome|chromium|brave|opera|edge|both|all (default: all)
  --skip-native-host    Do not register the native messaging host
  --dev-extension-link  Create ~/GrimLedger-browser-extension symlink (default)
  --no-dev-extension-link
                        Do not create the dev symlink
  -h, --help            Show this help

Do not run with sudo. Vault and browser bridge paths are per-user under ~/.local.
EOF
}

read_store_extension_id() {
    local id_file
    for id_file in "$SCRIPT_DIR/extension-id.txt" "$SCRIPT_DIR/../installer/extension-id.txt"; do
        if [[ -f "$id_file" ]]; then
            tr -d '[:space:]' < "$id_file"
            return 0
        fi
    done
    return 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --extension-id) EXTENSION_ID="$2"; shift 2 ;;
        --browser) BROWSER="$2"; shift 2 ;;
        --skip-native-host) SKIP_NATIVE_HOST=1; shift ;;
        --dev-extension-link) DEV_EXTENSION_LINK=1; shift ;;
        --no-dev-extension-link) DEV_EXTENSION_LINK=0; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
    esac
done

if [[ ! -x "$SCRIPT_DIR/GrimLedger" ]]; then
    echo "GrimLedger binary not found. Run installer/build-linux.sh first." >&2
    exit 1
fi

echo "Installing GrimLedger to $APP_DIR ..."
mkdir -p "$APP_DIR" "$EXT_DIR" "$BIN_DIR" "$DESKTOP_DIR"

if [[ -d "$SCRIPT_DIR/lib" || -d "$SCRIPT_DIR/plugins" ]]; then
    rsync -a --delete "$SCRIPT_DIR/" "$APP_DIR/" \
        --exclude browser-extension --exclude native-host \
        --exclude install.sh --exclude README.txt --exclude GrimLedger.desktop \
        --exclude extension-id.txt
else
    cp "$SCRIPT_DIR/GrimLedger" "$APP_DIR/"
    cp "$SCRIPT_DIR/grimledger_host" "$APP_DIR/"
    chmod 755 "$APP_DIR/GrimLedger" "$APP_DIR/grimledger_host"
fi

cp -r "$SCRIPT_DIR/browser-extension/." "$EXT_DIR/"
mkdir -p "$APP_DIR/native-host"
cp -r "$SCRIPT_DIR/native-host/." "$APP_DIR/native-host/"
if [[ -f "$SCRIPT_DIR/extension-id.txt" ]]; then
    cp "$SCRIPT_DIR/extension-id.txt" "$APP_DIR/native-host/extension-id.txt"
fi

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

if [[ "$DEV_EXTENSION_LINK" -eq 1 ]]; then
    ln -sfn "$EXT_DIR" "$DEV_LINK"
fi

echo ""
echo "Desktop app installed."
echo "  App:        $APP_DIR"
echo "  CLI:        grimledger  (via $BIN_DIR)"
echo "  Extension:  $EXT_DIR"
if [[ "$DEV_EXTENSION_LINK" -eq 1 ]]; then
    echo "  Dev link:   $DEV_LINK"
fi
echo ""
echo "=== Browser bridge ==="

STORE_ID=""
if STORE_ID="$(read_store_extension_id)" && [[ "$STORE_ID" =~ ^[a-p]{32}$ ]]; then
    echo "Store extension ID: $STORE_ID"
    echo "When the Chrome Web Store listing is live, install the extension from the store."
    echo "Until then, load unpacked from: ${DEV_LINK:-$EXT_DIR}"
else
    echo "1. Open chrome://extensions, enable Developer mode, Load unpacked"
    echo "   Folder: ${DEV_LINK:-$EXT_DIR}"
    echo "   (Hidden path: $EXT_DIR — press Ctrl+H in the file picker to show dotfolders)"
fi
echo "2. Enable browser bridge in GrimLedger Settings and Save Settings"
echo "3. Keep GrimLedger unlocked while filling logins"
echo ""

if [[ "$SKIP_NATIVE_HOST" -eq 0 ]]; then
    if [[ -z "$EXTENSION_ID" ]]; then
        DEFAULT_ID=""
        if DEFAULT_ID="$(read_store_extension_id)" && [[ "$DEFAULT_ID" =~ ^[a-p]{32}$ ]]; then
            read -r -p "Extension ID for native host [$DEFAULT_ID]: " EXTENSION_ID || true
            EXTENSION_ID="${EXTENSION_ID:-$DEFAULT_ID}"
        else
            read -r -p "Paste extension ID from chrome://extensions (Enter to skip): " EXTENSION_ID || true
        fi
    fi

    if [[ "$EXTENSION_ID" =~ ^[a-p]{32}$ ]]; then
        "$APP_DIR/native-host/install-linux.sh" \
            --extension-id "$EXTENSION_ID" \
            --host-exe "$APP_DIR/grimledger_host" \
            --browser "$BROWSER"
    else
        echo "Skipping native-host registration. Re-run later:"
        echo "  $APP_DIR/native-host/install-linux.sh --extension-id EXT_ID --host-exe $APP_DIR/grimledger_host"
    fi
fi

echo ""
echo "Vault location: ~/.local/share/GrimLedger/vault.grim"
