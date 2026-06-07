# GrimLedger Analysis — Codex

**Analysis date:** 2026-06-07  
**Analyzer:** Codex (GPT-5)  
**Scope:** Cryptography, session lifecycle, SQLite repositories, vault creation/unlock/rotation/migration, GRIMBKUP1/2 backup and restore, destructive workflows, notes/attachments/credentials, browser bridge/native host/extension, relevant UI/utilities, tests, CMake, and security-facing documentation.  
**Method:** Read-only static review. This report is the only file created. No build, test, install, git command, network request, or runtime browser/socket operation was performed.

## Executive Summary

GrimLedger is **improved but still mixed compared with audit 3**. The remediation materially strengthened the code: credential CRUD is transactional and checks affected rows, credential list state now uses summaries, exact ports and opt-in subdomains are represented, password generation uses rejection sampling, bridge enablement defaults off, bridge lifecycle is tied to unlock/lock, native-host installation uses a stable per-user directory, backups use SQLite's backup API, and three focused tests now exist.

No Critical issue was statically verified. Browser fill should nevertheless remain disabled for important credentials until the bridge authentication and pending-confirmation lifetime issues are fixed and tested end to end.

Top security risks:

1. The bridge bearer token does not establish native-host identity against a same-user process: the endpoint and token path are predictable, the token is stored in an ordinary user-readable file, and no restrictive `QLocalServer` socket option is selected (`src/bridge/BridgeAuth.cpp:11-25,34-51`; `src/bridge/CredentialBridgeServer.cpp:70-83`).
2. Locking during a browser confirmation can still destroy the bridge object before the delayed callback invokes it, leaving the audit-3 use-after-free path substantially open (`src/bridge/CredentialBridgeServer.cpp:295-305,312-358`; `src/ui/MainWindow.cpp:829-850,1259-1274,1296-1305`).
3. Restore can install a valid replacement, return `false` only because the old rollback copy could not be deleted, and then leave the old session key/editor state active against the new vault (`src/storage/VaultRepository.cpp:1419-1423`; `src/ui/MainWindow.cpp:1028-1039,1067-1086`).
4. The plaintext `allow_subdomains` flag is an authorization policy but is not encrypted or authenticated; database tampering can silently widen a credential from exact-host to subdomain use (`src/storage/Database.cpp:142-162`; `src/storage/CredentialRepository.cpp:55-73`; `src/bridge/CredentialBridgeServer.cpp:239-243`).
5. A credential integrity error is displayed as non-editable, but automatic save paths can still overwrite later corrupt fields with empty ciphertext and destroy recovery evidence (`src/storage/CredentialRepository.cpp:76-88`; `src/ui/MainWindow.cpp:516-568,829-848`; `src/ui/CredentialEditor.cpp:129-140`).

Top bugs/reliability issues:

1. Raw random bridge-token bytes are converted through `QString::fromUtf8()` and back through JSON; invalid UTF-8 bytes do not round-trip, so authenticated bridge requests will fail for many generated tokens (`src/bridge/BridgeAuth.cpp:28-31`; `tools/grimledger_host/main.cpp:87-105`; `src/bridge/CredentialBridgeServer.cpp:112-119`).
2. `rowToSummary()` reads column 3, the encrypted password, as the URL field. Normal V2 credentials therefore become summary integrity errors and browser matching receives an empty URL (`src/storage/CredentialRepository.cpp:63-66,95-117`).
3. New-vault reset still deletes the only live vault before replacement creation and unlock succeed (`src/App.cpp:88-121`).
4. Note create/update/delete and tag mutation remain non-transactional and do not verify exact row changes; a failed create cleanup can leave invalid placeholder ciphertext (`src/storage/NoteRepository.cpp:171-252,366-392`).
5. Plaintext export still ignores short writes, collisions, missing attachments, and per-note failures while reporting global success (`src/storage/NoteRepository.cpp:413-440`; `src/storage/AttachmentRepository.cpp:207-245`; `src/ui/MainWindow.cpp:1128-1150`).

Top feature/improvement opportunities:

1. A vault health dashboard for integrity, backup age, vault size, bridge state, and migration status.
2. TOTP secrets/codes with explicit reveal/copy/fill policy.
3. Authenticated per-site trust levels such as manual-only, username-only, exact-origin fill, and explicit subdomain sharing.
4. Transactional Bitwarden CSV and KeePass XML import with strong plaintext warnings.
5. A broad CI security regression suite covering crypto binding, failure injection, restore, bridge races, and extension navigation.

## Prior Finding Re-Verification Table

