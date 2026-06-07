# GrimLedger Analysis — auto

**Analysis date:** 2026-06-07  
**Analyzer:** auto (Cursor Auto agent router)  
**Scope:** Security (`src/security/*`, `src/storage/*`, `src/bridge/*`), browser bridge (`browser-extension/*`, `native-host/*`, `tools/grimledger_host/`), UI (`src/App.cpp`, `src/ui/*`), utils, `CMakeLists.txt`, `README.md`, `SECURITY.md`, prior audits SECURITY_AUDIT.md / SECURITY_AUDIT2.md / SECURITY_AUDIT3.md  
**Method:** Read-only static review. This report is the only file created. No build, test, install, git, or network operations were performed.

---

## Executive Summary

**Overall posture vs audit 3: improved, but uneven.** A large remediation pass after audit 3 addressed most High bridge findings in design: per-session token auth, renamed local endpoint, exact origin matching with opt-in subdomains, async fill confirmation with lock-generation checks, bridge lifecycle tied to unlock/lock, `CredentialSummary` listing, credential integrity errors, transactional credential create with `sqlite3_changes`, GRIMBKUP2 via SQLite backup API with password verification, stable native-host install, and three unit tests. **No new Critical issues** were statically verified beyond the accepted GL-SEC-001 product risk.

**Top 5 security risks**

1. **Bridge token file exposure (GL-SEC-112 residual):** Any same-user process that reads `%AppData%/GrimLedger/bridge/session.token` can call `list_matches` and `fill` with a caller-supplied origin (`BridgeAuth.cpp:38-51`, `CredentialBridgeServer.cpp:198-200`).
2. **Fill nonce not bound end-to-end (GL-SEC-114 residual):** Nonce is required but never stored, validated, or echoed on the desktop (`CredentialBridgeServer.cpp:274-276`, `CredentialBridgeServer.h:51-57`).
3. **Lock during fill confirmation UAF risk (GL-SEC-115 residual):** `stopBridge()` destroys the server while a deferred callback may still run after nested modal `exec()` (`MainWindow.cpp:1296-1305`, `CredentialBridgeServer.cpp:303-304`, `GrimMessageDialog.cpp:80-82`). Exact crash/UAF not runtime-verified.
4. **Migration/rotation TOCTOU (GL-SEC-002/103 residual):** Full table scans and decrypt/re-encrypt prep occur before `BEGIN IMMEDIATE` (`VaultRepository.cpp:446-594`, `694-844`).
5. **Note AEAD failures still silent (GL-SEC-111 residual):** Tampered note fields appear as empty editable text (`NoteRepository.cpp:37-38`).

**Top 5 bugs/reliability issues**

1. **Self-destruct leaves bridge listening:** `destroyVaultAfterFailedAttempts` locks session but never calls `stopBridge()` (`App.cpp:162-168`).
2. **New-vault reset deletes live file before replacement proven:** Delete-then-create ordering (`App.cpp:97-110`).
3. **Restore reports failure after successful install** when `.bak` removal fails (`VaultRepository.cpp:1419-1423`).
4. **Bulk export always reports success** despite per-file write failures and title collisions (`NoteRepository.cpp:426-440`, `MainWindow.cpp:1148-1150`).
5. **Preview re-decrypts attachments every 400 ms** while editing (`MainWindow.cpp:75-77`).

**Top 5 feature/improvement opportunities**

1. **Note integrity parity with credentials** — extend `DecryptResult` to notes; highest-value security/UX fix (S).
2. **CI security regression suite** — rotation, restore, bridge lock-during-fill tests (M).
3. **Vault health dashboard** — backup age, bridge status, integrity scan (M).
4. **TOTP/2FA fields in credential vault** — local-only 2FA codes (M).
5. **Per-site fill trust levels** — explicit fill policies beyond origin match (M).

---

## Prior Finding Re-Verification Table

