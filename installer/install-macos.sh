#!/usr/bin/env bash
# Install GrimLedger to ~/Applications for the current user.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DEST="$HOME/Applications/GrimLedger.app"
SUPPORT_DIR="$HOME/Library/Application Support/GrimLedger"
EXT_DIR="$SUPPORT_DIR/browser-extension"
HOST_DIR="$SUPPORT_DIR/native-host"
DEV_LINK="$HOME/GrimLedger-browser-extension"

EXTENSION_ID=""
BROWSER="both"
SKIP_NATIVE_HOST=0
DEV_EXTENSION_LINK=1

usage() {
    cat <<'EOF'
Usage: install.sh [options]

Options:
  --extension-id ID     Chrome extension ID (32 chars a-p); prompts if omitted
  --browser TARGET      chrome|chromium|both (default: both)
  --skip-native-host    Do not register the native messaging host
  --dev-extension-link  Create ~/GrimLedger-browser-extension symlink (default)
  --no-dev-extension-link
  -h, --help            Show this help
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
if [[ -f "$SCRIPT_DIR/extension-id.txt" ]]; then
    cp "$SCRIPT_DIR/extension-id.txt" "$HOST_DIR/extension-id.txt"
fi

if [[ "$DEV_EXTENSION_LINK" -eq 1 ]]; then
    ln -sfn "$EXT_DIR" "$DEV_LINK"
fi

echo ""
echo "Desktop app installed."
echo "  App:         $APP_DEST"
echo "  Extension:   $EXT_DIR"
if [[ "$DEV_EXTENSION_LINK" -eq 1 ]]; then
    echo "  Dev link:    $DEV_LINK"
fi
echo ""
echo "=== Browser bridge ==="

STORE_ID=""
if STORE_ID="$(read_store_extension_id)" && [[ "$STORE_ID" =~ ^[a-p]{32}$ ]]; then
    echo "Store extension ID: $STORE_ID"
    echo "When the Chrome Web Store listing is live, install the extension from the store."
    echo "Until then, load unpacked from: ${DEV_LINK:-$EXT_DIR}"
else
    echo "1. Load unpacked from: ${DEV_LINK:-$EXT_DIR}"
fi
echo "2. Enable browser bridge in GrimLedger Settings and Save Settings"
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
        "$HOST_DIR/install-macos.sh" \
            --extension-id "$EXTENSION_ID" \
            --host-exe "$HOST_DIR/grimledger_host" \
            --browser "$BROWSER"
    else
        echo "Skipping native-host registration. Re-run later:"
        echo "  $HOST_DIR/install-macos.sh --extension-id EXT_ID --host-exe $HOST_DIR/grimledger_host"
    fi
fi

echo ""
echo "Vault location: ~/Library/Application Support/GrimLedger/vault.grim"