| ID | Audit-3 status | Current verified status | Evidence (file:line) | Notes |
|---|---|---|---|---|
| GL-SEC-001 | Accepted risk | **Accepted risk** | `src/App.cpp:136-219`; `src/utils/AppSettings.cpp:11-33`; `src/ui/SettingsWindow.cpp:86-98` | The opt-in unauthenticated trigger remains by product decision. Delete failure is checked and physical erasure is not promised in the final dialog. The settings warning still says "permanently lost," which overstates erasure. |
| GL-SEC-002 | Partially fixed | **Partially fixed** | `src/storage/VaultRepository.cpp:660-930` | Updates and expected row counts are checked, including credentials. All scans/decryption still occur before `BEGIN IMMEDIATE` at line 844, so another instance can add an old-key row between scan and commit. |
| GL-SEC-003 | Partially fixed | **Partially fixed** | `src/storage/VaultRepository.cpp:1047-1074,1358-1425`; `src/ui/MainWindow.cpp:998-1086` | Staging, cryptographic checks, several schema checks, and rollback checks improved. Full schema/column/foreign-key validation is absent, and a cleanup-only failure can leave a replaced vault active under the stale session. See GL-ANALYZE-202. |
| GL-SEC-004 | Verified fixed | **Verified fixed** | `src/storage/VaultRepository.cpp:947-1015,1077-1262` | GRIMBKUP2 verifies the live master password, embeds bounded KDF metadata, authenticates the header, and can restore independently. |
| GL-SEC-005 | Verified fixed | **Verified fixed** | `src/security/CryptoManager.cpp:36-64`; `src/App.cpp:72-85,118-129`; `src/ui/LoginWindow.cpp:163-166` | Login inputs and the direct UTF-8 password buffer are cleared. Ordinary Qt copies remain an acknowledged limitation. |
| GL-SEC-006 | Partially fixed | **Partially fixed** | `src/security/CryptoManager.cpp:144-167,224-245`; `src/storage/VaultRepository.cpp:426-657` | Domain-bound AEAD covers notes, attachments, verifier, credentials, and backups. Migration exact-row checks are present, but enumeration remains outside the transaction snapshot. |
| GL-SEC-007 | Verified fixed | **Verified fixed** | `src/security/CryptoManager.cpp:14-27,36-64`; `src/storage/VaultRepository.cpp:338-423` | KDF/salt bounds, atomic creation, fail-closed verifier handling, and validated metadata remain present. |
| GL-SEC-008 | Partially fixed | **Partially fixed** | `src/storage/Database.cpp:19-40`; `SECURITY.md:13-19` | PRAGMA statements must execute, but effective values are never read back. The original verification criterion is still unmet. |
| GL-SEC-009 | Verified fixed | **Verified fixed** | `src/ui/MarkdownPreview.cpp:13-23`; `src/markdown/MarkdownRenderer.cpp:82-94` | Automatic external opening is disabled and schemes are allowlisted. Approved URLs are loaded with `QTextBrowser::setSource`; runtime behavior was not tested. |
| GL-SEC-010 | Partially fixed | **Partially fixed** | `src/utils/SecurityLimits.h:5-9`; `src/security/CryptoManager.cpp:80-140,176-221`; `src/storage/AttachmentRepository.cpp:27-65` | File limits exist, but generic crypto and blob APIs still narrow Qt sizes to `int`, and aggregate attachment/vault quotas are absent. |
| GL-SEC-011 | Partially fixed | **Partially fixed** | `CMakeLists.txt:12-18,48-57` | SQLite has a SHA-256 URL hash. Libsodium remains pinned to a tag rather than an immutable commit/hash. No network/CVE verification was performed. |
| GL-SEC-012 | Verified fixed | **Verified fixed** | `README.md:25-45`; `SECURITY.md:3-11`; `src/security/CryptoManager.h:15-21` | Legacy XSalsa20-Poly1305 and current XChaCha20-Poly1305 are accurately distinguished. |
| GL-SEC-101 | Partially fixed | **Partially fixed** | `src/ui/MainWindow.cpp:1028-1086`; `src/storage/VaultRepository.cpp:1419-1423` | Normal successful restore stops the bridge, clears UI state, and locks. A successful install followed by rollback-file cleanup failure returns false before those actions. See GL-ANALYZE-202. |
| GL-SEC-102 | Verified fixed | **Verified fixed** | `src/storage/VaultRepository.cpp:933-950,987-1015` | Export rejects a password that does not verify against the current vault before opening the destination. |
| GL-SEC-103 | Partially fixed | **Partially fixed** | `src/storage/VaultRepository.cpp:181-195,426-657,660-930`; `src/storage/DbTransaction.cpp:6-29` | Prepare/final-step, transaction start, marker writes, and row counts are checked. Scans remain outside `BEGIN IMMEDIATE`. |
| GL-SEC-104 | Partially fixed | **Partially fixed** | `src/App.cpp:88-121,162-219` | Delete failures are truthful. New-vault reset is still delete-first and can permanently lose the old vault if creation/unlock fails. |
| GL-SEC-105 | Partially fixed | **Partially fixed** | `src/utils/PathSafety.h:8-32`; `src/ui/MainWindow.cpp:968-995`; `src/storage/VaultRepository.cpp:947-1015` | UI canonical-path checks exist and SQLite backup API is used. Repository APIs do not enforce target safety, hard links are not detected, and output is not reopened/verified. |
| GL-SEC-106 | Partially fixed | **Partially fixed** | `src/storage/AttachmentRepository.cpp:110-192`; `src/ui/MainWindow.cpp:134-153` | New attachment IDs receive new AEAD ciphertext. Note creation, all copies, URL remap, and final note update are still separate operations with partial-success behavior. |
| GL-SEC-107 | Not fixed | **Partially fixed** | `src/storage/VaultRepository.cpp:36-136,1047-1074,1358-1425` | Required-table checks and more rollback errors are handled. Columns, indexes, folders/tags/attachments, `foreign_key_check`, and supported app metadata are not fully validated. |
| GL-SEC-108 | Verified fixed | **Verified fixed** | `src/storage/VaultRepository.cpp:158-178` | Header integers are copied into aligned objects with `memcpy` before endian conversion. |
| GL-SEC-109 | Not fixed | **Not fixed** | `src/utils/ImageSanitizer.h:14-16`; `src/utils/ImageSanitizer.cpp:32-71,92-117`; `src/ui/MainWindow.cpp:1158-1176`; `src/storage/AttachmentRepository.cpp:27-65` | The 8192x8192 decoded ceiling, repeated preview decrypt/base64 work, and missing repository/aggregate quotas remain. |
| GL-SEC-110 | Not fixed | **Not fixed** | `src/storage/NoteRepository.cpp:413-440`; `src/storage/AttachmentRepository.cpp:207-245`; `src/ui/MainWindow.cpp:1128-1150` | Plaintext export still silently accepts short writes, collisions, missing images, and per-file failures. |
| GL-SEC-111 | Not fixed | **Not fixed** | `src/storage/NoteRepository.cpp:28-40,63-168`; `src/ui/MainWindow.cpp:345-440` | Note AEAD failure is still converted to empty text and can later be saved over the corrupt ciphertext. |
| GL-SEC-112 | New/open High | **Partially fixed** | `src/bridge/BridgeAuth.cpp:11-62`; `src/bridge/CredentialBridgeServer.cpp:70-119,188-201`; `tools/grimledger_host/main.cpp:87-108` | A per-session token is required, but it is a bearer value in a predictable file and the endpoint is static. No restrictive socket option or native-host identity proof is present. The token transport is also broken for invalid UTF-8. See GL-ANALYZE-201 and GL-ANALYZE-206. |
| GL-SEC-113 | New/open High | **Partially fixed** | `src/bridge/OriginMatcher.cpp:9-77`; `src/storage/CredentialRepository.cpp:55-73` | Ports and explicit subdomain opt-in are implemented, and HTTPS-to-HTTP downgrade is blocked. Scheme equality is not enforced: an HTTP credential and HTTPS page with the same explicit port can match. The subdomain policy is unauthenticated. |
| GL-SEC-114 | New/open High | **Partially fixed** | `browser-extension/popup.js:20-29,71-97`; `browser-extension/content.js:48-59`; `src/bridge/CredentialBridgeServer.cpp:272-305` | The original tab ID and origin are rechecked and the content script checks origin/sender. The nonce is only required to be non-empty, is not retained/echoed/consumed, document/frame identity is absent, and an active-tab switch does not cancel filling the original now-background tab. |
| GL-SEC-115 | New/open High | **Partially fixed** | `src/bridge/CredentialBridgeServer.cpp:295-305,312-358`; `src/ui/MainWindow.cpp:829-850,1259-1274,1296-1305` | Pending state, lock generation, and post-confirmation rechecks were added. The callback still captures raw bridge `this`; `stopBridge()` can destroy it inside the modal dialog's nested event loop before the callback returns. |
| GL-SEC-116 | New/open Medium | **Partially fixed** | `src/storage/CredentialRepository.cpp:95-147`; `src/ui/MainWindow.cpp:482-506,1237-1243` | Summary/cache and lock clearing are implemented. A column-index bug makes the summary path attempt to decrypt the password as URL; legacy data could place password plaintext in `summary.url`. See GL-ANALYZE-207. |
| GL-SEC-117 | New/open Medium | **Partially fixed** | `src/storage/CredentialRepository.cpp:23-50,70-92`; `src/ui/CredentialEditor.cpp:129-140`; `src/ui/MainWindow.cpp:510-568,829-848` | Integrity failure is explicit and the editor is disabled, but automatic save paths do not check integrity state and can overwrite a partially loaded credential. See GL-ANALYZE-205. |
| GL-SEC-118 | New/open Medium | **Verified fixed** | `src/storage/CredentialRepository.cpp:173-295`; `src/ui/MainWindow.cpp:579-635` | Create is transactional, update/delete check exact changes, and UI save/delete failures are handled. |
| GL-SEC-119 | New/open Medium | **Partially fixed** | `src/bridge/CredentialBridgeServer.cpp:15-17,122-185,239-258,267-269`; `tools/grimledger_host/main.cpp:107-137` | Line/client/match/pending-fill limits and checked desktop response writes were added. There is no per-client request/rate/idle limit; host request write is unchecked and desktop response length is uncapped. |
| GL-SEC-120 | New/open Medium | **Verified fixed** | `src/ui/MainWindow.cpp:829-850,1231-1308`; `src/App.cpp:72-85` | Unlock starts the bridge when enabled; normal lock and restore stop it. Listen failure updates status and warns. |
| GL-SEC-121 | New/open Medium | **Partially fixed** | `native-host/install-windows.ps1:20-65`; `native-host/uninstall-windows.ps1:1-16` | Stable `%LOCALAPPDATA%` installation and ID validation are implemented. Chrome and Edge still share one manifest file, so installing different unpacked IDs sequentially overwrites the first browser's allowed origin; uninstall leaves installed files. |
| GL-SEC-122 | New/open Low | **Partially fixed** | `src/utils/ClipboardUtils.cpp:10-28`; `src/ui/CredentialEditor.cpp:27-29`; `src/ui/MainWindow.cpp:648-669` | The editor warns that clearing is best-effort, but copy dialogs still state unconditionally that the clipboard clears in 20 seconds. Timer-captured secret copies and OS history remain. |
| GL-SEC-123 | New/open Low | **Verified fixed** | `src/utils/PasswordGenerator.cpp:15-45` | Rejection sampling removes modulo bias and the temporary random buffer is wiped. No generator tests exist. |

