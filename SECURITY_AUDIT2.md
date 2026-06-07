# GrimLedger Follow-up Security Audit

**Audit date:** 2026-06-07  
**Scope:** Cryptography, KDF and metadata handling, SQLite storage, password rotation, legacy migration, backup and restore, attachments and image sanitization, Markdown rendering, locking, destructive operations, imports/exports, dependencies, documentation, and tests.  
**Method:** Read-only follow-up review of the current working tree. No application code, configuration, documentation, dependency, or resource file was changed. This report is the only file created.

## Executive Summary

GrimLedger's security posture is **improved but mixed** compared with the first audit. The remediation added sound primitives and several useful guardrails: domain-bound XChaCha20-Poly1305 AEAD, KDF bounds, fail-closed verifier handling, versioned backup envelopes, restore staging, SQLite hardening, URL scheme restrictions, file-size limits, and more accurate algorithm documentation.

The remaining weaknesses are primarily in workflow state management and failure handling rather than in the cryptographic algorithms themselves. Several operations look atomic in the normal path but can still leave data inaccessible or overwrite the wrong state after an error.

Top remaining risks:

- A successful GRIMBKUP2 restore leaves the old session key and old editor state active, allowing subsequent saves to corrupt the restored vault.
- Backup creation does not verify that the entered password is the vault's current master password, so a backup can report success yet be impossible to restore.
- Password rotation and legacy migration do not reject all partial-query failures and do not verify that `BEGIN IMMEDIATE` started.
- Destructive reset and self-destruct ignore `QFile::remove()` failure and can falsely claim deletion or rekey metadata over old encrypted notes.
- There is no automated security regression suite for rotation, migration, backup, restore, AEAD binding, attachments, or destructive failure paths.

**Reliability conclusion:**

- **Password rotation:** Not yet safe to rely on for the only copy of important data. It is transactional in the common path, but partial record scans and transaction-start handling remain unsafe.
- **Restore:** Not safe to use in the current unlocked session until the session key/editor-state bug is fixed. The staged-file design is an improvement, but validation and rollback are incomplete.
- **Backups:** GRIMBKUP2 has a sound envelope design, but creation is not reliably recoverable unless the user enters the exact current vault password, chooses a destination other than the live vault, and independently tests the backup.

## Remediation Verification Table