| ID | Audit-3 status | Current verified status | Evidence (file:line) | Notes |
|---|---|---|---|---|
| GL-SEC-001 | Accepted risk | **Accepted risk** | `src/App.cpp:136-219`; `src/utils/AppSettings.cpp:11-33`; `src/ui/SettingsWindow.cpp:86-91` | Opt-in unauthenticated 3-failure deletion remains by product decision. Delete failure now checked (`172-178`); messaging does not promise physical erasure (`217-218`). |
| GL-SEC-002 | Partially fixed | **Partially fixed** | Transactional writes: `src/storage/VaultRepository.cpp:844-930`; scans pre-tx: `694-740`, `446-492` | `BEGIN IMMEDIATE` wraps updates; `expectChanges` on row updates (`856,874,891,914`). Reads/decrypt prep still outside transaction snapshot. |
| GL-SEC-003 | Partially fixed | **Partially fixed** | Staged restore: `VaultRepository.cpp:1077-1356`; UI lock: `MainWindow.cpp:1075-1086` | `validateVaultFile` checks table existence + `quick_check` + metadata (`1060-1073`), not full FK/column schema. Install rollback improved (`1376-1416`). |
| GL-SEC-004 | Verified fixed | **Verified fixed** | `VaultRepository.cpp:947-950`; `MainWindow.cpp:987-995` | `verifyMasterPassword` before GRIMBKUP2 write. SQLite `backup_init/step` snapshot (`952-968`). No post-write reopen verify of finished file. |
| GL-SEC-005 | Verified fixed | **Verified fixed** | `src/security/CryptoManager.cpp:44-57`; `src/ui/LoginWindow.cpp:163-166`; `src/App.cpp:79-84,125-127` | Login fields and KDF UTF-8 buffer cleared. Qt `QString` implicit sharing not fully scrubbed (documented). |
| GL-SEC-006 | Partially fixed | **Partially fixed** | AEAD: `CryptoManager.cpp:224-245`; migration incl. credentials: `VaultRepository.cpp:426-657` | Domain-bound AAD on notes, attachments, credentials, verifier. Migration scan outside tx (`446-592` then `594`). |
| GL-SEC-007 | Verified fixed | **Verified fixed** | `CryptoManager.cpp:14-27`; `VaultRepository.cpp:338-410,375-383` | KDF/salt bounds, atomic vault create, fail-closed missing verifier. |
| GL-SEC-008 | Partially fixed | **Partially fixed** | `src/storage/Database.cpp:28-39`; `SECURITY.md:19` | PRAGMAs set; open fails on execute error. Effective values never queried after set. |
| GL-SEC-009 | Verified fixed | **Verified fixed** | `src/ui/MarkdownPreview.cpp:15-22` | `setOpenExternalLinks(false)`; scheme allowlist on click. |
| GL-SEC-010 | Partially fixed | **Partially fixed** | `src/utils/SecurityLimits.h:6-8`; `VaultRepository.cpp:974-975,981`; `AttachmentRepository.cpp:56-64` | Size limits on backup/restore. `qsizetype`→`int` narrowing at `981`. No aggregate attachment quota in repository. |
| GL-SEC-011 | Partially fixed | **Partially fixed** | `CMakeLists.txt:14-18,49-52` | SQLite URL_HASH pinned. Libsodium fetched by mutable `GIT_TAG 1.0.20-RELEASE`, not immutable commit hash. |
| GL-SEC-012 | Verified fixed | **Verified fixed** | `README.md:29-34`; `SECURITY.md:5-8`; `src/security/CryptoManager.h:17-19` | XChaCha20-Poly1305 and legacy XSalsa20 named accurately. README still omits credentials/bridge (see Documentation). |
| GL-SEC-101 | Partially fixed | **Mostly fixed** | `MainWindow.cpp:1075-1086`, `1237-1242` | Restore stops bridge, clears cred/note cache, locks session. **Supersedes audit-3** claim that cred cache was not cleared. GRIMBKUP1 uses session key during restore (`1049-1050`) by design. |
| GL-SEC-102 | Verified fixed | **Verified fixed** | `VaultRepository.cpp:947-950`; `MainWindow.cpp:987-994` | Wrong backup password rejected before write. |
| GL-SEC-103 | Partially fixed | **Partially fixed** | `queryAllRows`: `VaultRepository.cpp:181-195`; `expectChanges`: `608,623,644,874,891,914` | Query completion and row counts checked on updates. Scans remain pre-transaction (`446-492`, `694-740`). `storeVaultInfo` salt update lacks `expectChanges` (`275-277`, `922-924`). |
| GL-SEC-104 | Partially fixed | **Partially fixed** | Self-destruct: `App.cpp:172-178`; new-vault: `App.cpp:97-110` | Delete failures reported. New-vault reset still deletes live vault before create succeeds. |
| GL-SEC-105 | Partially fixed | **Partially fixed** | `PathSafety.h:24-33`; `MainWindow.cpp:979-985`; `VaultRepository.cpp:1002-1015` | UI blocks canonical match to live/restore paths. No hard-link/same-inode detection; repository API does not enforce path rule; no post-commit backup reopen. |
| GL-SEC-106 | Partially fixed | **Partially fixed** | `AttachmentRepository.cpp:158-168`; `MainWindow.cpp:134-156` | Copied attachments re-encrypt with new AAD. Note+attachment duplicate not one transaction; partial `idMap` possible. |
| GL-SEC-107 | Not fixed | **Partially fixed** | `validateVaultFile`: `1060-1073`; rollback: `1376-1424` | **Improved since audit-3.** Rollback paths check rename/remove; post-install `quick_check` (`1400-1416`). Still missing `note_attachments`/`folders` table checks, column-level validation, verifier on staging file before install decision. `.bak` removal failure fails restore after success (`1419-1423`). |
| GL-SEC-108 | Verified fixed | **Verified fixed** | `VaultRepository.cpp:166-174` | `memcpy` + endian for backup header integers. |
| GL-SEC-109 | Not fixed | **Not fixed** | `ImageSanitizer.h:16`; `MainWindow.cpp:75-77,1174-1175` | 8192×8192 decoded ceiling; preview timer re-decrypts; no aggregate vault attachment quota. |
| GL-SEC-110 | Not fixed | **Not fixed** | `NoteRepository.cpp:421-422,437-438`; `MainWindow.cpp:1148-1150` | `exportMarkdownFile` write result ignored in bulk path; UI always reports success; duplicate sanitized titles silently overwrite. |
| GL-SEC-111 | Not fixed | **Partially fixed** | Notes: `NoteRepository.cpp:37-38`; Credentials: `CredentialRepository.cpp:83-85`; UI: `CredentialEditor.cpp:129-140`; bridge: `CredentialBridgeServer.cpp:285-287,344-346` | Credential integrity signaling implemented. Notes still return empty `QString` on AEAD failure. |
| GL-SEC-112 | High / open | **Partially fixed** | Token: `BridgeAuth.cpp:28-42`, `CredentialBridgeServer.cpp:73-77,198-200`; endpoint: `BridgeAuth.cpp:22-25` | Per-launch 32-byte token required on every request; constant-time compare (`58-62`). Endpoint name deterministic from static seed; token file has no restrictive permissions; any same-user token holder can impersonate extension. |
| GL-SEC-113 | High / open | **Verified fixed** | `OriginMatcher.cpp:55-77`; `Database.cpp:160-162`; `test_origin_matcher.cpp:17-38` | Scheme+port+host; HTTPS cred blocked on HTTP page; subdomains opt-in per credential. **Supersedes audit-3** "unsafe hostname-only" claim. |
| GL-SEC-114 | High / open | **Partially fixed** | Tab/origin: `popup.js:26-28,62,81-84`; content: `content.js:54-56`; nonce gap: `CredentialBridgeServer.h:51-57`, `274-276` | Tab ID retained; origin re-checked before DOM fill. Nonce required but not stored/validated/consumed on desktop. No frame ID binding; tab not re-checked during desktop confirmation wait. |
| GL-SEC-115 | High / open | **Partially fixed** | Async: `MainWindow.cpp:1263-1274`; lock gen: `CredentialBridgeServer.cpp:300,324-326,89-91`; nested exec: `GrimMessageDialog.cpp:80-82` | Fill returns before dialog; `lockGeneration` checked post-confirm. `DialogUtils::question` still uses nested `exec()`; `stopBridge()` during open dialog may UAF — not runtime-verified. |
| GL-SEC-116 | Medium / open | **Mostly fixed** | `CredentialSummary.h:6-14`; `CredentialRepository.cpp:95-138`; `MainWindow.cpp:484-486,1237-1242` | List/search/bridge use summaries without password decrypt. Password loaded only on selection (`511`) or fill (`280,343`). `listCredentials()` still decrypts all fields if called (`140-147`) but UI does not use it. Selected cred plaintext remains in editor widgets until lock. |
| GL-SEC-117 | Medium / open | **Partially fixed** | Credentials: `RepositoryResult.h:6-18`, `CredentialRepository.cpp:28-50,83-85`; Notes: `NoteRepository.cpp:37-38` | Credential AEAD failure → `integrityError`; UI/bridge block edit/fill. Notes unchanged. |
| GL-SEC-118 | Medium / open | **Mostly fixed** | `CredentialRepository.cpp:176-233,277,295`; `DbTransaction.cpp:11-18` | Transactional create with ID-bound re-encrypt; `expectChanges` on update/delete. `updateCredential` inside create tx is not separately transactional but rolls back via `DbTransaction` destructor. Note create still non-transactional (`NoteRepository.cpp:171-206`). |
| GL-SEC-119 | Medium / open | **Partially fixed** | Limits: `CredentialBridgeServer.cpp:15-17,135-139,169-172,250-251,267-269` | 64 KiB line cap, 4 clients, 64 matches, one pending fill. No per-client rate limit; `writeResponse` ignores short writes (`127-128`); host ignores native write failure (`main.cpp:163`). |
| GL-SEC-120 | Medium / open | **Verified fixed** | `MainWindow.cpp:1231-1234,846-848,1075`; `App.cpp:85,129` | `onVaultUnlocked` → `startBridge()`; lock/restore → `stopBridge()`. **Supersedes audit-3** "permanently offline after first lock" claim. Listen failure surfaced (`1276-1280,1283-1287`). Self-destruct path exception: see GL-ANALYZE-205. |
| GL-SEC-121 | Medium / open | **Verified fixed** | `native-host/install-windows.ps1:27-31,34-58` | Stable `%LOCALAPPDATA%\GrimLedger\native-host\`; extension ID regex; `-Browser Chrome\|Edge\|Both`. Default host binary still copied from build output (`10-17`). |
| GL-SEC-122 | Low / open | **Partially fixed** | `CredentialEditor.cpp:28-29`; `ClipboardUtils.cpp:9-23` | UI states clipboard clear is best-effort. Timer lambda still captures plaintext `QString` by value for 20 s. |
| GL-SEC-123 | Low / open | **Verified fixed** | `PasswordGenerator.cpp:21-41` | Rejection sampling (`maxUnbiased`). **Supersedes audit-3** modulo-bias claim. |

---

## New Security Findings

### GL-ANALYZE-201: Bridge session token readable by any same-user process

- **Severity:** High  
- **Category:** Security  
- **Locations:** `src/bridge/BridgeAuth.cpp:18-51`; `src/bridge/CredentialBridgeServer.cpp:112-120,198-200`  
- **Description:** The per-launch token is written to a predictable path under AppData with default file permissions. The native host reads it and injects it into every request. Any local process running as the same user that can read this file gains the same bridge API as the registered extension, including `list_matches` (metadata enumeration) and `fill` (user confirmation still required).  
- **Risk:** Breaks the intended extension/native-host trust boundary without requiring memory compromise. Social engineering via misleading "browser fill" dialog remains possible for direct socket clients. Master key is not exposed.  
- **Remediation:** Restrict token file permissions (platform-specific ACL); consider named-pipe or OS-gated delivery; bind requests to extension process identity where feasible; shorten token lifetime.  
- **Acceptance criteria:** Same-user process without token file access cannot call protected bridge actions; token file is not world-readable.

### GL-ANALYZE-202: Bridge endpoint name is deterministic, not per-user random

- **Severity:** Medium  
- **Category:** Security  
- **Locations:** `src/bridge/BridgeAuth.cpp:22-25`  
- **Description:** Endpoint name is `grimledger-` + first 16 hex chars of SHA-256 of static seed `grimledger-bridge-v2`. Any reviewer of source or binary can derive the socket name.  
- **Risk:** Reduces obscurity benefit of renaming from `grimledger-bridge`; aids socket impersonation attempts (mitigated by token requirement).  
- **Remediation:** Mix per-launch or per-user random material into endpoint derivation; store mapping only in protected session state.  
- **Acceptance criteria:** Endpoint name is not derivable from public source alone; old fixed name cannot be used.

### GL-ANALYZE-203: Fill nonce required but never validated or consumed

- **Severity:** High  
- **Category:** Security  
- **Locations:** `browser-extension/popup.js:72-78`; `src/bridge/CredentialBridgeServer.cpp:274-276,295-301`; `src/bridge/CredentialBridgeServer.h:51-57`  
- **Description:** Extension generates `crypto.randomUUID()` and sends `nonce` with fill requests. Server rejects empty nonce but `PendingFill` has no nonce field and `completePendingFill` never compares it. Response does not echo nonce.  
- **Risk:** Cannot detect replay, response swapping, or cross-request confusion at desktop boundary. GL-SEC-114 acceptance criteria unmet.  
- **Remediation:** Store nonce in `PendingFill`; echo in response; reject mismatch; consume once.  
- **Acceptance criteria:** Fill response rejected if nonce does not match pending request; nonce cannot be reused.

### GL-ANALYZE-204: Lock during fill confirmation may use destroyed bridge object

- **Severity:** High  
- **Category:** Security  
- **Locations:** `src/bridge/CredentialBridgeServer.cpp:303-305,312-358`; `src/ui/MainWindow.cpp:846-848,1263-1274,1296-1305`; `src/ui/GrimMessageDialog.cpp:80-82`  
- **Description:** Fill handler invokes `m_confirmFill` then returns. Confirmation runs via `QTimer::singleShot(0)` into `DialogUtils::question` which calls nested `exec()`. While dialog is open, auto-lock can call `stopBridge()` which `reset()` destroys `CredentialBridgeServer` while `completePendingFill` lambda captures `this`. `lockGeneration` mitigates credential release but does not prevent object lifetime issues.  
- **Risk:** Application crash or use-after-free near auto-lock expiry. **Not runtime-verified** with sanitizers.  
- **Remediation:** Hold pending fills in a separate `QObject` or `shared_ptr` state machine that survives server teardown; cancel UI callback in `stopBridge()`; avoid nested `exec()` for security prompts.  
- **Acceptance criteria:** Lock during pending confirmation returns error without crash; ASan/TSan clean on deterministic test.

### GL-ANALYZE-205: Self-destruct path does not stop bridge listener

- **Severity:** Medium  
- **Category:** Security  
- **Locations:** `src/App.cpp:162-168` (contrast `MainWindow.cpp:846-848`)  
- **Description:** After three failed unlock attempts, `destroyVaultAfterFailedAttempts` calls `m_session->lock()` and hides `MainWindow` but never `stopBridge()`. Hidden `MainWindow` may retain a listening bridge until process exit.  
- **Risk:** Credential endpoint may remain reachable after vault deletion attempt while login screen is shown. Protected actions fail without key, but `ping` and token-authenticated status still respond.  
- **Remediation:** Call `m_main->stopBridge()` (or equivalent) before lock in self-destruct path.  
- **Acceptance criteria:** Self-destruct leaves no live bridge endpoint.

### GL-ANALYZE-206: HTTP-stored credential can fill on HTTPS page

- **Severity:** Low  
- **Category:** Security  
- **Locations:** `src/bridge/OriginMatcher.cpp:61-63`  
- **Description:** Matcher blocks HTTPS credential on HTTP page but does not block HTTP credential on HTTPS page. Only scheme downgrade in one direction is prevented.  
- **Risk:** Credential saved with `http://` origin can be offered on `https://` page if host/port match. Lower than full downgrade but widens fill surface.  
- **Remediation:** Require stored credential scheme to match page scheme, or normalize credentials to HTTPS on save.  
- **Acceptance criteria:** `http://example.com` credential does not match `https://example.com` unless explicitly configured.