## New Security Findings

### GL-ANALYZE-201: Bridge authentication is a same-user-readable bearer token, not native-host identity

**Severity:** High  
**Category:** Security  
**Locations:** `src/bridge/BridgeAuth.cpp:11-25,34-55`; `src/bridge/CredentialBridgeServer.cpp:70-119`; `tools/grimledger_host/main.cpp:87-108`

**Description:** The remediation creates a random token but stores it at the predictable app-data path `bridge/session.token`. No explicit file permissions are set, the endpoint name is a constant hash of a public string, and `QLocalServer::UserAccessOption` or an equivalent restrictive option is not selected. Any process running as the same OS user can normally read the same user-owned file and submit the token directly to the local socket. The desktop authenticates possession of the file, not the installed native host, extension, tab, or browser process.

Exact cross-user ACL behavior is platform-dependent and was not runtime verified. The same-user failure follows directly from the design and is within the stated socket-level threat model.

**Risk:** A same-user local process can enumerate matching account metadata, trigger convincing fill prompts for caller-chosen origins, and obtain approved credentials. The dialog still describes the requester as the browser extension even though the desktop has no proof of that identity.

**Remediation:**

- Treat the native host as a broker with an authenticated bootstrap protocol rather than giving every same-user process a reusable token file.
- Use a user-specific unpredictable endpoint and the most restrictive supported local-server access option.
- Bind a short-lived connection/session to the launched native-host process where the platform permits peer/process validation.
- Do not expose metadata before the authenticated session is established.
- Make the confirmation text identify the verified browser/extension session and fail closed when identity cannot be established.

**Acceptance criteria:**

- A process that can read ordinary files in the user's profile but was not launched as the registered native host cannot call `ping`, `list_matches`, or `fill`.
- The endpoint is not derivable from a public constant alone.
- Token/session files have explicit restrictive permissions and are not sufficient by themselves to impersonate the native host.
- The master key remains inside the desktop process.