| ID | Original severity | Claimed status (audit 1 table) | Verified status | Evidence | Gaps / notes |
|---|---|---|---|---|---|
| GL-SEC-001 | Critical | Accepted risk (retained) | **Accepted risk** | `src/App.cpp:127-185`; `src/utils/AppSettings.cpp:11-33`; `src/ui/SettingsWindow.cpp:75-87` | The opt-in unauthenticated deletion trigger remains as an explicit product decision and is documented. Its deletion error path is unsafe; see GL-SEC-104. This audit does not recommend removing the accepted feature. |
| GL-SEC-002 | High | Fixed | **Partially fixed** | `src/storage/VaultRepository.cpp:535-723`; `src/storage/DbTransaction.cpp:6-29`; `src/ui/MainWindow.cpp:622-661` | Records are prepared before writes and writes use `BEGIN IMMEDIATE`, but query prepare/step completion is not consistently checked, reads occur before the transaction snapshot, and callers cannot verify that the transaction began. Attachments can be skipped and left under the old key. See GL-SEC-103. |
| GL-SEC-003 | High | Fixed | **Partially fixed** | `src/storage/VaultRepository.cpp:799-815, 817-1003, 1098-1145`; `src/ui/MainWindow.cpp:684-756` | Restore now stages and checks a temporary DB before install. Required schema is not fully validated, rollback rename results are ignored, and the UI retains the pre-restore key/editor state after a successful restore. See GL-SEC-101 and GL-SEC-107. |
| GL-SEC-004 | High | Fixed | **Partially fixed** | `src/storage/VaultRepository.cpp:125-160, 725-797, 817-1003`; `src/ui/MainWindow.cpp:664-756` | GRIMBKUP2 is versioned and authenticates its header; GRIMBKUP1 has a separate compatibility path. Export does not verify the entered password against the current vault, so the envelope password may differ from the inner vault password and make restore impossible. See GL-SEC-102. |
| GL-SEC-005 | Medium | Fixed | **Verified fixed** | `src/security/CryptoManager.cpp:36-64`; `src/ui/LoginWindow.cpp:163-166`; `src/App.cpp:72-84, 105-120` | The original acceptance criteria are met: login widgets are cleared after successful unlock/create and the UTF-8 derivation buffer is wiped. Broader Qt implicit-sharing and session-key copy risks remain, as already acknowledged in the documentation. |
| GL-SEC-006 | Medium | Fixed | **Partially fixed** | `src/security/CryptoManager.cpp:144-167, 224-241`; `src/storage/NoteRepository.cpp:16-40`; `src/storage/VaultRepository.cpp:382-533` | New note fields, verifier values, attachments, and V2 backups use domain-specific AEAD. Migration can silently accept failed/partial table scans, ignores the format-marker write result, and attachment duplication breaks AAD identity. See GL-SEC-103 and GL-SEC-106. |
| GL-SEC-007 | Medium | Fixed | **Verified fixed** | `src/security/CryptoManager.cpp:14-27, 36-64`; `src/storage/VaultRepository.cpp:187-220, 301-379` | Vault version, salt, and KDF bounds are checked before derivation; established vaults without a verifier fail closed; creation places metadata and verifier writes in one transaction. The transaction helper still needs stronger start-state reporting as defense in depth. |
| GL-SEC-008 | Medium | Fixed | **Partially fixed** | `src/storage/Database.cpp:19-40`; `SECURITY.md:13-19` | `secure_delete`, `foreign_keys`, and `trusted_schema` are set, and physical-erasure limits are documented. The code checks only whether the PRAGMA statement executed, not the returned effective value, so the original "verifies that secure_delete is enabled" criterion is not fully met. |
| GL-SEC-009 | Medium | Fixed | **Verified fixed** | `src/ui/MarkdownPreview.cpp:13-23`; `src/markdown/MarkdownRenderer.cpp:72-94` | Automatic external opening is disabled and clicks are allowlisted to `http`, `https`, and `mailto`; local/custom schemes are ignored. There is no destination confirmation, and allowed links are sent to `QTextBrowser::setSource()` rather than an explicit external browser, but the original acceptance criteria are met. |
| GL-SEC-010 | Medium | Fixed | **Partially fixed** | `src/utils/SecurityLimits.h:5-9`; `src/storage/NoteRepository.cpp:395-410`; `src/storage/VaultRepository.cpp:725-797, 817-1096`; `src/security/CryptoManager.cpp:80-140, 176-221` | Import/backup/restore paths now reject oversized files before `readAll()`. Unchecked `qsizetype`/cipher-length to `int` conversions and `sqlite3_bind_blob()` length narrowing remain, and aggregate attachment growth can make the live DB exceed the backup limit. |
| GL-SEC-011 | Low | Partial | **Partially fixed** | `CMakeLists.txt:12-18, 48-57` | SQLite has an exact `URL_HASH`. Libsodium is still fetched using a mutable Git tag rather than an immutable commit or hashed release archive. No dependency vulnerability monitoring is configured. |
| GL-SEC-012 | Low | Fixed | **Verified fixed** | `README.md:25-45`; `SECURITY.md:3-11`; `src/security/CryptoManager.h:8-10` | Current XChaCha20-Poly1305 AEAD and legacy XSalsa20-Poly1305 are named accurately. The README's later backup table still describes older session-key semantics (`README.md:152-160`) and should be reconciled. |

## Feature Implementation Checklist

