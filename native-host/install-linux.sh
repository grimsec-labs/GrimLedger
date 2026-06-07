#!/usr/bin/env bash
set -euo pipefail

HOST_EXE=""
EXTENSION_ID=""
BROWSER="both"

usage() {
  echo "Usage: $0 --extension-id <32-char-id> [--host-exe path] [--browser chrome|chromium|both]"
  echo "Load browser-extension/ unpacked, copy ID from chrome://extensions"
  exit 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host-exe) HOST_EXE="$2"; shift 2 ;;
    --extension-id) EXTENSION_ID="$2"; shift 2 ;;
    --browser) BROWSER="$2"; shift 2 ;;
    *) usage ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -z "$HOST_EXE" ]]; then
  if [[ -x "$SCRIPT_DIR/../build/grimledger_host" ]]; then
    HOST_EXE="$SCRIPT_DIR/../build/grimledger_host"
  elif [[ -x "$SCRIPT_DIR/../build/Release/grimledger_host" ]]; then
    HOST_EXE="$SCRIPT_DIR/../build/Release/grimledger_host"
  else
    echo "Native host not found. Build GrimLedger first."
    exit 1
  fi
fi

if [[ ! "$EXTENSION_ID" =~ ^[a-p]{32}$ ]]; then
  usage
fi

INSTALL_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/GrimLedger/native-host"
mkdir -p "$INSTALL_DIR"
cp "$HOST_EXE" "$INSTALL_DIR/grimledger_host"
chmod 700 "$INSTALL_DIR/grimledger_host"

MANIFEST_OUT="$INSTALL_DIR/com.grimledger.bridge.json"
ORIGIN="chrome-extension://${EXTENSION_ID}/"
HOST_PATH="$INSTALL_DIR/grimledger_host"
sed \
  -e "s|@GRIMLEDGER_HOST_PATH@|${HOST_PATH//\\/\\\\}|g" \
  -e "s|@CHROME_EXTENSION_ORIGIN@|${ORIGIN}|g" \
  "$SCRIPT_DIR/com.grimledger.bridge.json" > "$MANIFEST_OUT"
chmod 600 "$MANIFEST_OUT"

register_browser() {
  local target_dir="$1"
  mkdir -p "$target_dir"
  ln -sf "$MANIFEST_OUT" "$target_dir/com.grimledger.bridge.json"
}

case "$BROWSER" in
  chrome)
    register_browser "${XDG_CONFIG_HOME:-$HOME/.config}/google-chrome/NativeMessagingHosts"
    ;;
  chromium)
    register_browser "${XDG_CONFIG_HOME:-$HOME/.config}/chromium/NativeMessagingHosts"
    ;;
  both)
    register_browser "${XDG_CONFIG_HOME:-$HOME/.config}/google-chrome/NativeMessagingHosts"
    register_browser "${XDG_CONFIG_HOME:-$HOME/.config}/chromium/NativeMessagingHosts"
    ;;
  *) usage ;;
esac

echo "Installed native messaging host."
echo "  Host: $INSTALL_DIR/grimledger_host"
echo "  Manifest: $MANIFEST_OUT"
echo "  Extension origin: $ORIGIN"
