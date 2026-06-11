#!/usr/bin/env bash
# Package browser-extension/ as a ZIP for Chrome Web Store upload.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
EXT_DIR="$REPO_ROOT/browser-extension"
OUT_DIR="${OUT_DIR:-$REPO_ROOT/dist}"
MANIFEST="$EXT_DIR/manifest.json"

if [[ ! -f "$MANIFEST" ]]; then
  echo "manifest.json not found at $MANIFEST" >&2
  exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
  if command -v python >/dev/null 2>&1; then
    PYTHON=python
  else
    echo "Python is required to read manifest version." >&2
    exit 1
  fi
else
  PYTHON=python3
fi

VERSION="$("$PYTHON" -c "import json; print(json.load(open('$MANIFEST'))['version'])")"
ZIP_NAME="grimledger-bridge-${VERSION}.zip"
mkdir -p "$OUT_DIR"
ZIP_PATH="$OUT_DIR/$ZIP_NAME"

rm -f "$ZIP_PATH"
(
  cd "$EXT_DIR"
  zip -r "$ZIP_PATH" . \
    -x "*.md" \
    -x ".extension-*" \
    -x "STORE_LISTING.md" \
    -x "PRIVACY.md"
)

echo "Packaged extension for Chrome Web Store upload:"
echo "  $ZIP_PATH"
echo ""
echo "Extension ID (from installer/extension-id.txt):"
if [[ -f "$SCRIPT_DIR/extension-id.txt" ]]; then
  tr -d '[:space:]' < "$SCRIPT_DIR/extension-id.txt"
  echo ""
else
  echo "  (installer/extension-id.txt not found)"
fi