| Remediation feature | Status | Evidence / notes |
|---|---|---|
| `[ ]` Atomic master-password rotation (`BEGIN IMMEDIATE`, validate all first) | **Partial** | `VaultRepository::changeMasterPassword`, `src/storage/VaultRepository.cpp:535-723`. Normal writes are transactional, but record scans can end on an error without detection, attachment prepare failure is ignored, reads are outside the transaction snapshot, and transaction start cannot be queried. |
| `[ ]` Validate-then-install restore | **Partial** | `src/storage/VaultRepository.cpp:799-815, 913-1003, 1098-1145`. Temp file and `quick_check` exist; full schema validation, checked rollback, and UI key transition are missing. |
| `[ ]` GRIMBKUP2 standalone envelope | **Partial** | `src/storage/VaultRepository.cpp:125-160, 725-768, 817-1003`. Header/KDF metadata and AAD are implemented, but export accepts an unverified password and can produce a backup that fails inner-vault verification. |
| `[x]` GRIMBKUP1 legacy compatibility | **Implemented** | `src/storage/VaultRepository.cpp:770-797, 1005-1096`; `src/ui/MainWindow.cpp:722-738`. Separate session-key restore path is present. |
| `[x]` KDF/metadata validation and fail-closed missing verifier | **Implemented** | `src/security/CryptoManager.cpp:14-27`; `src/storage/VaultRepository.cpp:187-220, 331-379`. |
| `[ ]` Domain-bound AEAD V2 plus legacy migration | **Partial** | `src/security/CryptoManager.cpp:144-167, 224-241`; `src/storage/VaultRepository.cpp:382-533`. Cryptography is implemented; migration completeness and attachment duplication are not safe. |
| `[ ]` Password/secret buffer minimization | **Partial** | Login fields and KDF UTF-8 buffers are cleared, but `MainWindow` and attachment preview create additional key/plaintext copies (`src/ui/MainWindow.cpp:629, 828-841`), and some optional plaintext buffers are not directly wiped after copied use (`src/storage/VaultRepository.cpp:604-653`). |
| `[ ]` SQLite hardening | **Partial** | `src/storage/Database.cpp:28-39`. PRAGMAs are set but not queried to verify effective values. |
| `[x]` Markdown URL scheme restrictions | **Implemented** | `src/ui/MarkdownPreview.cpp:13-23`. No automatic OS handler for unapproved schemes. |
| `[ ]` File-size limits | **Partial** | Limits exist in `src/utils/SecurityLimits.h`; integer narrowing, decoded-image memory, aggregate vault size, and live-vault backup refusal remain. |
| `[ ]` Dependency pinning | **Partial** | SQLite URL hash is pinned; libsodium remains tag-based. |
| `[ ]` Documentation accuracy | **Partial** | Algorithm descriptions are corrected, but backup behavior is internally inconsistent in `README.md:45, 63, 152-160`, and docs overstate restore/live-vault preservation. |
| `[ ]` Automated security regression tests | **Missing** | No test target, test source tree, `enable_testing()`, or `add_test()` registration was found. |

## New Findings

### GL-SEC-101: Successful restore keeps the old session key and editable pre-restore state

**Severity:** High  
**Locations:** `src/ui/MainWindow.cpp:684-756`; `src/ui/MainWindow.cpp:414-504`; `src/security/VaultSession.cpp:15-29`

**Description:** GRIMBKUP2 restore validates the restored database with a key derived from the entered password, but that key is discarded inside `restoreFromBackup()`. After installation, `MainWindow` reopens the database and reloads lists without replacing or locking `m_session`. The editor's current note ID and plaintext content are also not cleared.

**Risk:** If the restored backup uses a different key, or is an older backup from before password rotation, list decryption fails under the stale key. A subsequent save can overwrite restored notes with ciphertext under the stale key while the restored verifier and salt require the backup's key. This creates a mixed-key vault and permanent data loss.

**Recommended remediation:**

- Make restore return the verified restored vault key, or force a full lock and require a fresh unlock before any restored data is opened.
- Clear the current editor, cached notes, preview resolver, current IDs, and settings password fields before reopening.
- Disable save/edit actions during restore and do not call `loadNotes()` until the session key matches the restored verifier.
- For GRIMBKUP1, retain the current key only after explicitly verifying the restored verifier with it.

**Acceptance criteria:**

- Restoring a backup made before password rotation leaves no old key or old note content active.
- No save can occur between database replacement and authentication with the restored vault.
- Every restored note and attachment decrypts with the active session key before editing is enabled.

### GL-SEC-102: Backup creation can report success for an unrestorable password combination

**Severity:** High  
**Locations:** `src/ui/MainWindow.cpp:664-681`; `src/storage/VaultRepository.cpp:725-768, 959-999`; `SECURITY.md:33-37`

**Description:** The UI asks for the master password but passes any non-empty string to `exportEncryptedBackupV2()`. The outer backup is encrypted with that entered password. Restore then uses the same password both to decrypt the outer envelope and to derive the key for the inner database's original salt and verifier.

**Risk:** A typo or different password produces a valid, authenticated outer backup that is guaranteed to fail inner verifier validation. The application reports that the backup was saved successfully, creating false confidence in recovery.

**Recommended remediation:**

- Before exporting, derive and verify the entered password against the current vault metadata and verifier.
- Alternatively, explicitly support a distinct backup password by storing/recovering the inner vault key through a separately authenticated key-wrapping design; do not reuse one password for two unrelated checks implicitly.
- Add a post-write verification pass that reopens the finished backup, decrypts it, validates the schema, and verifies the inner sentinel before reporting success.
- Use SQLite's backup/snapshot API rather than copying a live DB file directly, especially if multiple app instances are possible.