### GL-ANALYZE-207: Binary session token transported as UTF-8 JSON string

- **Severity:** Low  
- **Category:** Security  
- **Locations:** `tools/grimledger_host/main.cpp:105`; `src/bridge/CredentialBridgeServer.cpp:117-119`  
- **Description:** Native host inserts token via `QString::fromUtf8(sessionToken)` into JSON. Random 32-byte token may contain invalid UTF-8 sequences, causing conversion/re-serialization to alter bytes. Server compares via `token.toUtf8()`.  
- **Risk:** Intermittent auth failures or brittle boundary if UTF-8 round-trip is lossy. **Behavior not runtime-verified** across all byte patterns.  
- **Remediation:** Base64-encode token in JSON transport; decode on server.  
- **Acceptance criteria:** All 32-byte random tokens authenticate reliably through native host round-trip.

### GL-ANALYZE-208: Migration and password rotation lack snapshot isolation

- **Severity:** High  
- **Category:** Security  
- **Locations:** `src/storage/VaultRepository.cpp:446-594,694-844`  
- **Description:** `migrateDomainBoundCrypto` and `changeMasterPassword` read and decrypt all rows, then start `DbTransaction` only for writes. Another writer (not possible in single-process UI today, but possible with future multi-instance or manual DB access) could modify rows between scan and commit.  
- **Risk:** Mixed-key vault after rotation/migration if concurrent modification occurs; rotation failure mid-scan leaves vault unchanged (fail-safe) but scan itself is not a consistent snapshot.  
- **Remediation:** `BEGIN IMMEDIATE` before reads used for migration/rotation decisions, or use SQLite backup API for consistent snapshot before transform.  
- **Acceptance criteria:** Injected concurrent write during rotation leaves vault fully on old or new key; no partial record sets.