### GL-ANALYZE-202: Restore can replace the vault, report failure, and retain the old session key

**Severity:** High  
**Category:** Security  
**Locations:** `src/storage/VaultRepository.cpp:1400-1425`; `src/ui/MainWindow.cpp:1028-1039,1067-1086`

**Description:** After a restored vault is installed and passes `quick_check`, failure to remove `vault.grim.bak` makes `installValidatedPlaintextVault()` return false. `MainWindow` reopens the now-restored live path, sees `restored == false`, displays a warning, and returns before stopping the bridge, clearing editors/caches, or locking the old session.

**Risk:** If the backup predates password rotation or otherwise uses another key, the active key and editable UI state no longer match the installed vault. Subsequent note/credential saves can create a mixed-key vault and permanent data loss. The old rollback copy exists, but the UI does not identify the active database/key mismatch or force recovery.

**Remediation:**

- Return a structured restore result that distinguishes `NotInstalled`, `InstalledAndClean`, and `InstalledWithCleanupWarning`.
- Once the live path has changed, always stop the bridge, clear all plaintext state, and lock before returning control to editing code.
- Treat rollback-copy cleanup as a warning after successful installation, not as an installation failure.
- Verify the installed vault cryptographically after reopening, not only with `quick_check`.

**Acceptance criteria:**

- Every path that changes the live vault invalidates the old session before editing or bridge access can continue.
- Failure to delete the rollback copy reports success-with-warning and requires fresh unlock.
- Fault injection at every rename/remove/open step cannot leave an editable vault under an unverified key.

### GL-ANALYZE-203: Subdomain fill authorization is stored as unauthenticated plaintext

**Severity:** Medium  
**Category:** Security  
**Locations:** `src/storage/Database.cpp:142-162`; `src/storage/CredentialRepository.cpp:55-73,181-211,253-270`; `src/bridge/CredentialBridgeServer.cpp:239-243,289-292,348-351`

**Description:** `allow_subdomains` directly controls bridge authorization but is a plaintext integer outside every credential AEAD field. An attacker who can modify the SQLite file can change `0` to `1` without causing an integrity error.

**Risk:** A credential intended for `https://example.com` can silently become eligible for `https://attacker.example.com`. Confirmation displays the target origin, which reduces but does not remove the social-engineering risk. This weakens the advertised per-credential exact-origin policy.

**Remediation:**

- Include all fill-policy fields in authenticated credential state.
- Prefer a versioned encrypted policy blob or bind the policy value into each credential field's associated data.
- Migrate existing rows transactionally and fail closed on missing/invalid policy.

**Acceptance criteria:**

- Flipping `allow_subdomains` in SQLite causes a credential integrity error or an explicit safe migration failure.
- Exact-origin remains the authenticated default.
- Tests cover tampered policy, missing policy, and migration from existing rows.

### GL-ANALYZE-204: Normal lock retains note plaintext in hidden widgets and caches

**Severity:** Medium  
**Category:** Security  
**Locations:** `src/ui/MainWindow.cpp:793-805,829-850,1158-1176,1237-1243`; `src/ui/MainWindow.h:145-156`; `src/App.cpp:38-54`

**Description:** `onLockVault()` clears credential state but does not call `closeNoteEditor()`, clear `m_cachedNotes`, clear the note search string, or blank the preview. `App` hides and reuses the same `MainWindow`. The editor body/title and rendered preview therefore remain allocated while the vault is locked.

Process-memory malware is out of scope, but this finding concerns avoidable retention in application-owned widgets, crash dumps, accessibility/UI bugs, and later state reuse. Complete Qt memory erasure cannot be guaranteed; clearing reachable plaintext is still valuable.

**Risk:** Lock removes the session key but leaves current note plaintext and decrypted titles in ordinary Qt objects. A preview may also retain embedded decrypted image data until it is refreshed.

**Remediation:**

- Clear note editor fields, current IDs, cached notes, search text, preview HTML/resources, and selection before clearing the session key.
- Stop the preview timer or render an explicit locked page while locked.
- Centralize all lock/restore cleanup in one idempotent method used by every lock path.

**Acceptance criteria:**

- After lock, all note/credential widgets and application caches are logically empty.
- Unlock reloads state from the authenticated vault rather than reusing hidden plaintext.
- Tests inspect cleanup hooks while documentation continues to disclaim complete physical memory scrubbing.

### GL-ANALYZE-205: Integrity-error credentials can be automatically overwritten

**Severity:** High  
**Category:** Security  
**Locations:** `src/storage/CredentialRepository.cpp:70-92`; `src/ui/CredentialEditor.cpp:129-140`; `src/ui/MainWindow.cpp:510-568,579-586,829-848,895-899`

**Description:** `rowToCredential()` stops at the first failed field and sets `integrityError`. The editor is disabled and later fields are blanked. However, `saveCurrentCredential()` has no integrity-state guard. It is invoked automatically when switching sections, opening settings, selecting another credential, and locking. If the label decrypted before a later field failed, validation passes and the code encrypts the partial/blank UI values over the original row.

**Risk:** A single corrupt password, URL, or notes field can cause otherwise recoverable ciphertext to be replaced with authenticated empty values during an unrelated lock/navigation action. This destroys recovery evidence and turns tampering into permanent data loss.

**Remediation:**

- Represent the current credential state explicitly as authenticated/editable versus integrity-error/read-only.
- Make every save path refuse integrity-error records, including automatic saves.
- Preserve the original row untouched and offer export/repair diagnostics that never reveal unauthenticated plaintext.

**Acceptance criteria:**

- Navigation, settings, lock, restore, and manual save cannot update an integrity-error credential.
- Corruption in each individual field leaves all database bytes unchanged until an explicit destructive recovery action.
- Valid empty fields remain editable and distinguishable from integrity failure.

## Bug and Reliability Findings

### GL-ANALYZE-206: Random bridge tokens are not safely encoded for JSON transport