**Acceptance criteria:**

- A wrong or mistyped current password is rejected before a backup is created.
- Every reported-success GRIMBKUP2 file restores on a clean profile using the password accepted at export.
- Automated tests cover correct password, wrong password, header tamper, payload tamper, and backup after password rotation.

### GL-SEC-103: Rotation and migration can commit after partial record scans

**Severity:** High  
**Locations:** `src/storage/VaultRepository.cpp:382-533, 535-723`; `src/storage/DbTransaction.h:5-18`; `src/storage/DbTransaction.cpp:6-29`

**Description:** Migration ignores note/attachment prepare failures by treating the corresponding collection as empty. Password rotation returns on note prepare failure, but silently ignores attachment prepare failure. Both use `while (sqlite3_step(...) == SQLITE_ROW)` without requiring the final result to be `SQLITE_DONE`. Reads and validation also happen before `BEGIN IMMEDIATE`, and `DbTransaction` exposes no `isActive()` result.

**Risk:** An I/O, schema, corruption, or concurrency error can leave records undiscovered. Rotation may then update the verifier, visible records, and salt while skipped attachments or notes remain under the old key. Migration may mark the vault V2 while legacy records remain. Those records become inaccessible or bypass the intended domain binding.

**Recommended remediation:**

- Start and verify `BEGIN IMMEDIATE` before the record scan so validation and writes use one stable snapshot.
- Fail on every prepare error and require each scan to terminate with `SQLITE_DONE`.
- Expose transaction start state and make all callers abort before any operation if begin failed.
- Check affected-row counts for every expected update and make `setCryptoFormatV2()` return a checked result.
- Wipe all plaintext buffers without creating extra implicit-sharing copies.

**Acceptance criteria:**

- Injected prepare, step, update, marker, commit, and disk-full failures leave the old password and all records usable.
- Concurrent app instances cannot add or change a record between validation and commit.
- The V2 marker is written only after every encrypted record was enumerated and successfully rewritten.

### GL-SEC-104: Destructive reset and self-destruct ignore deletion failure

**Severity:** High  
**Locations:** `src/App.cpp:87-120, 153-185`

**Description:** Both "Create New Vault" and failed-login self-destruct close the DB and call `QFile::remove(m_db->path())` without checking its return value. They then reopen the same path. If removal failed, reopen can succeed on the original database. New-vault creation can replace only metadata/verifier while retaining old encrypted notes; self-destruct can claim permanent deletion while the vault still exists.

**Risk:** The reset path can create a mixed-key database and destroy access to existing notes. The accepted anti-coercion feature can give a false assurance that sensitive data was deleted.

**Recommended remediation:**

- Preserve the opt-in self-destruct feature, but check deletion and confirm that the original file no longer exists before reopening or reporting success.
- Implement new-vault reset with a staged new database and checked atomic replacement, preserving a recoverable old file until the new vault is fully initialized and unlocked.
- Use accurate wording: file deletion is not guaranteed physical erasure.

**Acceptance criteria:**

- Simulated delete denial never changes metadata in the old vault and never reports successful destruction.
- Creating a new vault either installs a complete new database or leaves the old vault fully usable.
- The self-destruct setting remains explicit and documented as accepted risk.

### GL-SEC-105: Backup output is not prevented from targeting the live vault

**Severity:** High  
**Locations:** `src/ui/SettingsWindow.cpp:115-121, 142-149`; `src/ui/MainWindow.cpp:664-681`; `src/storage/VaultRepository.cpp:725-768`

**Description:** The selected backup/archive destination is not compared with the live database path. `QSaveFile` may atomically replace its target. On platforms that permit replacing an open SQLite file, choosing the live vault as the backup destination can replace the SQLite pathname with GRIMBKUP2 data.

**Risk:** The application may continue temporarily using the old open file handle, but the next open/restart sees an encrypted backup envelope instead of a SQLite database. This can make the live vault appear destroyed.

**Recommended remediation:**

- Compare canonical/absolute destination and live-vault paths, including case-insensitive and same-file checks where applicable.
- Reject the live path, restore temp path, rollback path, and any hard-link/same-file equivalent.
- Verify after commit that the live SQLite database still opens and that the output is a separate file.

**Acceptance criteria:**

- Backup/archive export cannot target or alias the live database or restore working files.
- Tests cover relative paths, case differences, symlinks/hard links where supported, and the exact live path.