### GL-ANALYZE-209: `list_matches` exposes credential metadata without fill approval

- **Severity:** Informational  
- **Category:** Security  
- **Locations:** `src/bridge/CredentialBridgeServer.cpp:232-258`  
- **Description:** Any valid token holder receives up to 64 credential IDs, labels, and usernames for a caller-supplied origin without user confirmation.  
- **Risk:** Acceptable for extension UX; problematic when combined with GL-ANALYZE-201 (arbitrary local clients).  
- **Remediation:** Require confirmation for metadata enumeration, or rate-limit and log `list_matches`.  
- **Acceptance criteria:** Documented threat model acknowledges metadata disclosure to token holders.

---

## Bug and Reliability Findings

### GL-ANALYZE-220: Restore returns failure after successful install when backup cleanup fails

- **Severity:** Medium  
- **Category:** Bug/Reliability  
- **Locations:** `src/storage/VaultRepository.cpp:1419-1423`  
- **Description:** After successful install and post-install `quick_check`, if `vault.grim.bak` cannot be removed, function returns `false` with error "Restore succeeded but old vault backup could not be removed."  
- **Risk:** UI may report restore failure when vault was actually replaced; user confusion; leftover `.bak` consumes disk.  
- **Repro hint:** Restore with read-only or locked `.bak` file in vault directory.  
- **Remediation:** Treat `.bak` removal as best-effort warning, not failure.