**Severity:** Medium  
**Category:** Bug/Reliability  
**Locations:** `src/bridge/BridgeAuth.cpp:28-31`; `tools/grimledger_host/main.cpp:87-105`; `src/bridge/CredentialBridgeServer.cpp:112-119`; `tests/test_bridge_auth.cpp:3-25`

**Description:** `generateToken()` returns 32 arbitrary bytes. The host converts those bytes with `QString::fromUtf8()` and JSON-encodes the result; the server then converts the JSON string back with `toUtf8()` and compares it to the original raw bytes. Invalid UTF-8 sequences are not byte-preserving, so many generated tokens cannot authenticate. The existing test only compares raw file bytes and never covers host-to-JSON-to-server transport.

**Repro hint:** Generate arbitrary high-bit token bytes, run them through `QString::fromUtf8(token).toUtf8()`, and compare against the original.

**Remediation:** Encode tokens as fixed-length lowercase hex or base64url at generation time and validate exact decoded length/canonical form at every boundary.

**Acceptance criteria:**

- Every generated token survives file, JSON, native-host, and local-socket round trips exactly.
- Malformed, non-canonical, truncated, and oversized encodings are rejected.
- An end-to-end test exercises the actual host/server request format.

### GL-ANALYZE-207: Credential summaries read the password column as URL

**Severity:** Medium  
**Category:** Bug/Reliability  
**Locations:** `src/storage/CredentialRepository.cpp:63-66,95-117,120-147`; `src/ui/MainWindow.cpp:482-506`; `src/bridge/CredentialBridgeServer.cpp:239-243`

**Description:** The selected columns are `id, label, username, password, url, notes, ...`, but `rowToSummary()` loops over columns `1 + i` for fields `label`, `username`, and `url`. Its third iteration reads encrypted password column 3 with URL AAD. For V2 rows this produces an integrity error, labels the summary `(integrity error)`, and clears URL. For a legacy row, the fallback can decrypt the password and place it in `summary.url`.

**Risk:** Credential lists/search and browser matching are functionally broken for healthy V2 rows. In an incompletely migrated legacy state, password plaintext can enter a summary field not intended to hold it.

**Remediation:** Use explicit column constants or a dedicated summary SELECT containing only `id, encrypted_label, encrypted_username, encrypted_url, created_at, updated_at, allow_subdomains`.

**Acceptance criteria:**

- Summary queries never select or decrypt password/notes.
- Healthy credentials list and match correctly.
- Tests assert the selected column identities and cover V2, legacy-migration, valid-empty, and integrity-error rows.

### GL-ANALYZE-208: Note CRUD can leave placeholder ciphertext and report false success

**Severity:** Medium  
**Category:** Bug/Reliability  
**Locations:** `src/storage/NoteRepository.cpp:171-252,366-392`

**Description:** Note creation inserts one-byte placeholder blobs, updates the row in a separate operation, attempts unchecked cleanup on failure, and ignores tag-update failure. Update/delete return success on `SQLITE_DONE` even when no row matched. Tag replacement deletes first, ignores its step result, and skips failed tag inserts while returning true.

**Risk:** Disk/I/O/schema failures can leave undecryptable note rows, partial tags, or UI success for a missing note. Such rows can block migration/rotation and be displayed as empty because GL-SEC-111 remains open.

**Remediation:** Use one checked transaction for note insert, ID-bound encryption, tag creation/linking, update, and cleanup. Check all binds, steps, final query status, and exact changes.

**Acceptance criteria:**

- Failure injection leaves either a complete note with complete tags or no new row.
- Updating/deleting a missing ID returns false.
- No one-byte placeholder can remain after any public repository call returns.

### GL-ANALYZE-209: SQLite backup retries BUSY/LOCKED in an unbounded tight loop

**Severity:** Medium  
**Category:** Bug/Reliability  
**Locations:** `src/storage/VaultRepository.cpp:952-969`

**Description:** `sqlite3_backup_step()` is called repeatedly while it returns `SQLITE_BUSY` or `SQLITE_LOCKED`, with no sleep, busy timeout, cancellation, or retry bound. A competing connection can make the UI spin at high CPU for an unbounded period.

**Risk:** Backup can freeze the GUI and provide no actionable failure. Multiple application instances make this more plausible.

**Remediation:** Add bounded retries with a short sleep/event-friendly progress mechanism, honor cancellation, and return a specific busy error. Consider preventing multiple writable app instances.

**Acceptance criteria:**

- A held source/destination lock causes a bounded, surfaced failure or cancellable wait.
- The UI remains responsive and CPU use is bounded.
- Tests cover `BUSY`, `LOCKED`, cancellation, and successful resume.

### GL-ANALYZE-210: Browser fill remains limited to simple top-level login forms

**Severity:** Medium  
**Category:** Bug/Reliability  
**Locations:** `browser-extension/manifest.json:15-20`; `browser-extension/content.js:1-45`; `browser-extension/popup.js:71-101`

**Description:** The content script selects the first password input and first broad text/email candidate, writes `.value` directly, and only dispatches basic events. It does not support current-password versus new-password semantics, multiple forms, shadow DOM, iframes, or framework-native value setters. The popup still owns the long-running confirmation workflow; browser popups commonly close when focus moves to a desktop dialog, but that behavior was not runtime verified here.

**Risk:** Credentials can be placed in the wrong fields, framework-controlled forms may ignore the fill, and confirmation may complete after the popup context is gone. The current success response only means a candidate password field existed.

**Remediation:** Move request state to the service worker, use explicit tab/frame/document binding, rank visible fields by form and autocomplete semantics, support native setters/events, and return per-field results.

**Acceptance criteria:**

- Sign-in, sign-up, password-change, decoy/hidden field, multi-form, framework, iframe, and shadow-DOM fixtures behave deterministically.
- Popup closure does not lose or redirect pending authorization.
- Success means the intended fields in the authorized document were actually updated.

## Feature and Improvement Proposals

