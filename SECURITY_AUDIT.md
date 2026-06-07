# GrimLedger Security Audit

Audit date: 2026-06-07

Scope: Read-only review of the current GrimLedger working tree, including
cryptography, authentication, session locking, SQLite storage, password
rotation, backup and restore, Markdown rendering, file I/O, dependencies, and
the Windows release binary.

No application code was changed as part of the audit.

## Executive Summary

GrimLedger uses sound building blocks: Argon2id, authenticated encryption from
libsodium, prepared SQL statements for user-controlled values, and a local-only
architecture. However, several surrounding workflows can still cause permanent
data loss or weaken the guarantees expected from an encrypted note vault.

The most urgent problems are:

1. The optional failed-login self-destruct feature permits unauthenticated
   deletion of the vault.
2. Master-password rotation is not transactional and can leave the database
   encrypted under multiple incompatible keys.
3. Restore deletes the live vault before the replacement has been safely
   validated and installed.
4. Backups cannot independently recover a lost vault using only the backup and
   its password.

Do not rely on password rotation, restore, or self-destruct for important data
until the critical and high-severity findings are fixed.

## Findings

### GL-SEC-001: Failed-login self-destruct permits unauthenticated deletion

Severity: Critical

Locations:

- `src/App.cpp:121-170`
- `src/utils/AppSettings.cpp:7-33`
- `src/ui/SettingsWindow.cpp:75-87`

The optional feature deletes the vault after three incorrect passwords. An
attacker does not need to know the master password to trigger the deletion.
Anyone with access to the login screen can intentionally submit three bad
passwords and destroy the database.

The attempt counter is process-local and resets when the application restarts
or returns to the login screen. It therefore provides neither durable
rate-limiting nor meaningful brute-force protection. `QFile::remove()` is also
not secure erasure, and its return value is not checked before the application
claims that deletion succeeded.

Recommended remediation:

- Remove the failed-login self-destruct behavior.
- If a destructive reset feature is retained, require successful
  authentication, a separate explicit confirmation, and a recoverable backup.
- Use non-destructive throttling for failed logins. Because this is a local
  offline vault, document that an attacker with the database can attempt
  passwords outside the application.

Acceptance criteria:

- Incorrect passwords can never delete, truncate, replace, or rename the vault.
- No unauthenticated UI path can perform a destructive vault operation.
- Existing persisted self-destruct settings are ignored or migrated to `false`.

### GL-SEC-002: Master-password rotation is not atomic

Severity: High

Locations:

- `src/storage/VaultRepository.cpp:168-274`
- `src/ui/MainWindow.cpp:599-638`

Password rotation updates the verification token first, re-encrypts each note
one at a time, and updates the salt last. These operations are not protected by
a SQLite transaction.

A failed statement, application crash, power loss, disk-full condition, or
authentication failure on one damaged note can leave:

- the verifier encrypted with the new key;
- some notes encrypted with the old key;
- some notes encrypted with the new key; and
- metadata containing either the old or new salt.

The UI reports "Vault unchanged" when the operation returns false, even though
earlier writes may already have committed.

Recommended remediation:

- Validate and decrypt every encrypted record before making persistent changes.
- Run all verifier, note, and metadata updates in one `BEGIN IMMEDIATE`
  transaction.
- Roll back on every failure path and commit only after all writes succeed.
- Update the in-memory session key only after a successful commit.
- Zero temporary plaintext and key buffers on success and failure.

Acceptance criteria:

- Injected failure at any write step leaves the original password and all notes
  fully usable.
- A successful change makes the old password fail and the new password unlock
  every note.
- The UI never claims the vault is unchanged unless rollback succeeded.

### GL-SEC-003: Restore can destroy the live vault

Severity: High

Location:

- `src/ui/MainWindow.cpp:651-695`

Restore decrypts the selected backup, closes the database, removes the live
vault, writes the replacement directly, and only then attempts to open it.
Return values from `remove()` and `write()` are not checked. The decrypted file
is not validated as a complete and expected GrimLedger database before
installation.

Disk-full conditions, short writes, malformed SQLite data, an unexpected
schema, or a process crash can destroy the original vault and leave an unusable
replacement.

Recommended remediation:

- Enforce a maximum backup size before reading or decrypting.
- Write decrypted data to a temporary file in the vault directory.
- Check every write and flush result.
- Open the temporary database separately and run `PRAGMA quick_check` or
  `integrity_check`.
- Validate the required schema, vault version, KDF metadata, and verification
  token.
- Preserve the old vault until the replacement is fully validated.
- Install the replacement atomically and restore the original if installation
  fails.

Acceptance criteria:

- Corrupt, truncated, oversized, wrong-password, or wrong-schema backups never
  alter the live vault.
- Simulated short writes and failed replacement leave the original vault
  usable.
- Restore succeeds only after database and cryptographic validation.

### GL-SEC-004: Backups are not independent recovery artifacts

Severity: High

Locations:

- `src/storage/VaultRepository.cpp:277-311`
- `src/ui/MainWindow.cpp:641-695`

The entire database is encrypted with the current in-memory vault key. Restore
also requires that same current session key. The salt and KDF parameters needed
to derive the key are inside the encrypted database, so a user who has lost or
corrupted the live vault cannot derive the backup key from the password and
backup alone.

An older backup can also become awkward or impossible to restore after password
rotation because it was encrypted under the key active when that backup was
created.

Recommended remediation:

- Define a versioned backup envelope.
- Store only non-secret derivation metadata in its header: format version,
  random salt, bounded KDF parameters, nonce, and lengths.
- Authenticate the complete header as associated data.
- Require the backup password during restore and derive the backup key from the
  envelope metadata.
- Consider asking for and verifying the master password when creating a backup,
  or use a separate backup password.
- Preserve backward compatibility through an explicit legacy import path, or
  clearly reject legacy backups without touching the live vault.

Acceptance criteria:

- A backup plus its password can restore onto a clean machine with no existing
  GrimLedger vault.
- Header tampering, wrong passwords, and ciphertext tampering are rejected.
- The backup format is versioned and has documented size and KDF bounds.

### GL-SEC-005: Password plaintext remains in application memory

Severity: Medium

Locations:

- `src/App.cpp:57-81`
- `src/ui/LoginWindow.cpp:164-166`
- `src/security/CryptoManager.cpp:20-44`

After successful unlock, the login window is hidden but its password fields are
not cleared. The plaintext password can therefore remain in the hidden widget
for the unlocked session. `deriveKey()` also creates a UTF-8 `QByteArray` copy
that is not explicitly wiped.

Qt strings may still create unavoidable copies, so complete memory erasure
cannot be guaranteed. The application should nevertheless minimize retention.

Recommended remediation:

- Clear password and confirmation widgets immediately after successful key
  derivation.
- Refactor key derivation to wipe temporary UTF-8 buffers before returning.
- Avoid unnecessary `QString` and `QByteArray` copies of passwords and keys.
- Consider guarded or locked libsodium memory for the session key.

Acceptance criteria:

- Successful unlock and vault creation clear all password input widgets.
- Temporary byte representations of passwords are wiped on all return paths.

### GL-SEC-006: Ciphertext is not bound to record, field, or purpose

Severity: Medium

Locations:

- `src/security/CryptoManager.cpp:47-99`
- `src/storage/NoteRepository.cpp:15-23`
- `src/storage/VaultRepository.cpp:123-152`

The same vault key and ciphertext format are used for note titles, note bodies,
the verification token, and whole-database backups. `crypto_secretbox` provides
confidentiality and integrity for each ciphertext, but has no associated-data
parameter. A valid ciphertext can be copied between rows or purposes without
authentication failure.

This permits undetected record swapping and replay by an attacker who can
modify the database. It also complicates safe format evolution.

Recommended remediation:

- Introduce a versioned encrypted-field envelope.
- Use `crypto_aead_xchacha20poly1305_ietf_*` and authenticate domain context,
  such as format version, object type, record identifier, and field name.
- Derive separate domain keys for notes, verification, and backups where
  practical.
- Add a tested migration path for existing `crypto_secretbox` ciphertext.

Acceptance criteria:

- Moving a title ciphertext to a body, another note, verifier, or backup fails
  authentication.
- Existing vaults migrate without data loss and interrupted migration is
  recoverable.

### GL-SEC-007: KDF and vault metadata are not safely validated

Severity: Medium

Locations:

- `src/storage/VaultRepository.cpp:34-61`
- `src/storage/VaultRepository.cpp:112-165`
- `src/security/CryptoManager.cpp:20-44`

KDF operation and memory limits are loaded from the writable SQLite database
and passed to Argon2id without application-defined minimum and maximum bounds.
Corrupt or hostile metadata can cause unlock failures or excessive resource
consumption.

If the verification token is missing, unlock treats this as first use and
creates a new token with the entered password. Existing vault metadata should
not silently enter a first-unlock state.

Recommended remediation:

- Validate vault version, salt length, KDF algorithm, operation limit, and
  memory limit before deriving a key.
- Enforce conservative application-defined bounds.
- Create vault metadata and the verification token atomically during vault
  creation.
- Fail closed if an established vault is missing its verifier.

Acceptance criteria:

- Malformed KDF metadata is rejected before expensive allocation.
- Deleting or corrupting the verifier never causes a new verifier to be
  silently created for an existing vault.

### GL-SEC-008: Deleted ciphertext can remain in SQLite free pages

Severity: Medium

Locations:

- `src/storage/Database.cpp:19-29`
- `src/storage/NoteRepository.cpp:218-227`

SQLite normally leaves deleted or replaced content in free pages until it is
reused or vacuumed. GrimLedger does not enable `secure_delete`. This mainly
exposes old ciphertext rather than plaintext, but it can matter if the relevant
key is later recovered and it conflicts with a user's expectation of permanent
note deletion.

Recommended remediation:

- Enable and verify `PRAGMA secure_delete = ON` for the vault connection.
- Document the limits of logical deletion, SSD wear-leveling, filesystem
  snapshots, backups, and secure erasure.
- Consider an explicit compact/vacuum operation where operationally safe.

Acceptance criteria:

- The application verifies that `secure_delete` is enabled.
- Documentation does not claim guaranteed physical erasure.

### GL-SEC-009: Markdown links use unrestricted OS URL handlers

Severity: Medium

Locations:

- `src/ui/MarkdownPreview.cpp:12-19`
- `src/markdown/MarkdownRenderer.cpp:45-54`

`QTextBrowser::setOpenExternalLinks(true)` sends external links to
`QDesktopServices`. Imported or attacker-supplied Markdown can therefore offer
`file:`, custom protocol, and other OS-handler links. Exploitation still
requires the user to activate the link, but custom handlers can have unsafe
behavior.

Recommended remediation:

- Disable automatic external-link opening.
- Handle `anchorClicked` explicitly.
- Allow only a small scheme list, normally `https` and optionally `http` and
  `mailto`.
- Require confirmation that displays the destination before leaving the app.
- Reject local file, executable, and custom-protocol links by default.

Acceptance criteria:

- `file:`, `javascript:`, `data:`, and custom schemes never reach an OS handler.
- Approved web links require deliberate user action.

### GL-SEC-010: Import, backup, and restore sizes are unbounded

Severity: Medium

Locations:

- `src/storage/NoteRepository.cpp:370-382`
- `src/storage/VaultRepository.cpp:277-305`
- `src/ui/MainWindow.cpp:651-666`

These paths use `readAll()` without file-size limits. Large selected files can
consume excessive memory or crash the application. The encryption and SQLite
binding code also performs some narrowing conversions from Qt's size type to
32-bit integers.

Recommended remediation:

- Define explicit maximum note, database, and backup sizes.
- Check file metadata before reading.
- Use checked arithmetic and reject values that exceed API limits.
- Prefer streaming encryption for large backup files.
- Check every read and write result.

Acceptance criteria:

- Oversized files are rejected with a clear error before large allocation.
- No unchecked narrowing is used for ciphertext or SQLite blob sizes.

### GL-SEC-011: Dependencies are old and downloads lack integrity hashes

Severity: Low

Locations:

- `CMakeLists.txt:14-18`
- `CMakeLists.txt:49-53`

The build fetches libsodium and SQLite over HTTPS but does not pin the SQLite
archive with `URL_HASH`. Git tags provide weaker reproducibility than a pinned
commit hash. At audit time, the project used libsodium 1.0.20 and SQLite 3.46.1,
while newer maintained releases existed.

Recommended remediation:

- Update to supported dependency releases after reviewing compatibility.
- Pin source commits or release archives and verify cryptographic hashes.
- Add a documented dependency-update process and automated vulnerability
  monitoring.

Acceptance criteria:

- A clean build fetches content with immutable identifiers and verified hashes.
- Dependency versions are documented and reproducible.

### GL-SEC-012: Security documentation names the wrong encryption algorithm

Severity: Low

Locations:

- `README.md:27-42`
- `SECURITY.md:3-9`
- `src/security/CryptoManager.h:8-9`

The implementation calls `crypto_secretbox_easy()`, which uses
XSalsa20-Poly1305. Documentation describes it as XChaCha20-Poly1305. Both are
modern authenticated constructions, but security documentation must match the
actual code.

Recommended remediation:

- Correct current documentation to XSalsa20-Poly1305.
- If GL-SEC-006 migrates fields to XChaCha20-Poly1305 AEAD, document the format
  version and legacy compatibility accurately.

## Positive Observations

- Argon2id is used for password-based key derivation.
- Nonces and salts use libsodium's cryptographic random generator.
- Note title and body ciphertexts are authenticated.
- User-controlled SQL values generally use prepared statements.
- The current auto-lock flow uses `lockRequested` and a re-entry guard; the
  lock-recursion bug observed in an earlier working-tree snapshot is no longer
  present.
- The inspected Windows binary enabled ASLR, high-entropy ASLR, and DEP.

## Verification Performed

- Manual review of all security, storage, authentication, backup, restore,
  Markdown, import/export, and relevant UI code.
- Pattern scan for dangerous process execution, logging, SQL construction,
  external URL opening, file replacement, and unsafe C APIs.
- GCC 13 `-fanalyzer` plus warning checks on security-sensitive translation
  units.
- Inspection of release executable mitigation metadata.
- No project test suite was present during the audit.

## Recommended Remediation Order

1. Remove unauthenticated self-destruct.
2. Make password rotation transactional.
3. Replace restore with validate-then-atomically-install behavior.
4. Create a standalone, password-restorable, versioned backup format.
5. Add metadata validation and create the verifier atomically.
6. Minimize password and key retention.
7. Add ciphertext domain separation and migration.
8. Restrict Markdown URL schemes and bound all file sizes.
9. Enable SQLite hardening and update pinned dependencies.
10. Add automated security regression tests and correct documentation.

## Cursor Remediation Prompt

Paste the following prompt into Cursor from the GrimLedger repository root:

```text
You are performing a security remediation of the GrimLedger Qt 6 / C++20
encrypted note manager. Read SECURITY_AUDIT.md completely, then inspect the
current working tree before editing. The tree may contain uncommitted user
changes: preserve them and do not revert unrelated work.

Implement the findings in SECURITY_AUDIT.md, prioritizing data-loss prevention
and compatibility. Do not merely silence warnings or change error messages.
Fix the underlying security properties.

Required work:

1. Remove the unauthenticated failed-login self-destruct behavior.
   - Wrong passwords must never delete, replace, truncate, or rename a vault.
   - Remove or disable the setting and migrate any persisted value to false.
   - Do not replace it with misleading client-side brute-force protection.

2. Make master-password rotation fully atomic.
   - Validate/decrypt all encrypted records before persistent mutation.
   - Use BEGIN IMMEDIATE and COMMIT/ROLLBACK with RAII rollback protection.
   - Update notes, verifier, salt, and metadata in one transaction.
   - Check every prepare, bind, step, and commit result.
   - Wipe temporary plaintext and key buffers on every path.
   - Update VaultSession only after commit succeeds.
   - Never claim "Vault unchanged" unless rollback succeeded.

3. Redesign restore as validate-then-install.
   - Enforce strict file-size limits.
   - Never remove the live vault before the replacement is validated.
   - Write decrypted content to a temporary file in the same directory.
   - Check complete writes and flushes.
   - Open the temporary SQLite database separately and run quick_check or
     integrity_check.
   - Validate expected tables, vault version, bounded KDF metadata, and the
     cryptographic verifier.
   - Atomically replace the live vault while retaining a rollback copy until
     the new database opens successfully.
   - On any failure, leave the original vault byte-for-byte usable.

4. Introduce a standalone versioned backup format.
   - A backup plus its password must restore on a clean machine without an
     existing vault.
   - Define a binary envelope with magic, version, bounded KDF parameters,
     random salt, nonce, lengths, and ciphertext.
   - Authenticate the header as associated data using
     crypto_aead_xchacha20poly1305_ietf.
   - Ask for and verify the relevant password when creating/restoring backups,
     or introduce a separate backup password.
   - Reject wrong passwords, altered headers, altered ciphertext, oversized
     values, and unsupported versions before touching the live vault.
   - Provide an explicit safe behavior for legacy GRIMBKUP1 files.

5. Harden vault creation and metadata loading.
   - Create metadata and the verifier in one transaction during createVault.
   - Existing vaults with a missing verifier must fail closed.
   - Validate vault version, salt size, KDF algorithm, ops limit, and memory
     limit against conservative application-defined bounds before Argon2id.
   - Avoid optional dereference of loadVaultInfo() without checking it.

6. Minimize secret retention.
   - Clear login and confirmation fields immediately after successful unlock
     and creation.
   - Wipe temporary password UTF-8 buffers and keys on all return paths.
   - Avoid unnecessary copies of passwords and keys.
   - Use libsodium guarded/locked memory for the long-lived session key if it
     can be integrated cleanly without unsafe ownership.

7. Add cryptographic domain binding with backward compatibility.
   - Create a versioned encrypted-field format using
     crypto_aead_xchacha20poly1305_ietf.
   - Authenticate context including format version, object type, record
     identity, and field name.
   - Separate note, verifier, and backup domains, preferably with derived
     subkeys.
   - Implement a transactional, interruption-safe migration from existing
     crypto_secretbox fields.
   - Swapping ciphertext between notes, fields, or purposes must fail.

8. Harden SQLite and content handling.
   - Enable and verify PRAGMA foreign_keys=ON, secure_delete=ON, and
     trusted_schema=OFF where compatible.
   - Handle PRAGMA failures instead of ignoring them.
   - Disable automatic external link opening in MarkdownPreview.
   - Handle anchorClicked and allow only https plus explicitly justified
     schemes. Reject file, javascript, data, and custom schemes.
   - Add maximum sizes for notes, databases, and backups.
   - Replace unchecked readAll usage where it can allocate unbounded memory.
   - Check all file read/write results and all size conversions.

9. Pin and update dependencies.
   - Update libsodium and SQLite to supported releases compatible with the
     project.
   - Pin immutable source identifiers.
   - Add URL_HASH for downloaded archives.
   - Correct README.md, SECURITY.md, and CryptoManager comments: the legacy
     crypto_secretbox format is XSalsa20-Poly1305, not XChaCha20-Poly1305.

10. Add focused automated tests.
    - Use temporary directories and databases; never touch the user's vault.
    - Test password rotation success and injected failure at every write stage.
    - Test restore with truncation, corruption, wrong password, oversized
      lengths, invalid schema, failed writes, and failed replacement.
    - Test clean-machine backup restore.
    - Test malformed KDF metadata and missing verifier behavior.
    - Test ciphertext swapping and tampering.
    - Test URL scheme rejection and file-size limits.
    - Test lock and auto-lock exactly once without recursion.

Engineering constraints:

- Preserve current Qt/C++ style and existing ownership patterns unless a change
  is necessary for safety.
- Use RAII for SQLite statements, transactions, temporary files, and secret
  buffers where practical.
- Do not add custom cryptographic primitives.
- Do not log passwords, keys, plaintext notes, or decrypted backups.
- Do not weaken Argon2id parameters to make tests faster in production code;
  inject test parameters through a test-only seam.
- Keep migrations versioned, transactional, and recoverable.
- Do not report success unless every durable operation has succeeded.

Before finishing:

- Build the Release configuration.
- Run all new and existing tests.
- Run the project's available static analysis.
- Review the final diff for accidental plaintext logging, unchecked I/O,
  missing rollback paths, and unrelated changes.
- Update SECURITY_AUDIT.md with a remediation status table containing the
  commit/diff location and test covering each GL-SEC item.
- Summarize any finding that could not be safely fixed and explain the exact
  remaining risk.
```

## Remediation Status (2026-06-07)