### GL-ANALYZE-221: New-vault creation deletes live database before replacement succeeds

- **Severity:** High  
- **Category:** Bug/Reliability  
- **Locations:** `src/App.cpp:97-110`  
- **Description:** When creating a vault over an existing one, code closes DB, removes `vault.grim`, then reopens and calls `createVault`. Power loss or `createVault` failure after delete leaves no vault.  
- **Risk:** Permanent data loss on destructive reset failure.  
- **Repro hint:** Fill disk or interrupt after `QFile::remove` during create-over-existing flow.  
- **Remediation:** Stage new vault to temp path; atomic rename on success (mirror `installValidatedPlaintextVault`).

### GL-ANALYZE-222: Note creation is not transactional

- **Severity:** Medium  
- **Category:** Bug/Reliability  
- **Locations:** `src/storage/NoteRepository.cpp:171-206`  
- **Description:** `createNote` INSERTs placeholder blobs, then `updateNote` with real ciphertext, then `setNoteTags`. No `DbTransaction`. Failed `updateNote` attempts `deleteNote` cleanup.  
- **Risk:** Orphan or undecryptable note row if cleanup fails.  
- **Remediation:** Wrap insert/update/tags in one `DbTransaction` with `expectChanges`.

### GL-ANALYZE-223: Attachment duplication can leave partial copy

- **Severity:** Medium  
- **Category:** Bug/Reliability  
- **Locations:** `src/storage/AttachmentRepository.cpp:155-190`; `src/ui/MainWindow.cpp:134-156`  
- **Description:** Each attachment copied in a loop without wrapping transaction. Failure mid-loop returns partial `idMap`.  
- **Risk:** Note body references new attachment IDs that were not all inserted.  
- **Remediation:** Single transaction for note duplicate + all attachment copies.

