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
- GrimLedger uses **field-level** encryption, not whole-file (SQLCipher) encryption. The SQLite file therefore contains encrypted BLOBs **plus unencrypted structural/metadata columns**, specifically: row timestamps (`created_at`/`updated_at`), folder names, tag names, favorite flags, and — for attachments — the original filename (`original_name`), `source_url`, `acquisition_note`, and `mime_type`. The security chronicle's `event_type` is also stored in cleartext (the event detail is encrypted). An attacker with the vault file at rest can read this metadata without the master password. Secret content (note titles/bodies, credential fields, attachment bytes, TOTP secrets) is always encrypted.
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

## Fast Unlock (optional, opt-in)

Fast Unlock trades some security for convenience and is off by default.

- **Windows — "Quick Unlock (Windows account)":** the vault key is wrapped with a secret
  protected by **user-scoped DPAPI** (`CryptProtectData`). This is **NOT** Windows Hello
  and requires **no biometric/PIN gesture**: any code running as the signed-in Windows
  user can unwrap it. Treat it as "anyone with your unlocked Windows account can open the
  vault without the master password." Keep your Windows account locked. (Earlier builds
  labelled this "Windows Hello", which overstated the protection.)
- **Android — biometric unlock:** the vault key is stored only in Keystore-backed
  `EncryptedSharedPreferences` whose master key requires recent device authentication
  (`setUserAuthenticationRequired`). The key is **never** written to plaintext preferences.
  Note that an enabled fast unlock stores a key equivalent to your master password, gated
  by the OS auth window rather than a fresh in-app biometric prompt for every unlock.
- In all cases the **master password remains the primary unlock** and fast unlock can be
  disabled in Settings, which clears the stored material.

## Optional Self-Destruct

- Settings may enable destruction of the vault after three failed unlock attempts.
- This is an explicit, opt-in anti-coercion feature — not brute-force protection.
- The failed-attempt counter is persisted, so it is not reset by restarting the app.
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
