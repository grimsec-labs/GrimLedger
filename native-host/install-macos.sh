#!/usr/bin/env bash
set -euo pipefail

HOST_EXE=""
EXTENSION_ID=""
BROWSER="both"

usage() {
  echo "Usage: $0 --extension-id <32-char-id> [--host-exe path] [--browser chrome|chromium|both]"
  exit 1
}

read_store_extension_id() {
  local id_file
  for id_file in \
    "$SCRIPT_DIR/extension-id.txt" \
    "$SCRIPT_DIR/../installer/extension-id.txt" \
    "$HOME/Library/Application Support/GrimLedger/native-host/extension-id.txt"; do
    if [[ -f "$id_file" ]]; then
      tr -d '[:space:]' < "$id_file"
      return 0
    fi
  done
  return 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host-exe) HOST_EXE="$2"; shift 2 ;;
    --extension-id) EXTENSION_ID="$2"; shift 2 ;;
    --browser) BROWSER="$2"; shift 2 ;;
    -h|--help) usage ;;
    *) usage ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -z "$HOST_EXE" ]]; then
  if [[ -x "$SCRIPT_DIR/grimledger_host" ]]; then
    HOST_EXE="$SCRIPT_DIR/grimledger_host"
  elif [[ -x "$SCRIPT_DIR/../grimledger_host" ]]; then
    HOST_EXE="$SCRIPT_DIR/../grimledger_host"
  elif [[ -x "$SCRIPT_DIR/../build/grimledger_host" ]]; then
    HOST_EXE="$SCRIPT_DIR/../build/grimledger_host"
  else
    echo "Native host not found. Build GrimLedger first or pass --host-exe." >&2
    exit 1
  fi
fi

if [[ -z "$EXTENSION_ID" ]]; then
  if EXTENSION_ID="$(read_store_extension_id)" && [[ "$EXTENSION_ID" =~ ^[a-p]{32}$ ]]; then
    :
  else
    usage
  fi
fi

if [[ ! "$EXTENSION_ID" =~ ^[a-p]{32}$ ]]; then
  usage
fi

INSTALL_DIR="$HOME/Library/Application Support/GrimLedger/native-host"
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
    register_browser "$HOME/Library/Application Support/Google/Chrome/NativeMessagingHosts"
    ;;
  chromium)
    register_browser "$HOME/Library/Application Support/Chromium/NativeMessagingHosts"
    ;;
  both)
    register_browser "$HOME/Library/Application Support/Google/Chrome/NativeMessagingHosts"
    register_browser "$HOME/Library/Application Support/Chromium/NativeMessagingHosts"
    ;;
  *) usage ;;
esac

echo "Installed native messaging host."
echo "  Host: $INSTALL_DIR/grimledger_host"
echo "  Manifest: $MANIFEST_OUT"
echo "  Extension origin: $ORIGIN"
