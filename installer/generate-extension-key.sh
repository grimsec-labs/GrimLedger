#!/usr/bin/env bash
# Generate a stable Chrome extension public key and write installer/extension-id.txt.
# Run once from repo root; commit extension-id.txt and the manifest "key" field only.
# Private key is written to browser-extension/.extension-private.pem (gitignored).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRIV="$ROOT/browser-extension/.extension-private.pem"
PUB_DER="$(mktemp)"
trap 'rm -f "$PUB_DER"' EXIT

openssl genrsa -out "$PRIV" 2048
chmod 600 "$PRIV"
openssl rsa -in "$PRIV" -pubout -outform DER -out "$PUB_DER"

KEY_B64="$(openssl base64 -A -in "$PUB_DER")"
EXT_ID="$(python3 "$ROOT/installer/compute-extension-id.py" "$KEY_B64" 2>/dev/null || python "$ROOT/installer/compute-extension-id.py" "$KEY_B64")"

printf '%s\n' "$EXT_ID" > "$ROOT/installer/extension-id.txt"
printf '%s\n' "$KEY_B64" > "$ROOT/browser-extension/.extension-key.b64"

echo "Extension ID: $EXT_ID"
echo "Wrote installer/extension-id.txt"
echo "Wrote browser-extension/.extension-key.b64 (public key only)"
echo "Private key: $PRIV (gitignored — keep offline for CRX signing if needed)"
echo ""
echo "Add to browser-extension/manifest.json:"
echo "  \"key\": \"$KEY_B64\","