### GL-ANALYZE-224: Bulk Markdown export reports unconditional success

- **Severity:** Medium  
- **Category:** Bug/Reliability  
- **Locations:** `src/storage/NoteRepository.cpp:426-440`; `src/ui/MainWindow.cpp:1148-1150`  
- **Description:** `exportAllMarkdown` always returns `true`; ignores `exportMarkdownFile` write failures; duplicate sanitized titles overwrite same path.  
- **Risk:** User believes all notes exported; silent data loss in export set.  
- **Repro hint:** Export to read-only directory or with duplicate note titles.  
- **Remediation:** Return count/failures; detect filename collisions.

### GL-ANALYZE-225: Extension fill uses first password field heuristic

- **Severity:** Medium  
- **Category:** Bug/Reliability  
- **Locations:** `browser-extension/content.js:1-21`  
- **Description:** `findLoginFields` selects first `input[type=password]` and first text/email candidate. No support for `autocomplete` attributes, multiple accounts, sign-up vs login, or `current-password` vs `new-password` beyond skipping `new-password`.  
- **Risk:** Wrong field filled on complex forms.  
- **Remediation:** Prefer `autocomplete=username/current-password`; support multiple candidate scoring.

### GL-ANALYZE-226: Live preview re-decrypts attachments on 400 ms timer

- **Severity:** Medium  
- **Category:** Bug/Reliability  
- **Locations:** `src/ui/MainWindow.cpp:75-77`  
- **Description:** `QTimer` fires `updatePreview` every 400 ms while editing, triggering attachment decrypt and base64 embedding.  
- **Risk:** CPU/memory pressure on image-heavy notes (GL-SEC-109 adjacent).  
- **Remediation:** Cache decrypted preview payload per attachment ID; invalidate on edit.

### GL-ANALYZE-227: Bridge socket write failures silently ignored

- **Severity:** Low  
- **Category:** Bug/Reliability  
- **Locations:** `src/bridge/CredentialBridgeServer.cpp:127-128`; `tools/grimledger_host/main.cpp:117-118,163`  
- **Description:** `writeResponse` returns without retry/error if `socket->write` short-writes. Native host does not check write completeness.  
- **Risk:** Truncated JSON responses under backpressure; extension parse errors.  
- **Remediation:** Loop until full payload written or disconnect.

### GL-ANALYZE-228: Extension popup may abandon async fill on focus loss

- **Severity:** Low  
- **Category:** Bug/Reliability  
- **Locations:** `browser-extension/popup.js:71-101`  
- **Description:** Fill runs in popup context; browser action popups typically close when focus moves to desktop confirmation dialog.  
- **Risk:** Fill may not complete or error handling may not run. **Not runtime-verified** under Chrome/Edge.  
- **Remediation:** Move fill orchestration to service worker background script.

---

## Feature and Improvement Proposals

| ID | Proposal | User value | Complexity | Security notes | Suggested files |
|---|---|---|---|---|---|
| GL-FEAT-301 | TOTP/2FA code fields in credential vault | Copy 2FA codes without separate app | M | Secrets in RAM while editing; clipboard exposure — reuse best-effort policy | `Credential.h`, `CredentialEditor.cpp`, `CredentialRepository.cpp` |
| GL-FEAT-302 | Per-site fill trust levels (ask always / never / bridge-off per origin) | Finer control than global bridge toggle | M | Misconfiguration could auto-fill on untrusted origins — default to ask | `OriginMatcher.cpp`, `AppSettings.cpp`, `SettingsWindow.cpp` |
| GL-FEAT-303 | Note integrity parity (`DecryptResult` for notes) | Detect tampering; prevent overwrite of corrupt ciphertext | S | Pure security win; mirror credential pattern | `NoteRepository.cpp`, `MainWindow.cpp`, `NoteEditor.cpp` |
| GL-FEAT-304 | Vault health dashboard (backup age, bridge status, integrity scan) | Recovery confidence at a glance | M | Scan must not leak plaintext; run only unlocked | New UI panel, `VaultRepository.cpp` |
| GL-FEAT-305 | CI security regression suite expansion | Prevent remediation regressions | M | Tests may need temp vault fixtures | `tests/`, `CMakeLists.txt` |
| GL-FEAT-306 | Import Bitwarden CSV / KeePass XML | Migration from other managers | L | Plaintext in memory during import; clear after import | New `import/` module, UI wizard |
| GL-FEAT-307 | Windows Hello convenience unlock (wrap master key) | Faster unlock UX | L | **High risk if implemented carelessly** — Hello key must not replace Argon2id verifier; optional only | `VaultSession.cpp`, platform auth module |
| GL-FEAT-308 | Credential tags and advanced search | Organize large vaults | S | Tags/metadata plaintext in DB unless encrypted | `Database.cpp`, `CredentialRepository.cpp` |
| GL-FEAT-309 | Attachment quota UI and vault size warnings | Avoid unbounded growth / backup surprises | S | Enforce `SecurityLimits` in repository, not just UI | `AttachmentRepository.cpp`, `SecurityLimits.h` |
| GL-FEAT-310 | Linux/macOS native-host installers | Cross-platform bridge | M | Same token/origin threat model; document platform socket ACLs | `native-host/install-linux.sh`, `install-macos.sh` |
| GL-FEAT-311 | Offline breach check (HIBP k-anonymity) | Warn on known leaked passwords | M | Requires network; contradicts strict local-only unless opt-in | `PasswordGenerator.cpp`, settings |
| GL-FEAT-312 | Framework-aware autofill (React/Vue native setters) | Works on modern SPAs | L | Does not widen origin authority | `browser-extension/content.js` |
| GL-FEAT-313 | Full password-manager autofill (cards, addresses, passkeys) | Broader utility | L | Large new attack surface; defer until bridge hardened | Extension + bridge protocol v3 |

