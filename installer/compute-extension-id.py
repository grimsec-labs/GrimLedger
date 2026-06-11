#!/usr/bin/env python3
"""Compute Chrome extension ID from manifest 'key' (base64 DER SubjectPublicKeyInfo)."""
from __future__ import annotations

import base64
import hashlib
import sys
from pathlib import Path


def extension_id_from_public_key_der(pub_der: bytes) -> str:
    digest = hashlib.sha256(pub_der).digest()[:16]
    return "".join(chr(ord("a") + (byte >> 4)) + chr(ord("a") + (byte & 0xF)) for byte in digest)


def main() -> int:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <base64-public-key>", file=sys.stderr)
        return 1
    pub_der = base64.b64decode(sys.argv[1].strip())
    print(extension_id_from_public_key_der(pub_der))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