### GL-SEC-106: Duplicating an image attachment invalidates its AEAD identity

**Severity:** Medium  
**Locations:** `src/storage/AttachmentRepository.cpp:110-174`; `src/security/CryptoManager.cpp:236-238`; `src/ui/MainWindow.cpp:119-142`

**Description:** Attachment ciphertext is authenticated with AAD containing its attachment UUID. Duplication assigns a new UUID but copies the ciphertext byte-for-byte. The copied ciphertext therefore authenticates only under the old UUID, while load, migration, and rotation use the new UUID.

**Risk:** Images in duplicated notes fail authentication and appear unavailable. The invalid duplicate can also cause later password rotation or migration to abort, reducing availability of security-critical operations.

**Recommended remediation:**

- Duplicate attachments by decrypting with the old UUID's AAD and re-encrypting with the new UUID's AAD.
- Include duplication and body URL remapping in one checked transaction.
- Pass the active key into the duplication API and wipe temporary plaintext image buffers.

**Acceptance criteria:**

- Every duplicated attachment loads under its new UUID.
- A note with duplicated images can subsequently migrate and rotate passwords successfully.
- Any duplication failure leaves no partial copied note, orphan attachment, or rewritten URL.

### GL-SEC-107: Restore validation and rollback do not fully protect the original vault

**Severity:** Medium  
**Locations:** `src/storage/VaultRepository.cpp:34-123, 799-815, 941-1003, 1086-1145`

**Description:** `validateVaultFile()` checks `quick_check` and one metadata row but does not validate all required tables, columns, foreign keys, or application metadata. The later verifier check covers only `app_metadata.verify`. During installation, rollback `rename()` calls and final backup deletion are unchecked.

**Risk:** An authenticated but structurally incomplete database can replace the live vault and then fail in normal repository operations. If installation or post-install validation fails and the rollback rename also fails, the live vault pathname can be absent even though restore reports failure.

**Recommended remediation:**

- Validate the complete expected schema using `sqlite_master`, `PRAGMA table_info`, foreign-key checks, required metadata, and supported schema/version values.
- Check every remove/rename result and retain the preserved original until the reopened restored database passes all checks.
- Use unique temporary paths and an explicit recovery state that can be repaired on next startup.

**Acceptance criteria:**

- Missing/renamed tables or columns, foreign-key errors, and unsupported versions never alter the live vault.
- Injected install and rollback failures always leave either the original live path or a clearly identified recoverable original file.
- Success is reported only after the installed database passes full schema, integrity, and cryptographic validation.

### GL-SEC-108: Backup header parsing performs unaligned typed reads

**Severity:** Medium  
**Location:** `src/storage/VaultRepository.cpp:144-160`

**Description:** The parser casts byte offsets 1, 19, 27, and 35 to `quint16*` or `quint64*` and dereferences them. These offsets are not naturally aligned for those integer types.

**Risk:** The code has undefined behavior in C++ and can fault on strict-alignment architectures when parsing a crafted or ordinary backup header. It also gives optimizers assumptions that are not valid for a byte buffer.

**Recommended remediation:**

- Use the pointer-taking Qt endian helpers on byte data, or copy bytes into aligned integer objects with `memcpy` before endian conversion.
- Add truncated, malformed, and alignment-sensitive parser tests.

**Acceptance criteria:**

- Header parsing performs no typed dereference of unaligned storage.
- Sanitizer/static-analysis builds report no alignment or object-lifetime violation.

### GL-SEC-109: Image and aggregate attachment limits permit memory exhaustion and disable backups

**Severity:** Medium  
**Locations:** `src/utils/ImageSanitizer.h:12-19`; `src/utils/ImageSanitizer.cpp:32-71, 76-117`; `src/storage/AttachmentRepository.cpp:27-65`; `src/storage/VaultRepository.cpp:725-735`

**Description:** An 8192 x 8192 RGBA image requires about 256 MiB for one raster, before scaling/format conversion/PNG output copies. Preview additionally base64-encodes decrypted images every 400 ms. Individual attachments are limited by the UI sanitizer, but the repository has no aggregate vault or per-note quota. Once the DB exceeds 200 MiB, backup creation refuses it.

**Risk:** A selected compressed image can cause large transient allocations or UI denial of service. Repeated valid attachments can grow a vault into a state where the documented backup feature no longer works.

**Recommended remediation:**