---

## Test Coverage Assessment

| Area | Covered by tests/ | Gaps |
|---|---|---|
| BridgeAuth token I/O | `tests/test_bridge_auth.cpp` — generate, write, read, clear, constant-time compare | Endpoint uniqueness; unauthorized socket rejection; token file permissions |
| Origin matching | `tests/test_origin_matcher.cpp` — exact match, HTTPS downgrade, subdomain flag, port mismatch | IDN/punycode, trailing dots, IPv6, userinfo, public suffix, HTTP-on-HTTPS, malformed URLs |
| SqliteUtils | `tests/test_sqlite_utils.cpp` — `stepDone`, `expectChanges` | Integration with real credential/vault mutations |
| CredentialBridgeServer protocol | **None** | Token reject, fill cancel on lock, pending-fill limits, oversize lines, many clients |
| Bridge lifecycle unlock/lock/unlock | **None** | Regression for GL-SEC-120 |
| Lock during fill dialog | **None** | GL-SEC-115 / GL-ANALYZE-204 |
| Password rotation / migration | **None** | Failure injection, concurrent write simulation |
| GRIMBKUP2 backup/restore round-trip | **None** | Core recovery path |
| Restore rollback | **None** | GL-SEC-107 scenarios |
| Credential CRUD transactions | **None** | Insert/update/delete failure injection |
| Note integrity | **None** | AEAD failure vs empty field |
| CryptoManager KDF/AEAD | **None** | Vectors for AAD binding, wrong-key rejection |
| Extension integration | **None** | Tab race, nonce binding, content-script fill verification |
| PathSafety / backup targets | **None** | Hard-link, symlink to live vault |
| Self-destruct / new-vault reset | **None** | Destructive failure paths |

**Build integration:** `enable_testing()` and `add_grimledger_test` register three tests (`CMakeLists.txt:195-216`). Audit-3 claim of zero tests is **stale**.

---

## Documentation Accuracy

- **README.md main features (`9-21`):** Lists notes only; omits credential vault, browser bridge, and password generator.
- **README.md dependencies (`78`):** Lists Qt Core/Gui/Widgets only; omits **Qt Network** required for bridge (`CMakeLists.txt:10,179`).
- **README.md project structure (`168-183`):** Missing `bridge/`, `CredentialRepository`, `Credential` models, `SecurityLimits`, `PathSafety`, `browser-extension/`, `native-host/`, `tests/`.
- **README.md security model (`41-42`):** Says note titles/bodies/attachments encrypted; does not mention credential fields (also encrypted per `CredentialRepository.cpp:13-20`).
- **SECURITY.md data at rest (`15`):** Same omission — credentials not listed though encrypted in DB.
- **SECURITY.md backups (`35-37`):** Accurate for GRIMBKUP2 password-verified restore.
- **browser-extension/README.md:** Largely accurate post-remediation — disabled by default, per-session token, exact origin, confirmation, bridge restarts on unlock (`16-27`).
- **SECURITY_AUDIT3.md non-security bug list:** **Stale** on auto-lock persistence (now in `AppSettings.cpp:46-64`, loaded `SettingsWindow.cpp:61-73`, applied `MainWindow.cpp:72-73`); **stale** on app-wide activity filter (`MainWindow.cpp:327` installs on `qApp`); **stale** on zero tests; **stale** on credential delete hidden (now warns `MainWindow.cpp:624-629`); **stale** on GL-SEC-101 cred cache not cleared.
- **SECURITY_AUDIT3.md GL-SEC-113/120/123:** Origin, lifecycle, and generator claims **superseded** by current code (see re-verification table).

---

## Comparison to Audit 3 and Remediation Plan

### What remediation fixed (verified in current tree)

