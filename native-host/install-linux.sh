#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID:-$(id -u)}" -eq 0 ]] || [[ "${HOME:-}" == "/root" ]]; then
  echo "Do not run as root or with sudo. Native messaging hosts register under your user ~/.config." >&2
  exit 1
fi

HOST_EXE=""
EXTENSION_ID=""
BROWSER="all"

usage() {
  echo "Usage: $0 --extension-id <32-char-id> [--host-exe path] [--browser chrome|chromium|brave|opera|edge|both|all]"
  echo "Load browser-extension/ unpacked or install from Chrome Web Store, then copy ID from chrome://extensions"
  exit 1
}

read_store_extension_id() {
  local id_file
  for id_file in \
    "$SCRIPT_DIR/extension-id.txt" \
    "$SCRIPT_DIR/../installer/extension-id.txt" \
    "${XDG_DATA_HOME:-$HOME/.local/share}/GrimLedger/app/native-host/extension-id.txt"; do
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
  if [[ -x "$SCRIPT_DIR/../grimledger_host" ]]; then
    HOST_EXE="$SCRIPT_DIR/../grimledger_host"
  elif [[ -x "$SCRIPT_DIR/grimledger_host" ]]; then
    HOST_EXE="$SCRIPT_DIR/grimledger_host"
  elif [[ -x "$SCRIPT_DIR/../build/grimledger_host" ]]; then
    HOST_EXE="$SCRIPT_DIR/../build/grimledger_host"
  elif [[ -x "$SCRIPT_DIR/../build/Release/grimledger_host" ]]; then
    HOST_EXE="$SCRIPT_DIR/../build/Release/grimledger_host"
  elif [[ -x "$SCRIPT_DIR/../../build-linux/grimledger_host" ]]; then
    HOST_EXE="$SCRIPT_DIR/../../build-linux/grimledger_host"
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

if [[ ! -x "$HOST_EXE" ]]; then
  echo "Native host binary not executable: $HOST_EXE" >&2
  exit 1
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

CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"

register_browser() {
  local target_dir="$1"
  mkdir -p "$target_dir"
  ln -sf "$MANIFEST_OUT" "$target_dir/com.grimledger.bridge.json"
  echo "  Registered: $target_dir"
}

register_chrome_family() {
  register_browser "$CONFIG_HOME/google-chrome/NativeMessagingHosts"
  register_browser "$CONFIG_HOME/chromium/NativeMessagingHosts"
  register_browser "$CONFIG_HOME/BraveSoftware/Brave-Browser/NativeMessagingHosts"
  register_browser "$CONFIG_HOME/opera/NativeMessagingHosts"
  register_browser "$CONFIG_HOME/microsoft-edge/NativeMessagingHosts"
}

case "$BROWSER" in
  chrome)
    register_browser "$CONFIG_HOME/google-chrome/NativeMessagingHosts"
    ;;
  chromium)
    register_browser "$CONFIG_HOME/chromium/NativeMessagingHosts"
    ;;
  brave)
    register_browser "$CONFIG_HOME/BraveSoftware/Brave-Browser/NativeMessagingHosts"
    ;;
  opera)
    register_browser "$CONFIG_HOME/opera/NativeMessagingHosts"
    ;;
  edge)
    register_browser "$CONFIG_HOME/microsoft-edge/NativeMessagingHosts"
    ;;
  both)
    register_browser "$CONFIG_HOME/google-chrome/NativeMessagingHosts"
    register_browser "$CONFIG_HOME/chromium/NativeMessagingHosts"
    ;;
  all)
    register_chrome_family
    ;;
  *) usage ;;
esac

echo "Installed native messaging host."
echo "  Host: $INSTALL_DIR/grimledger_host"
echo "  Manifest: $MANIFEST_OUT"
echo "  Extension origin: $ORIGIN"