- Set a conservative maximum decoded pixel count and calculate allocation bounds with checked arithmetic before decode.
- Cache preview resources instead of decrypting and base64-encoding them every timer tick.
- Enforce per-attachment, per-note, and aggregate encrypted-vault quotas below the supported backup ceiling, or implement streaming backups that support the full allowed vault size.
- Enforce limits inside `AttachmentRepository`, not only in the UI.

**Acceptance criteria:**

- Images exceeding the decoded-pixel budget are rejected before a large raster allocation.
- Repeated preview refresh does not repeatedly decrypt/base64 the same unchanged image.
- A vault accepted by attachment storage remains backup-capable, or the backup implementation supports its documented maximum.

### GL-SEC-110: Plaintext export reports success despite write failures and filename collisions

**Severity:** Medium  
**Locations:** `src/storage/NoteRepository.cpp:413-440`; `src/storage/AttachmentRepository.cpp:189-227`; `src/ui/MainWindow.cpp:777-821`

**Description:** Export functions do not check `QFile::write()` or close/flush results. "Export all" derives filenames from titles, silently overwrites notes with duplicate sanitized names, ignores individual export failures, does not export image attachments through the attachment rewrite path, and always returns true. The UI always displays success.

**Risk:** Users can believe they have a complete plaintext recovery export when notes or images were omitted, truncated, or overwritten.

**Recommended remediation:**

- Use `QSaveFile`, check every write and commit, create collision-resistant filenames, and return a detailed success/failure summary.
- Route bulk export through the same attachment-aware export logic as single-note export.
- Do not display global success when any note or attachment failed.

**Acceptance criteria:**

- Disk-full, permission, short-write, duplicate-title, and attachment-write failures are surfaced.
- Exporting multiple notes with the same title preserves every note.
- A reported-complete export contains all selected note bodies and referenced images.

### GL-SEC-111: Authentication failures are silently converted into empty note fields

**Severity:** Medium  
**Locations:** `src/storage/NoteRepository.cpp:28-41, 99-130, 133-169`; `src/ui/MainWindow.cpp:351-368`

**Description:** `decryptNoteField()` returns an empty `QString` both for valid empty plaintext and for AEAD authentication failure. Repository methods still return a `Note`, and the UI can display and later save it as empty content.

**Risk:** Ciphertext tampering, wrong-key state, or corruption is hidden from the user. A later save can replace evidence of the failure with newly encrypted empty data, complicating recovery and integrity diagnosis.

**Recommended remediation:**

- Return an explicit success/error result for every encrypted field.
- Refuse editing/saving a note when any field fails authentication and surface a generic integrity error.
- Record only non-secret diagnostic context such as note ID and operation, never plaintext or keys.

**Acceptance criteria:**

- Empty plaintext is distinguishable from decryption failure.
- A note with a corrupted title or body cannot be silently overwritten through the normal editor.
- Tests cover wrong AAD, wrong key, truncated blob, swapped title/body, and valid empty fields.

## Positive Observations

- The application now distinguishes legacy XSalsa20-Poly1305 from current XChaCha20-Poly1305 AEAD accurately.
- Note title/body AAD includes note identity and field purpose; verifier and backup purposes are separate.
- KDF parameters and salt length are bounded before Argon2id is called.
- Missing verification metadata fails closed instead of creating a verifier during unlock.
- Restore uses a staged file, checks writes, runs SQLite `quick_check`, and performs cryptographic verifier validation before normal installation.
- GRIMBKUP2 authenticates its complete non-secret header as associated data.
- GRIMBKUP1 is recognized and routed through an explicit legacy session path.
- Login password widgets and the KDF UTF-8 buffer are cleared after successful use.
- SQLite enables `foreign_keys`, `secure_delete`, and `trusted_schema=OFF`.
- Markdown rendering escapes note-controlled HTML, blocks non-attachment image URLs, and allowlists clicked link schemes.
- Image imports are decoded and re-encoded as PNG, stripping original file metadata.
- No logging of passwords, keys, decrypted notes, or attachment plaintext was found in the reviewed source.
- SQLite's downloaded archive now has an exact SHA-256 hash.

## Verification Performed

