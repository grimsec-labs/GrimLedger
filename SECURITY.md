# GrimLedger Security Notes

## Cryptography

- **KDF:** Argon2id via libsodium `crypto_pwhash` (moderate ops/mem limits)
- **Encryption:** XChaCha20-Poly1305 via libsodium `crypto_secretbox`
- **Library:** [libsodium](https://libsodium.gitbook.io/) — audited, widely deployed

No custom ciphers, no homegrown KDF, no ECB, no unauthenticated encryption.

## Data at Rest

- Note titles and bodies are encrypted before SQLite storage.
- Each ciphertext includes a unique nonce (prepended).
- The SQLite file may contain encrypted BLOBs and unencrypted structural metadata (timestamps, folder names, tag names, favorite flags).

## Data in Memory

- The derived master key is held in `VaultSession` while unlocked and zeroed with `sodium_memzero` on lock.
- Decrypted notes exist in Qt widgets during editing. Full memory hygiene for all `QString` copies is not guaranteed.

## Authentication

- Unlock derives the key and verifies it against an encrypted sentinel value.
- Wrong passwords produce a generic error — no oracle distinguishing failure modes.

## Operational Guidance

1. Use a long, unique master password (16+ characters recommended).
2. Enable auto-lock.
3. Lock manually when stepping away.
4. Treat `.md` exports as plaintext sensitive data.
5. Store `.grimbak` backups offline and encrypted-at-rest if possible (e.g., in an encrypted volume).

## Non-Goals

GrimLedger does **not** protect against:

- Malware or keyloggers on the host system
- Physical access while the vault is unlocked
- Memory forensics / crash dumps
- Quantum adversaries (uses classical crypto)

## Recovery

**There is no password recovery mechanism.** If you lose your master password, your notes are permanently inaccessible. This is by design.