| ID | Proposal | User value | Complexity | Security notes | Suggested files |
|---|---|---|---|---|---|
| GL-FEAT-301 | Vault health dashboard | Shows integrity status, backup age, vault size, migration format, attachment usage, and bridge state in one place. | M | Run checks only while unlocked; avoid exposing sensitive counts/details on the login screen. Never auto-repair corruption. | `src/ui/SettingsWindow.*`, `src/storage/VaultRepository.*`, `src/storage/Database.*`, `src/utils/SecurityLimits.h` |
| GL-FEAT-302 | TOTP/2FA secrets and codes | Keeps login password and second factor together with time-based code display/copy. | M | Encrypt TOTP seed with credential-record AAD; make reveal/copy best-effort and do not send TOTP through the bridge without separate approval/policy. | `src/models/Credential.h`, `src/storage/CredentialRepository.*`, `src/ui/CredentialEditor.*`, `src/security/CryptoManager.*` |
| GL-FEAT-303 | Authenticated per-site trust levels | Lets users choose manual-only, username-only, ask-every-time, exact-origin fill, or explicit subdomain fill. | M | Policy itself must be AEAD-authenticated. Default to exact HTTPS origin and never silently widen during migration. | `src/models/Credential*`, `src/storage/CredentialRepository.*`, `src/bridge/OriginMatcher.*`, `src/ui/CredentialEditor.*` |
| GL-FEAT-304 | Bitwarden CSV and KeePass XML import wizard | Reduces migration friction from established password managers. | L | Imports are plaintext at source; warn clearly, avoid temp plaintext files, parse with structured APIs, validate sizes, and commit all selected records transactionally. | New `src/import/*`, `src/storage/CredentialRepository.*`, `src/ui/SettingsWindow.*` |
| GL-FEAT-305 | Full password-manager autofill | Supports robust login, multi-account, framework, iframe, and shadow-DOM flows. | L | Do only after GL-SEC-112 through 115 are closed. Preserve exact document/origin binding and never expose the master key. | `browser-extension/*`, `tools/grimledger_host/main.cpp`, `src/bridge/*` |
| GL-FEAT-306 | Windows Hello convenience unlock | Faster local unlock without repeatedly typing the master password. | L | Wrap a random vault-unlock secret with Windows Hello/DPAPI; retain master-password fallback and revocation. Biometrics must not become an unexportable sole recovery key. | New `src/security/WindowsHello*`, `src/ui/LoginWindow.*`, `src/storage/VaultRepository.*` |
| GL-FEAT-307 | Credential tags and improved encrypted search | Organizes large credential sets and makes local lookup faster. | M | Encrypt tags or explicitly document metadata leakage. Avoid a persistent plaintext full-text index. | `src/models/Credential*`, `src/storage/Database.cpp`, `src/storage/CredentialRepository.*`, `src/ui/CredentialList.*` |
| GL-FEAT-308 | Optional HIBP k-anonymity check | Warns about known-compromised generated/stored passwords. | M | Must be opt-in network access; disclose that a hash prefix leaves the device, rate-limit, and never send full hashes/passwords. Keep local-only behavior as default. | New `src/security/BreachCheck*`, `src/ui/CredentialEditor.*`, `SECURITY.md` |
| GL-FEAT-309 | Attachment quotas and vault-size warnings | Prevents memory spikes and preserves backup capability before the vault hits hard limits. | S/M | Enforce in the repository, use checked decoded-pixel budgets, and keep accepted vault size compatible with backup/export. | `src/utils/ImageSanitizer.*`, `src/utils/SecurityLimits.h`, `src/storage/AttachmentRepository.*`, `src/ui/SettingsWindow.*` |
| GL-FEAT-310 | Linux/macOS native-host installers | Makes the browser bridge support match the documented desktop platforms. | M/L | Use per-user stable paths and restrictive permissions; keep browser IDs separate and provide repair/uninstall. | New `native-host/install-linux.*`, `native-host/install-macos.*`, `browser-extension/README.md`, `CMakeLists.txt` |
| GL-FEAT-311 | Security regression CI | Prevents recurrence of mixed-key, restore, AAD, and bridge lifecycle bugs. | L | Use temporary profiles/databases and synthetic credentials only. Include sanitizers and extension fixtures without production secrets. | `tests/*`, `CMakeLists.txt`, new CI workflow |
| GL-FEAT-312 | Reliable export bundle with manifest | Produces a verifiable folder/zip containing every note, attachment, stable filename, and error manifest. | M | Plaintext warning remains mandatory; use atomic writes, collision-resistant names, and optional checksums, not encryption claims. | `src/storage/NoteRepository.*`, `src/storage/AttachmentRepository.*`, `src/ui/MainWindow.*` |

## Test Coverage Assessment