- Read the original `SECURITY_AUDIT.md`, including findings GL-SEC-001 through GL-SEC-012, acceptance criteria, remediation prompt, and claimed status table.
- Read `SECURITY.md`, `README.md`, `CMakeLists.txt`, and all named security-sensitive implementation files.
- Traced vault creation, unlock, migration, password rotation, backup V1/V2, restore, install/rollback, self-destruct, and new-vault reset end to end.
- Traced note and attachment encryption identities, duplication, image insertion, preview resolution, and plaintext export.
- Scanned for SQL construction, SQLite prepare/step handling, unchecked file remove/rename/write/readAll calls, URL handlers, process execution, and secret logging.
- Scanned for test files and CMake/CTest registrations. No application test harness was found.
- Reviewed the working tree status before writing this report; it was clean and `SECURITY_AUDIT2.md` did not exist.
- No build, runtime restore, destructive fault-injection, or sanitizer run was performed because the audit's only permitted write action was this report and the repository contains no non-writing test target. No dependency/CVE network scan was performed.

## Comparison to Audit 1

Audit 1 correctly identified weak workflow safety around rotation, restore, backups, metadata, field binding, and input handling. The remediation materially improved the cryptographic format and added useful defensive infrastructure. GL-SEC-005, GL-SEC-007, GL-SEC-009, and GL-SEC-012 meet their original acceptance criteria. GL-SEC-001 remains an explicitly documented accepted risk.

The claimed fixes for GL-SEC-002, GL-SEC-003, GL-SEC-004, GL-SEC-006, GL-SEC-008, and GL-SEC-010 are incomplete. Most gaps arise from unchecked secondary failure paths, stale in-memory state, partial record enumeration, or documentation that describes the intended normal path as an unconditional guarantee.

The most important regression introduced by the new attachment/domain-binding work is that attachment duplication copies ciphertext to a new authenticated identity without re-encryption. The most important restore regression is the failure to transition the active session key after installing a different vault.

## Recommended Next Steps

1. Fix GL-SEC-101 first: restore must lock or install the verified restored key before any reload or save.
2. Fix GL-SEC-102 and GL-SEC-105: verify the export password, prevent live-path aliasing, and verify every completed backup before reporting success.
3. Fix GL-SEC-103: put scan and writes in one verified transaction, reject partial scans, and check marker/update row counts.
4. Fix GL-SEC-104 and GL-SEC-107: make destructive reset, self-destruct reporting, install, and rollback explicit and fully checked.
5. Fix GL-SEC-106 so attachment duplication decrypts and re-encrypts under the new UUID in one transaction.
6. Add automated failure-injection tests before treating rotation, migration, restore, or backups as reliable.
7. Add crypto regression tests for all AAD swap cases, legacy migration interruption, malformed headers, and valid empty plaintext.
8. Add decoded-image and aggregate-vault quotas, then make exports report partial failures accurately.
9. Pin libsodium immutably and add a documented dependency update/vulnerability review process.
10. Update `README.md` and `SECURITY.md` only after behavior and tests match the guarantees.

## Cursor Remediation Prompt

