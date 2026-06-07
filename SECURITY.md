# GrimLedger Security Notes

## Cryptography

- **KDF:** Argon2id via libsodium `crypto_pwhash` (moderate ops/mem limits, validated on load)
- **Legacy field encryption:** XSalsa20-Poly1305 via libsodium `crypto_secretbox`
- **Current field encryption:** XChaCha20-Poly1305 AEAD via `crypto_aead_xchacha20poly1305_ietf` with domain-bound associated data (note id, field name, attachment id, verifier purpose)
- **Standalone backups (GRIMBKUP2):** Password-derived key with header metadata in associated data
- **Library:** [libsodium](https://libsodium.gitbook.io/) — audited, widely deployed

No custom ciphers, no homegrown KDF, no ECB, no unauthenticated encryption.

## Data at Rest

- Note titles, bodies, and image attachments are encrypted before SQLite storage.
- Each ciphertext includes a unique nonce (prepended).
- Domain-bound AEAD prevents undetected ciphertext swapping between notes, fields, or purposes.
- Credential `allow_subdomains` fill policy is stored in authenticated `encrypted_fill_policy` blobs (AEAD with per-credential AAD).
- The SQLite file may contain encrypted BLOBs and unencrypted structural metadata (timestamps, folder names, tag names, favorite flags).
- `PRAGMA secure_delete = ON` is enabled to reduce recovery of deleted ciphertext from free pages (not guaranteed physical erasure).

## Data in Memory

- The derived master key is held in `VaultSession` while unlocked and zeroed with `sodium_memzero` on lock.
- Login password fields are cleared after successful unlock or vault creation.
- Decrypted notes exist in Qt widgets during editing. Full memory hygiene for all `QString` copies is not guaranteed.

## Authentication

- Unlock derives the key and verifies it against an encrypted sentinel value.
- Existing vaults without a verification token fail closed (no silent verifier creation).
- Wrong passwords produce a generic error — no oracle distinguishing failure modes.

## Backups

- **GRIMBKUP2** backups can restore on a clean machine using only the backup file and the master password used when creating the backup.
- **GRIMBKUP1** legacy backups require the same unlocked vault session key; restore validates before replacing the live vault.
- Restore never deletes the live vault until the replacement passes integrity and cryptographic checks.
- If restore succeeds but the `.bak` cleanup fails, GrimLedger still locks the session and forces re-login (`InstalledWithCleanupWarning`).

## Optional Self-Destruct

- Settings may enable destruction of the vault after three failed unlock attempts.
- This is an explicit, opt-in anti-coercion feature — not brute-force protection.
- Anyone with access to the login screen can trigger it. Back up your vault first.

## Operational Guidance

1. Use a long, unique master password (16+ characters recommended).
2. Enable auto-lock.
3. Lock manually when stepping away.
4. Create **GRIMBKUP2** backups and store them securely offline.
5. Treat `.md` exports as plaintext sensitive data.

## Non-Goals

GrimLedger does **not** protect against:

- Malware or keyloggers on the host system
- Physical access while the vault is unlocked
- Memory forensics / crash dumps
- Quantum adversaries (uses classical crypto)

## Recovery

**There is no password recovery mechanism.** If you lose your master password, your notes are permanently inaccessible. This is by design.