| Area | Covered by `tests/` | Gaps |
|---|---|---|
| Origin matching | Exact HTTPS, HTTPS-to-HTTP rejection, subdomain default/opt-in, and one port mismatch in `tests/test_origin_matcher.cpp:15-38`. | Scheme equality in both directions, explicit unusual ports, default-scheme credential URLs, malformed URLs, trailing dots, IDN/punycode, IPv4/IPv6, userinfo, public suffixes, and authenticated policy tamper. |
| SQLite helper | One successful update and expected row count in `tests/test_sqlite_utils.cpp:5-32`. | Zero/multiple changes, step errors, constraints, finalized/null statements, transaction rollback, and repository integration. |
| Bridge token primitives | Raw token generation, raw file round-trip, equality/inequality in `tests/test_bridge_auth.cpp:3-25`. | UTF-8/JSON/native-host round-trip, permissions, canonical encoding, token theft, endpoint identity, stale files, concurrent instances, cleanup failure, and isolated temporary app-data paths. |
| Crypto/KDF/AEAD | None. | KDF bounds, verifier, legacy migration, title/body/credential/attachment swaps, tamper/truncation, valid empty fields, wrong keys/AAD, and secret cleanup. |
| Vault create/unlock/reset | None. | Atomic creation, missing verifier, malformed metadata, staged reset, delete denial, self-destruct failure, and fresh-vault recovery. |
| Password rotation/migration | None. | Notes/attachments/credentials together; concurrent insertion between scan and transaction; failure at every prepare/step/update/metadata/commit; old/new password invariants. |
| GRIMBKUP1/2 backup | None. | Clean-machine restore, wrong password, header/payload tamper, size bounds, SQLite snapshot busy/locked, path aliases/hard links, post-write validation, and legacy compatibility. |
| Restore/install/rollback | None. | Full schema/columns/FKs, malformed authenticated DB, all rename/remove/open failures, cleanup-warning result, stale-key prevention, and recovery-file discovery. |
| Note repository | None. | Integrity-error signaling, transactional create/tags, missing IDs, duplicate with attachments, import limits, and export failures/collisions. |
| Credential repository | None. | Summary column mapping, no password decrypt during listing, integrity-error autosave prevention, CRUD rollback, missing IDs, allow-subdomain tamper, and rotation. |
| Attachments/images | None. | Pixel-budget bombs, aggregate quota, repository size enforcement, duplicate rollback, preview cache behavior, and export short writes. |
| Bridge server/native host | None. | Unauthorized direct clients, fake servers, token encoding, line/client/rate/idle bounds, short writes, disconnects, lock during confirmation, destruction safety, and repeated lifecycle. |
| Extension/DOM fill | None. | Navigation, active-tab switch, tab replacement, document/frame nonce binding, popup closure, React/Vue setters, multi-form selection, iframe/shadow DOM, and result verification. |
| Auto-lock/UI cleanup | None. | Application-wide activity, persisted settings, failed-save prompts, note/credential/editor/cache clearing, restore cleanup, and hidden-window plaintext. |
| Clipboard/password generator | None. | Clipboard changed/unavailable/history scenarios, timer ownership, generator length bounds, class policies, and distribution/rejection behavior. |

The three tests are useful smoke tests but do not cover any end-to-end security workflow. No test results are claimed because tests were not run.

## Documentation Accuracy

- `browser-extension/README.md:27` says only the running GrimLedger instance can answer because of the token. The token file is a same-user-readable bearer capability, and its raw-byte UTF-8 transport is unreliable.
- `browser-extension/README.md:18,25` describes exact scheme/host/port matching. `OriginMatcher` does not require scheme equality for an HTTP credential used by an HTTPS page with the same explicit port (`src/bridge/OriginMatcher.cpp:61-70`).
- `SECURITY.md:15-18` describes encrypted notes and attachments but omits encrypted credential fields and the plaintext `allow_subdomains` policy.
- `SECURITY.md:37` says restore never deletes the live vault until checks pass. The validate-before-install statement is broadly true, but documentation does not describe cleanup-warning/recovery-copy states or the stale-session hazard when installation succeeds but cleanup returns failure.
- `README.md:11-21` and `README.md:166-184` omit the credential vault, bridge, native host, extension, and tests from feature/project-structure summaries.
- `src/ui/SettingsWindow.cpp:94-98` says all notes will be "permanently lost," while `App.cpp:214-219` correctly says file deletion does not guarantee physical erasure. Credentials and attachments are also affected.
- `src/ui/MainWindow.cpp:653-669` says clipboard content "clears in 20 seconds" without the best-effort qualification shown in `CredentialEditor`.
- `README.md:148` describes configurable auto-lock accurately in the current tree; persistence and application-wide event filtering are now implemented (`src/utils/AppSettings.cpp:46-63`; `src/ui/MainWindow.cpp:327-342`).
- The browser README's lifecycle claim is now accurate for normal unlock/lock/restore paths (`src/ui/MainWindow.cpp:829-850,1075-1086,1231-1308`).

## Comparison to Audit 3 and Remediation Plan

Materially fixed since audit 3:

- Credential creation/update/delete now use a transaction where needed and exact `sqlite3_changes()` checks.
- Credential list/search uses a summary type and lock/restore clears credential UI/cache state.
- Password generation uses unbiased rejection sampling.
- Bridge enablement defaults off and is persisted.
- Bridge starts after every unlock, stops on lock/restore, surfaces listen failures, and has client/line/match/pending limits.
- Origin matching checks effective ports and uses explicit per-credential subdomain opt-in.
- Extension fill retains the original tab ID, rechecks origin, checks the content-script result, and verifies sender/origin in the content script.
- Native-host installation copies the executable/manifest to a stable `%LOCALAPPDATA%` path and validates extension IDs.
- Backup uses SQLite's backup API rather than copying a live database file.
- Restore checks several required tables and handles more rollback failures.
- Three CTest targets now cover origin basics, SQLite row-count helpers, and raw bridge-token primitives.
- Auto-lock settings persist and activity filtering is installed application-wide.

Still open or only partial:

- Rotation and migration enumerate before acquiring their transaction snapshot.
- New-vault reset remains delete-first.
- Restore validation is not a complete schema/foreign-key/application-version validation.
- Restore result semantics conflate installed-with-warning with not-installed.
- Backup hard-link identity, repository-level path enforcement, post-write verification, and bounded BUSY handling remain.
- Note integrity errors, note CRUD transactions, note/attachment duplication atomicity, aggregate quotas, and plaintext export reliability remain.
- Effective SQLite PRAGMA values are not read back.
- Libsodium is tag-pinned.
- Bridge authentication still does not prove native-host identity; token transport is broken for arbitrary bytes.
- Fill nonce/document binding and lock-safe callback lifetime remain incomplete.
- The new credential summary implementation contains a column-index regression.
- No adversarial, failure-injection, browser, restore, rotation, or repository tests exist.

Regressions/newly exposed issues:

- The bridge token's binary-to-UTF-8 conversion can make the remediated bridge unavailable.
- The summary query reads password ciphertext as URL, breaking normal credential lists and browser matches.
- Restore can change the live vault but return failure on rollback-copy cleanup, bypassing the new lock/clear behavior.
- `allow_subdomains` became a security policy without authenticated storage.
- Credential integrity UI is read-only visually but not protected from automatic saves.
- Note plaintext remains in the hidden main window after lock even though credential plaintext is now cleared.