```text
You are fixing the security and data-loss findings in SECURITY_AUDIT2.md for
GrimLedger, a Qt 6 / C++20 local encrypted Markdown note manager.

Read these files first:
- SECURITY_AUDIT2.md
- SECURITY_AUDIT.md
- SECURITY.md
- README.md
- src/security/CryptoManager.*
- src/security/VaultSession.*
- src/storage/Database.*
- src/storage/DbTransaction.*
- src/storage/VaultRepository.*
- src/storage/NoteRepository.*
- src/storage/AttachmentRepository.*
- src/ui/MainWindow.*
- src/ui/MarkdownPreview.*
- src/ui/LoginWindow.*
- src/ui/SettingsWindow.*
- src/utils/SecurityLimits.h
- src/utils/ImageSanitizer.*
- CMakeLists.txt

Important product constraint:
- Preserve GL-SEC-001 as an explicit opt-in anti-coercion self-destruct feature.
  Do not remove it. Fix its error handling, truthful reporting, and tests.

Implement the fixes in this priority order:

1. Restore/session correctness (GL-SEC-101)
- During restore, disable editing and saving.
- On success, either return and install the key that verified the restored
  vault, or lock the app and require a fresh unlock before reopening content.
- Clear editor plaintext, preview resources, current note IDs, caches, and old
  session-key copies before using the restored database.
- Verify GRIMBKUP1's restored verifier with the retained session key.
- Never call loadNotes() with a key that has not verified the installed vault.

2. Backup reliability and path safety (GL-SEC-102, GL-SEC-105)
- Verify the entered export password against the current vault verifier before
  writing GRIMBKUP2.
- Reject any output path that is the same file as, aliases, or resolves to the
  live vault, restore temp file, or rollback file.
- Prefer SQLite's online backup API for a consistent database snapshot.
- Reopen and fully verify the completed backup before reporting success.
- Keep the V2 header authenticated and retain GRIMBKUP1 compatibility.

3. Rotation and migration atomicity (GL-SEC-002, GL-SEC-006, GL-SEC-103)
- Add DbTransaction::isActive() or a factory/result that makes failed BEGIN
  impossible to ignore.
- Start verified BEGIN IMMEDIATE before scanning records.
- Check every sqlite3_prepare_v2 result.
- After each row loop, require sqlite3_step() == SQLITE_DONE.
- Validate/decrypt every encrypted note and attachment before any update.
- Check sqlite3_changes() for expected verifier/record updates.
- Make setCryptoFormatV2() return bool and check it before commit.
- Roll back on every error and wipe all temporary plaintext/new-key buffers.

4. Destructive and restore file operations (GL-SEC-104, GL-SEC-107)
- Check every QFile::remove and QFile::rename result.
- New-vault reset must build and validate a staged new DB before replacing the
  old DB, retaining a recoverable original until new unlock succeeds.
- Self-destruct must not claim deletion if the file still exists; wording must
  not promise physical erasure.
- Validate all required tables, columns, supported versions, metadata,
  foreign-key consistency, quick/integrity check, and verifier before install.
- Preserve a recoverable original through post-install validation and handle
  rollback failure explicitly.

5. Attachment correctness (GL-SEC-106)
- Change duplication to decrypt with old attachment-ID AAD and re-encrypt with
  new attachment-ID AAD.
- Include new note creation, all attachment copies, and body URL remapping in
  one transaction.
- Abort without partial notes/orphans on any failure.

6. Parser, limits, and integrity signaling (GL-SEC-008, GL-SEC-010,
   GL-SEC-108, GL-SEC-109, GL-SEC-111)
- Replace unaligned typed header reads with safe byte/endian helpers or memcpy.
- Add checked integer conversions before QByteArray allocation and
  sqlite3_bind_blob.
- Query and verify effective SQLite PRAGMA values after setting them.
- Enforce decoded pixel count and repository-level attachment/aggregate quotas.
- Cache attachment preview data instead of reprocessing every 400 ms.
- Return explicit decryption errors; never convert authentication failure into
  an editable empty note.

7. Export correctness (GL-SEC-110)
- Use QSaveFile and check write/commit results.
- Prevent duplicate sanitized titles from overwriting files.
- Export attachments in bulk export and return a detailed partial-failure
  result to the UI.

8. Dependencies and documentation
- Pin libsodium to an immutable commit or a release archive with a verified
  hash.
- Update README.md and SECURITY.md only after implementation and tests match.
- Preserve accurate legacy XSalsa20 vs current XChaCha20 documentation.

Required automated tests:
- Password rotation succeeds for notes plus attachments.
- Failure injection at every scan, encrypt, update, metadata, marker, and
  commit step leaves the old password and every record usable.
- Interrupted migration is recoverable and never sets the V2 marker early.
- V2 backup created with correct password restores on a clean profile.
- Wrong export password is rejected before output creation.
- Wrong restore password, header tamper, ciphertext tamper, truncation,
  oversize input, malformed KDF values, and wrong schema never change live DB.
- Restore across password rotation installs or requests the correct key and
  cannot save with the stale key.
- Failed install and failed rollback preserve an identified recoverable copy.
- Backup output cannot target the live vault through relative paths, case
  variants, symlinks, or hard links where supported.
- Title/body/note/verifier/attachment ciphertext swaps fail authentication.
- Attachment duplication remains readable and supports later password change.
- Empty plaintext is distinct from authentication failure.
- Self-destruct delete denial never reports successful deletion.
- Duplicate export titles, disk full, short writes, and attachment export
  failures are reported without silent loss.

Use focused changes that follow existing repository patterns. Do not weaken
Argon2id bounds, AEAD associated data, verifier fail-closed behavior, backup
header authentication, URL scheme restrictions, or existing file-size checks.
Do not log passwords, keys, plaintext notes, or decrypted attachments.

When complete:
- Run the full test suite, sanitizer/static-analysis targets if available, and
  a clean build.
- Report each SECURITY_AUDIT2.md finding as fixed, partial, or intentionally
  deferred with exact code and test references.
- Do not claim a workflow is safe unless its failure-path tests pass.
```