- Bridge per-session token + renamed endpoint + fail-closed missing handler (`CredentialBridgeServer.cpp:263-265`)
- Exact origin matching with HTTPS downgrade block and per-credential `allow_subdomains` (`OriginMatcher.cpp:61-77`)
- Async fill deferral + `lockGeneration` + `cancelPendingRequests` on stop
- Bridge start/stop tied to unlock/lock/restore; disabled by default
- Stable Windows native-host install under `%LOCALAPPDATA%\GrimLedger\native-host\`
- `CredentialSummary` listing; credential integrity errors; transactional credential create; `expectChanges` on credential update/delete
- GRIMBKUP2 password verification + SQLite backup API snapshot
- Credentials included in `migrateDomainBoundCrypto`
- Restore clears sensitive UI state and forces re-login
- Password generator rejection sampling
- Unit tests for BridgeAuth, OriginMatcher, SqliteUtils

### What remains open or partial

- Bridge token trust boundary (GL-SEC-112) — token file exposure, deterministic endpoint
- Tab/nonce binding (GL-SEC-114) — partial tab/origin re-check only
- Fill lifecycle UAF risk (GL-SEC-115) — deferred but nested modal remains
- Migration/rotation snapshot atomicity (GL-SEC-002, 103)
- Note integrity signaling (GL-SEC-111)
- Export reliability (GL-SEC-110), attachment quotas (GL-SEC-109)
- PRAGMA readback (GL-SEC-008), libsodium immutable pin (GL-SEC-011)
- Path hard-link detection, post-backup verify (GL-SEC-105)
- Self-destruct bridge stop (GL-ANALYZE-205)

### Regressions

- None identified relative to audit-3 remediation intent. Audit-3 documented bugs in bridge lifecycle and origin policy are resolved in source.

---

## Recommended Next Steps

### Security fixes (priority order)

1. Harden bridge token delivery and storage (GL-ANALYZE-201, GL-SEC-112) — permissions, Base64 transport (GL-ANALYZE-207).
2. Bind fill requests with stored/consumed nonce (GL-ANALYZE-203, GL-SEC-114).
3. Fix fill confirmation lifetime — survive lock without UAF (GL-ANALYZE-204, GL-SEC-115).
4. Stop bridge on self-destruct path (GL-ANALYZE-205).
5. Begin migration/rotation reads inside `BEGIN IMMEDIATE` or snapshot DB (GL-ANALYZE-208).
6. Extend integrity errors to notes (GL-SEC-111, GL-FEAT-303).

### Bug fixes (priority order)

1. Staged new-vault creation (GL-ANALYZE-221, GL-SEC-104).
2. Restore success vs `.bak` cleanup semantics (GL-ANALYZE-220).
3. Transactional note create and attachment duplicate (GL-ANALYZE-222, 223).
4. Honest bulk export results (GL-ANALYZE-224, GL-SEC-110).
5. Preview attachment cache (GL-ANALYZE-226, GL-SEC-109).

### Features (priority order)

1. Note integrity parity (GL-FEAT-303).
2. CI security regression suite (GL-FEAT-305).
3. Vault health dashboard (GL-FEAT-304).
4. TOTP fields (GL-FEAT-301) after bridge hardening.

### Tests (priority order)

1. `CredentialBridgeServer` integration — token reject, lock-during-fill, limits.
2. Rotation/migration failure injection with in-memory SQLite.
3. GRIMBKUP2 round-trip and restore rollback.
4. Origin matcher edge-case expansion.
5. Note/credential integrity adversarial cases.

---

## Optional Implementation Prompt

```text
You are performing a post-analyze1 remediation pass on GrimLedger (Qt 6 / C++20
encrypted notes + credential vault + Chrome/Edge native-messaging bridge).

Read auto_analyze1.md, SECURITY_AUDIT3.md, and every cited file before editing.

Product constraints:
- GL-SEC-001 self-destruct remains an accepted opt-in product risk.
- Master key must never reach native host, extension, or web page.
- Do not weaken Argon2id, AEAD AAD binding, verifier fail-closed, or size limits.
- Never log passwords, keys, or decrypted content.

Priority 1 — Bridge trust boundary (GL-ANALYZE-201..207, GL-SEC-112..115):
- Restrict session.token file permissions; Base64-encode token in JSON transport.
- Store and validate fill nonce in PendingFill; echo in response; consume once.
- Move pending fill state to a QObject that outlives server teardown; cancel on
  stopBridge(); avoid UAF when lock fires during confirmation dialog.
- Call stopBridge() from destroyVaultAfterFailedAttempts in App.cpp.
- Consider per-launch random component in endpoint name.

Priority 2 — Data integrity and atomicity (GL-ANALYZE-208, GL-SEC-111):
- Add DecryptResult to NoteRepository; block edit/save on integrity error.
- BEGIN IMMEDIATE before migration/rotation scans OR snapshot via sqlite3_backup.
- expectChanges on storeVaultInfo salt update during rotation.

Priority 3 — Reliability (GL-ANALYZE-220..224):
- Staged new-vault create (temp file + atomic rename).
- exportAllMarkdown returns per-file results; UI reports failures/collisions.
- DbTransaction around note create and attachment duplicate.
- Treat post-restore .bak removal as warning not failure.

Priority 4 — Tests and docs:
- Add test_bridge_server (token reject, lock-during-fill, line size limit).
- Add test_vault_rotation_migration with failure injection.
- Update README.md and SECURITY.md for credentials, bridge, Qt Network, tests/.

Run all tests after changes. Mark each GL-ANALYZE and GL-SEC item fixed,
partial, or deferred with file:line and test references.
```