Overall, the remediation improved engineering discipline and reduced several audit-3 risks, but it did not yet produce a trustworthy browser-credential boundary. The local note vault remains substantially safer than the browser integration. No Critical finding remains, but High-severity data-loss and bridge-lifetime/authentication issues are still present.

## Recommended Next Steps

1. Fix restore result semantics and force lock/clear after any live-vault replacement, including cleanup-warning and rollback-failure paths.
2. Make bridge tokens canonical text (hex/base64url), then redesign authentication so a same-user token file alone cannot impersonate the native host.
3. Remove the pending-confirmation raw-`this` lifetime hazard; use guarded request objects/QPointer and never destroy a bridge with an outstanding callback.
4. Fix the credential summary SELECT/index bug and add repository tests before re-enabling browser matching.
5. Prevent every automatic save of an integrity-error credential; add equivalent explicit integrity results for notes.
6. Authenticate `allow_subdomains` and future fill-policy fields as part of the credential cryptographic record.
7. Move rotation/migration scans inside a verified `BEGIN IMMEDIATE` snapshot and add deterministic concurrency/failure-injection tests.
8. Replace delete-first new-vault reset with staged creation and atomic installation.
9. Finish full restore schema/foreign-key validation, hard-link/path identity enforcement, post-write backup verification, and bounded SQLite backup retries.
10. Make note CRUD/tags and note-plus-attachment duplication transactional with exact row counts.
11. Clear note/editor/cache/preview state on every lock path and stop hidden preview processing while locked.
12. Fix plaintext exports with `QSaveFile`, checked writes, unique filenames, attachment-aware bulk export, and partial-failure reporting.
13. Add decoded-pixel, per-note attachment, and aggregate vault quotas compatible with backup limits.
14. Expand CTest/CI to cover crypto, repositories, rotation, migration, backup/restore, bridge server/host, extension races, auto-lock, clipboard, and password generation.
15. Pin libsodium immutably and update security documentation only after tests demonstrate the claimed behavior.

## Optional Implementation Prompt

```text
Perform a focused remediation pass on GrimLedger using codex_analyze1.md and
SECURITY_AUDIT3.md as the baseline. Preserve GL-SEC-001 as the accepted opt-in
self-destruct risk and never expose the master key outside the desktop process.

Priority 1: prevent data loss.
- Return structured restore outcomes. If the live vault path changed for any
  reason, stop the bridge, clear all note/credential UI and caches, lock the old
  session, and require fresh verifier-backed unlock.
- Treat failure to delete the old rollback copy as success-with-warning.
- Stage new-vault creation and atomically replace the old vault only after the
  new database and password unlock are verified.
- Add explicit note integrity results and prevent all automatic/manual saves of
  integrity-error notes or credentials.
- Make note CRUD/tags and note-plus-attachment duplication transactional with
  exact expected row counts.

Priority 2: repair and secure the browser bridge.
- Encode session tokens canonically as fixed-length hex or base64url; test the
  actual JSON/native-host/socket round trip.
- Replace the same-user-readable bearer-file trust model with a restrictive,
  user-specific, unpredictable endpoint and authenticated native-host session.
- Use the strongest QLocalServer access option and platform peer/process checks
  available. The token file alone must not authorize a direct local client.
- Fix the credential summary query so it selects only label, username, and URL;
  never select/decrypt password or notes for list/search.
- Authenticate allow_subdomains and every future fill-policy value.
- Make confirmation request objects lifetime-safe. Lock/disconnect/timeout must
  cancel them without callbacks invoking a destroyed server.
- Bind and consume a one-time nonce with tab, frame, document, origin, and lock
  generation; revalidate immediately before delivery.
- Keep the bridge disabled by default.

Priority 3: finish storage workflow hardening.
- Acquire verified BEGIN IMMEDIATE before rotation/migration enumeration and
  keep scan, validation, writes, marker/verifier updates, and commit in one
  snapshot.
- Validate the complete restore schema, required columns, metadata versions,
  foreign keys, integrity, and verifier.
- Enforce backup target identity in the repository, including hard links where
  supported; reopen and verify the completed backup.
- Bound SQLITE_BUSY/LOCKED backup retries with cancellation and responsive UI.
- Query and verify effective foreign_keys, secure_delete, and trusted_schema
  PRAGMA values.
- Add checked size conversions and repository-level image/attachment/vault
  quotas.
- Make plaintext export atomic, attachment-aware, collision-safe, and honest
  about partial failures.

Priority 4: locked-state and extension reliability.
- Clear note editor, preview, list/cache/search state, credential state, and
  pending bridge state before clearing the key on every lock/restore path.
- Move long-running fill state out of the popup into the service worker.
- Improve field ranking, framework-native setters, multi-form behavior, frames,
  and shadow DOM while preserving strict origin/document authority.
- Support separate Chrome and Edge manifests/IDs and remove installed files on
  uninstall.
- Make all clipboard text explicitly best-effort.

Required tests:
- Restore install success plus rollback-copy deletion failure cannot retain the
  old key or permit a save.
- Credential summary never reads password/notes and returns correct URL.
- Corruption in each note/credential field cannot be overwritten by navigation,
  settings, lock, restore, or manual save.
- Direct same-user socket clients and fake servers are rejected without native
  host identity; token JSON transport is lossless for every generated token.
- Lock during every confirmation boundary produces no secret response and no
  use-after-free under sanitizers.
- HTTP/HTTPS schemes, ports, subdomains, tampered policy, IDN, IPv4/IPv6,
  malformed URL, navigation, tab/frame/document replacement, popup closure,
  timeout, and disconnect cases.
- Concurrent insert during rotation/migration cannot create mixed-key records.
- Failure injection for note/credential CRUD, duplication, backup, restore,
  rollback, exports, and transaction commit.
- Full schema/foreign-key validation, hard-link targets, BUSY/LOCKED backup,
  decoded-image budgets, aggregate quotas, and locked-state UI cleanup.

Use temporary application-data directories and databases for every test. Do not
touch a real vault or production bridge token. Run the complete test suite,
sanitizers/static analysis, and Chrome/Edge integration fixtures before claiming
any finding fixed.
```