Self-destruct (**GL-SEC-001**) was **retained** per product requirement: optional, opt-in anti-coercion when a device may be seized. Documented in `SECURITY.md` and Settings. Remaining findings were addressed in the working tree below.

| ID | Status | Primary locations | Notes |
|----|--------|-------------------|-------|
| GL-SEC-001 | **Accepted risk (retained)** | `src/App.cpp`, `src/utils/AppSettings.cpp`, `src/ui/SettingsWindow.cpp` | Optional toggle; documented trade-off. Not brute-force protection. |
| GL-SEC-002 | **Fixed** | `src/storage/VaultRepository.cpp`, `src/storage/DbTransaction.*`, `src/ui/MainWindow.cpp` | Validate-then-transactional rotation; session key updated only after commit. |
| GL-SEC-003 | **Fixed** | `src/ui/MainWindow.cpp`, `src/storage/VaultRepository.cpp` | Validate temp DB, integrity check, atomic install; live vault preserved on failure. |
| GL-SEC-004 | **Fixed** | `src/storage/VaultRepository.cpp`, `src/ui/MainWindow.cpp` | GRIMBKUP2 standalone envelope; GRIMBKUP1 legacy path preserved. |
| GL-SEC-005 | **Fixed** | `src/App.cpp`, `src/ui/LoginWindow.cpp`, `src/security/CryptoManager.cpp` | Password fields cleared after unlock/create; UTF-8 buffers wiped in `deriveKey`. |
| GL-SEC-006 | **Fixed** | `src/security/CryptoManager.*`, `src/storage/NoteRepository.cpp`, `src/storage/AttachmentRepository.cpp`, `VaultRepository::migrateDomainBoundCrypto` | XChaCha20 AEAD v2 + legacy secretbox migration on unlock. |
| GL-SEC-007 | **Fixed** | `src/storage/VaultRepository.cpp`, `src/security/CryptoManager.cpp` | KDF bounds validation; atomic vault create; fail-closed missing verifier. |
| GL-SEC-008 | **Fixed** | `src/storage/Database.cpp`, `SECURITY.md` | `PRAGMA secure_delete=ON`; limits documented. |
| GL-SEC-009 | **Fixed** | `src/ui/MarkdownPreview.cpp` | External links disabled; only `http`/`https`/`mailto` on explicit click. |
| GL-SEC-010 | **Fixed** | `src/utils/SecurityLimits.h`, `NoteRepository.cpp`, `VaultRepository.cpp`, `MainWindow.cpp` | Size limits before `readAll`; checked conversions. |
| GL-SEC-011 | **Partial** | `CMakeLists.txt` | SQLite archive pinned with `URL_HASH`; libsodium still uses `GIT_TAG` (not commit hash). |
| GL-SEC-012 | **Fixed** | `README.md`, `SECURITY.md`, `src/security/CryptoManager.h` | Legacy vs current algorithms documented accurately. |
| Automated tests | **Not implemented** | — | No test harness in repo; manual build verification only. |

### Remaining risk

- **GL-SEC-001:** Unauthenticated vault deletion remains possible when self-destruct is enabled (by design).
- **GL-SEC-005:** Qt `QString` may retain password copies; full memory erasure is not guaranteed.
- **GL-SEC-008:** `secure_delete` does not guarantee physical erasure on SSDs or with filesystem snapshots.
- **GL-SEC-011:** Reproducible builds depend on git-fetched libsodium tag; consider pinning a commit hash.
- **Tests:** Password-rotation failure injection, restore corruption, and ciphertext-swap regression tests are not yet automated.

## External References

- Libsodium secretbox:
  https://doc.libsodium.org/secret-key_cryptography/secretbox
- Libsodium XChaCha20-Poly1305 AEAD:
  https://doc.libsodium.org/secret-key_cryptography/aead/chacha20-poly1305
- Libsodium secure memory:
  https://doc.libsodium.org/memory_management
- Qt `QSaveFile`:
  https://doc.qt.io/qt-6/qsavefile.html
- Qt `QTextBrowser` external links:
  https://doc.qt.io/qt-6/qtextbrowser.html
- SQLite secure deletion:
  https://sqlite.org/pragma.html#pragma_secure_delete
- SQLite vulnerability guidance:
  https://sqlite.org/cves.html
