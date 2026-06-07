# GrimLedger Third Security & Reliability Audit

**Audit date:** 2026-06-07  
**Scope:** Core cryptography and session handling, SQLite repositories, password
rotation, backup and restore, destructive workflows, notes and attachments,
credential storage, clipboard and password generation, the local credential
bridge, native messaging host, Chrome/Edge extension, build inputs, and security
documentation.  
**Method:** Read-only static review. No application code changed. This report is
the only file created. No build, test, install, git, package-manager, network, or
runtime browser operation was performed.

## Executive Summary

GrimLedger's posture is **mixed compared with audit 2**. Several important fixes
are present: backup creation now verifies the current master password, backup
header parsing no longer performs unaligned reads, transaction startup and
crypto-format marker failures are checked, password rotation includes
credentials, attachment duplication re-encrypts under the new attachment
identity, restore locks the old session, and destructive deletion failures are
reported more honestly.

The new credential bridge materially expands the attack surface. No new
Critical issue was statically verified, but four new High-severity bridge issues
need attention before the browser integration should be trusted with important
credentials:

1. Any local process able to reach the predictable local socket can impersonate
   the native host, submit a caller-chosen origin, enumerate matching account
   metadata, and request plaintext credentials through a misleading browser-fill
   confirmation.
2. Origin matching ignores scheme and port and automatically trusts all
   subdomains. An HTTPS credential can therefore be offered to HTTP, and a root
   domain credential can be offered to an attacker-controlled subdomain.
3. The extension authorizes a fill for one origin, then re-queries the active tab
   and sends the returned password without verifying that the tab and origin are
   unchanged.
4. Fill confirmation runs a nested event loop. Auto-lock can destroy the bridge
   while its request handler is still executing, creating a use-after-free/crash
   path and a possible response after the vault has locked.

Workflow conclusions:

- **Password rotation:** Improved and includes credential fields, but still not
  fully reliable. The database is scanned before the write transaction begins,
  update row counts are not checked, and plaintext/ciphertext copies remain in
  ordinary Qt containers.
- **Restore:** The stale session-key issue is substantially fixed by locking
  after success. Full schema validation and rollback error handling remain
  incomplete, and credential plaintext/caches are not cleared.
- **Backups:** GRIMBKUP2 now verifies the entered password and uses checked
  `QSaveFile` writes. Path alias protection and post-write validation remain
  partial, and the live database is copied as a raw file rather than through the
  SQLite backup API.
- **Credentials:** Fields are individually encrypted with record-and-field AEAD
  binding and are included in password rotation. Reads silently turn
  authentication failure into empty strings, creation is a non-transactional
  insert/update sequence, and the UI decrypts and caches every password for list
  display.
- **Browser bridge:** The master key is not sent to the native host or extension,
  which is good. The surrounding client authentication, origin authorization,
  fill lifecycle, limits, and post-lock behavior are not yet strong enough for a
  password-manager trust boundary.

## Prior Finding Re-Verification Table

| ID | Prior status (audit 2) | Current verified status | Evidence (file:line) | Notes |
|---|---|---|---|---|
| GL-SEC-001 | Accepted risk | **Accepted risk** | `src/App.cpp:134-219`; `src/utils/AppSettings.cpp:11-33`; `src/ui/SettingsWindow.cpp:75-87` | The opt-in unauthenticated three-failure trigger remains by product decision. Delete failure is now checked and physical erasure is no longer promised. |
| GL-SEC-002 | Partially fixed | **Partially fixed** | `src/storage/VaultRepository.cpp:596-642,746-829`; `src/storage/DbTransaction.h:13-15` | Rotation now checks query completion and transaction startup and includes credentials. Reads still occur before `BEGIN IMMEDIATE`, and successful `UPDATE` steps are not checked with `sqlite3_changes()`. |
| GL-SEC-003 | Partially fixed | **Partially fixed** | `src/storage/VaultRepository.cpp:927-943,1226-1272`; `src/ui/MainWindow.cpp:978-1026` | Restore is staged and the old session is locked after success. Required schema is not fully validated and rollback/remove results remain unchecked. |
| GL-SEC-004 | Partially fixed | **Verified fixed** | `src/storage/VaultRepository.cpp:835-895`; `src/ui/MainWindow.cpp:918-945` | GRIMBKUP2 creation now verifies the supplied password against the live vault before writing, so the envelope and inner vault use the same password. |
| GL-SEC-005 | Verified fixed | **Verified fixed** | `src/security/CryptoManager.cpp:36-64`; `src/ui/LoginWindow.cpp:163-166`; `src/App.cpp:79-84,123-127` | Login fields and the password derivation UTF-8 buffer are cleared after successful use. Full Qt plaintext scrubbing is still not guaranteed. |
| GL-SEC-006 | Partially fixed | **Partially fixed** | `src/security/CryptoManager.cpp:224-245`; `src/storage/VaultRepository.cpp:413-559`; `src/storage/AttachmentRepository.cpp:158-190` | Notes, attachments, verifier, backups, and credentials use domain-bound AEAD. Attachment-copy identity is fixed, but migration atomicity and row-count verification remain incomplete. |
| GL-SEC-007 | Verified fixed | **Verified fixed** | `src/security/CryptoManager.cpp:14-27`; `src/storage/VaultRepository.cpp:325-410` | KDF/salt bounds, atomic creation, verifier checks, and fail-closed missing verification metadata remain present. |
| GL-SEC-008 | Partially fixed | **Partially fixed** | `src/storage/Database.cpp:19-40`; `SECURITY.md:13-25` | Security PRAGMAs are set and failures to execute them close the DB. Their effective values are not queried and verified. |
| GL-SEC-009 | Verified fixed | **Verified fixed** | `src/ui/MarkdownPreview.cpp:13-23`; `src/markdown/MarkdownRenderer.cpp:82-94` | Automatic external handlers remain disabled and clicked schemes are allowlisted to HTTP, HTTPS, and mailto. |
| GL-SEC-010 | Partially fixed | **Partially fixed** | `src/utils/SecurityLimits.h:5-9`; `src/security/CryptoManager.cpp:80-140,176-221`; `src/storage/AttachmentRepository.cpp:56-64` | Import/backup limits exist, but unchecked size-to-`int` conversions and repository-level aggregate attachment limits remain. |
| GL-SEC-011 | Partially fixed | **Partially fixed** | `CMakeLists.txt:12-18,48-55` | SQLite has a SHA-256 archive hash. Libsodium still uses a tag rather than an immutable commit or hashed archive, and no local vulnerability-monitoring configuration was found. |
| GL-SEC-012 | Verified fixed | **Verified fixed** | `README.md:25-45`; `SECURITY.md:3-11`; `src/security/CryptoManager.h:15-21` | Current XChaCha20-Poly1305 and legacy XSalsa20-Poly1305 are named accurately. Other backup and bridge documentation is stale, but the algorithm claim remains fixed. |
| GL-SEC-101 | New/open (High) | **Partially fixed** | `src/ui/MainWindow.cpp:1017-1026`; `src/ui/MainWindow.h:142-145` | Restore now closes the note editor, clears note cache/preview, and locks the old key. Credential cache/editor state is not cleared, and the bridge is not explicitly stopped on this direct lock path. |
| GL-SEC-102 | New/open (High) | **Verified fixed** | `src/storage/VaultRepository.cpp:835-852`; `src/ui/MainWindow.cpp:918-945` | A wrong backup password is rejected before output is opened. |
| GL-SEC-103 | New/open (High) | **Partially fixed** | `src/storage/VaultRepository.cpp:168-182,424-559,596-829` | Query prepare/final-step checks, transaction startup, and marker writes are now checked. Scans remain outside the transaction snapshot, and expected update counts are not verified. |
| GL-SEC-104 | New/open (High) | **Partially fixed** | `src/App.cpp:87-127,160-219` | Delete failures are checked and self-destruct reporting is truthful. New-vault reset still deletes the only live vault before a replacement database and unlock have succeeded. |
| GL-SEC-105 | New/open (High) | **Partially fixed** | `src/utils/PathSafety.h:8-33`; `src/ui/MainWindow.cpp:929-945`; `src/storage/VaultRepository.cpp:849-895` | The UI rejects canonical/absolute matches to live and restore files. Hard-link identity is not detected, the repository API does not enforce the rule, and completed output is not reopened and verified. |
| GL-SEC-106 | New/open (Medium) | **Partially fixed** | `src/storage/AttachmentRepository.cpp:110-193`; `src/storage/NoteRepository.cpp:255-263` | Copied attachments are decrypted with old AAD and re-encrypted with new AAD. Note creation, all attachment copies, and body remapping are still not one transaction, so partial copies remain possible. |
| GL-SEC-107 | New/open (Medium) | **Not fixed** | `src/storage/VaultRepository.cpp:927-943,1226-1272` | Validation still checks only `quick_check` plus one metadata row. Rollback rename/remove failures can still leave an ambiguous or missing live path. |
| GL-SEC-108 | New/open (Medium) | **Verified fixed** | `src/storage/VaultRepository.cpp:145-165` | Backup integers are read with `memcpy` into aligned objects before endian conversion. |
| GL-SEC-109 | New/open (Medium) | **Not fixed** | `src/utils/ImageSanitizer.h:12-19`; `src/utils/ImageSanitizer.cpp:32-71`; `src/ui/MainWindow.cpp:1098-1115` | The 8192-by-8192 decoded image ceiling, repeated decrypt/base64 preview work, and lack of aggregate attachment quotas remain. |
| GL-SEC-110 | New/open (Medium) | **Not fixed** | `src/storage/NoteRepository.cpp:413-440`; `src/storage/AttachmentRepository.cpp:207-245`; `src/ui/MainWindow.cpp:1047-1091` | Plaintext writes, attachment writes, duplicate filenames, and bulk-export failures remain unchecked or silently ignored. |
| GL-SEC-111 | New/open (Medium) | **Not fixed** | `src/storage/NoteRepository.cpp:28-40,133-168`; `src/storage/CredentialRepository.cpp:21-72` | Note authentication failure still becomes editable empty text. The new credential repository repeats the same ambiguity for every credential field. |

## New Findings

### GL-SEC-112: The local bridge has no authenticated client identity

**Severity:** High  
**Category:** Security  
**Locations:** `src/bridge/CredentialBridgeServer.cpp:40-66,77-83,115-203`;
`tools/grimledger_host/main.cpp:85-115`; `browser-extension/background.js:45-63`

**Description:** The desktop app listens on the fixed name
`grimledger-bridge`, accepts every local connection, and has no challenge,
per-launch capability, peer validation, or authenticated handshake. The native
host is only a transparent JSON forwarder. The desktop trusts the request's
caller-supplied `origin`; it has no evidence that the request came from the
installed extension or from the browser tab named in that origin.

`list_matches` silently returns credential IDs, labels, and usernames.
`fill` returns a plaintext username and password after a confirmation dialog.
The dialog says "Allow browser extension" even when the requester is a direct
local socket client. A local process can also bind or impersonate the predictable
server while GrimLedger is not listening and receive extension requests.

Qt's exact local-socket ACL behavior differs by platform and was not runtime
verified. The code itself does not request a restrictive socket option or add
application-layer authentication. At minimum, same-user local processes should
be treated as able to attempt this connection.

**Risk / impact:** An untrusted local process can enumerate account metadata,
spoof origins, create repeated credential prompts, and socially engineer a user
into releasing a password to a non-browser client. A socket impersonator can
also inject arbitrary fill responses into the extension. This does not expose
the master key, but it breaks the intended native-host/extension trust boundary.

**Recommended remediation:**

- Create a high-entropy per-launch bridge capability and require it on every
  request, delivered only through a protected native-host bootstrap channel.
- Use the most restrictive supported `QLocalServer` access option and a
  user-specific, unpredictable endpoint.
- Bind requests to an authenticated extension/native-host session rather than
  accepting an origin string as proof.
- Make missing confirmation handlers fail closed.
- Avoid returning account metadata before client authentication.

**Acceptance criteria:**

- A same-user process that knows only the old fixed socket name cannot call
  `ping`, enumerate matches, trigger a fill prompt, or obtain a credential.
- A fake local server cannot produce a fill accepted by the extension.
- Every non-status request is tied to an authenticated, short-lived session.
- The master key never crosses the desktop-process boundary.

### GL-SEC-113: Origin matching crosses HTTPS, port, and subdomain boundaries

**Severity:** High  
**Category:** Security  
**Locations:** `src/bridge/OriginMatcher.cpp:11-36`;
`browser-extension/popup.js:20-25,40-58`

**Description:** Matching validates that the page uses HTTP or HTTPS but compares
only hostnames. It ignores the stored credential's scheme and both URLs' ports.
Consequently, a credential saved for `https://example.com` matches
`http://example.com`, and credentials scoped to one non-default port match every
port on that host.

The matcher also automatically accepts every subdomain with
`pageHost.endsWith('.' + credHost)`. A credential for `example.com` therefore
matches `anything.example.com`, including delegated or attacker-controlled
subdomains. There is no explicit user opt-in for this widening and no
registrable-domain/public-suffix policy.

QUrl's IDN normalization behavior was not runtime tested, so homograph/display
handling should also be covered by tests rather than assumed.

**Risk / impact:** Passwords can be offered and filled into a plaintext HTTP
page, the wrong service on another port, or an untrusted subdomain. The HTTP case
can expose the credential to network interception in addition to page scripts.

**Recommended remediation:**

- Default to exact normalized origin matching: scheme, canonical host, and
  effective port.
- Never downgrade an HTTPS credential to HTTP.
- Require an explicit credential setting for subdomain sharing and display the
  exact target origin in confirmation.
- Normalize and test punycode/Unicode hostnames and reject ambiguous URLs.

**Acceptance criteria:**

- `https://example.com` does not match `http://example.com`.
- Non-default ports are isolated unless explicitly configured.
- Root-domain credentials do not match subdomains by default.
- Tests cover trailing dots, case, userinfo, paths containing `@`, IPv4/IPv6,
  IDN/punycode, public suffixes, malformed URLs, and explicit subdomain opt-in.

### GL-SEC-114: Fill authorization is not bound to the tab that receives the password

**Severity:** High  
**Category:** Security  
**Locations:** `browser-extension/popup.js:20-25,46-59,67-76`

**Description:** The popup captures an origin when it lists matches and sends
that origin to the desktop for confirmation. After the desktop returns the
plaintext credential, `fillMatch()` performs a new active-tab query and sends
the password to whichever tab is active at that later moment. It does not retain
and compare the original tab ID, current URL, frame, or origin.

**Risk / impact:** A tab navigation or active-tab switch during confirmation can
send a credential approved for origin A to origin B. The confirmation text does
not authorize the actual receiving document, only the earlier caller-supplied
origin.

**Recommended remediation:**

- Capture the tab ID, frame ID, document identity, and normalized origin before
  requesting credentials.
- Immediately before delivery, query that same tab and require the origin and
  document to be unchanged.
- Pass an opaque request nonce through popup, native host, and desktop response
  and consume it once.
- Cancel on navigation, tab replacement, popup teardown, or any mismatch.

**Acceptance criteria:**

- Navigation, redirect, tab switch, tab close/reopen, and frame replacement
  during confirmation always cancel the fill.
- The password is delivered only to the exact tab/document/origin shown in the
  desktop confirmation.
- Automated extension tests exercise the race at each asynchronous boundary.

### GL-SEC-115: Auto-lock can destroy an in-flight bridge request

**Severity:** High  
**Category:** Security  
**Locations:** `src/bridge/CredentialBridgeServer.cpp:115-203`;
`src/ui/MainWindow.cpp:70-75,789-801,1171-1200`;
`src/ui/GrimMessageDialog.cpp:80-82`; `src/security/VaultSession.cpp:48-58`

**Description:** `handleRequest()` invokes the confirmation callback
synchronously. `DialogUtils::question()` enters a nested modal event loop. While
that loop is running, the auto-lock timer can emit `lockRequested`.
`onLockVault()` calls `stopBridge()`, which resets and destroys `m_bridge` while
one of its member functions is still on the stack.

The request handler also copies the session key and decrypted credential before
confirmation and does not re-check the unlocked state after confirmation. If
execution survives the object/socket lifetime problem, it can proceed to write
the password after the session has been locked. Exact Qt destruction behavior
for the accepted `QLocalSocket` was not runtime tested; the source-level
re-entrancy and missing post-confirmation check are nevertheless present.

**Risk / impact:** A fill near auto-lock expiry can crash the application, use a
dangling bridge/socket object, or release a password after the user believes the
vault is locked.

**Recommended remediation:**

- Make fill authorization asynchronous and keep the server object alive through
  an explicit request state object.
- Invalidate all pending request tokens on lock before clearing the key.
- Re-check authenticated client session, vault generation, unlocked state,
  target origin, and request nonce after user approval and immediately before
  constructing the response.
- Wipe per-request key and plaintext buffers on approval, denial, timeout,
  disconnect, and lock.

**Acceptance criteria:**

- Auto-lock/manual lock during a pending confirmation cancels the request and
  returns no credential.
- The bridge object and socket cannot be destroyed while callbacks still access
  them.
- Thread/address sanitizers and a deterministic lock-during-dialog test report
  no use-after-free.
- No response containing a password can be emitted after the lock generation
  changes.

### GL-SEC-116: Credential passwords are decrypted and retained far beyond need

**Severity:** Medium  
**Category:** Security  
**Locations:** `src/storage/CredentialRepository.cpp:57-94,231-248`;
`src/ui/MainWindow.cpp:463-505,789-801,1017-1026`;
`src/ui/MainWindow.h:142-145`; `src/models/Credential.h:6-14`

**Description:** `listCredentials()` decrypts all five fields for every row,
including passwords and notes, even though the list and bridge matching need
only label, username, and URL. `MainWindow` stores the resulting full
`QVector<Credential>` in `m_cachedCredentials`, while the selected credential is
also copied into editor widgets.

Normal lock saves and clears the key but does not clear the credential cache,
credential editor, selection, or search state. Successful restore clears note
state but not credential state. `App` hides and reuses the same `MainWindow`, so
these plaintext objects can remain allocated while the vault is locked.

**Risk / impact:** Locking removes the encryption key but leaves a complete
decrypted password inventory in ordinary Qt heap objects, increasing exposure
to crash dumps, memory inspection, use-after-free bugs, and accidental reuse
after restore. The documented general Qt memory limitation does not justify
decrypting every password for list rendering.

**Recommended remediation:**

- Introduce a summary query/type that decrypts only label, username, and URL.
- Load password and notes only for an explicitly selected credential or approved
  fill.
- On every lock/restore, clear editor fields, list models, caches, IDs, search
  strings, pending requests, and best-effort sensitive temporary buffers before
  clearing the key.
- Avoid copying the session key and full `Credential` values where scoped,
  move-only or callback-based access can be used.

**Acceptance criteria:**

- Listing/searching credentials does not decrypt password or notes fields.
- After lock or restore, no UI model or application cache contains credential
  plaintext.
- Unlock after restore cannot display credentials from the replaced vault.
- Memory-oriented tests verify cleanup hooks, while documentation remains honest
  that Qt cannot guarantee complete physical scrubbing.

### GL-SEC-117: Credential corruption is presented as valid empty data

**Severity:** Medium  
**Category:** Security  
**Locations:** `src/storage/CredentialRepository.cpp:21-72,77-120`;
`src/ui/MainWindow.cpp:491-545`

**Description:** Credential decryption returns an empty `QString` for both a
valid empty plaintext and an AEAD authentication failure. `rowToCredential()`
does not expose a field-level error, so list/get operations return an apparently
valid credential even when one or more fields are corrupt, swapped, truncated,
or encrypted under the wrong key. The UI can then save that object and replace
the failed field with encrypted empty text.

This is the credential equivalent of still-open GL-SEC-111, but it is newly
introduced across password, username, URL, label, and notes fields.

**Risk / impact:** Tampering and data corruption can be hidden, and a later save
can destroy forensic/recovery evidence by overwriting the bad ciphertext with
empty values.

**Recommended remediation:**

- Return an explicit result containing either a complete authenticated
  credential or a generic integrity error.
- Never display or save a partially decrypted credential.
- Keep valid empty plaintext distinguishable from authentication failure.

**Acceptance criteria:**

- Wrong key, wrong record ID, wrong field AAD, swapped fields, truncation, and
  bit flips produce a non-editable integrity error.
- Valid empty fields continue to load normally.
- A corrupted credential cannot be updated through the normal UI or bridge.

### GL-SEC-118: Credential creation and mutations can report success without the intended row change

**Severity:** Medium  
**Category:** Bug/Reliability  
**Locations:** `src/storage/CredentialRepository.cpp:122-229`;
`src/ui/MainWindow.cpp:548-595`

**Description:** Credential creation inserts fields encrypted for temporary
credential ID 0, then performs a separate update after obtaining the row ID.
There is no transaction around the two operations. If the update fails, cleanup
deletion is attempted but its result is ignored, potentially leaving a row that
cannot authenticate under its real ID.

Update and delete return true when `sqlite3_step()` is `SQLITE_DONE` even if no
row matched, because `sqlite3_changes()` is not checked. The UI ignores delete
failure and ignores save failure when switching credentials.

**Risk / impact:** Failed operations can leave undecryptable rows, silently lose
unsaved edits, or tell the surrounding workflow that a mutation completed when
the target row did not change. Such rows can later block password rotation.

**Recommended remediation:**

- Use one checked transaction for insert, row-ID-bound encryption, update, and
  cleanup, or allocate a stable random record identity before insertion.
- Check every bind/step result and exact expected row count.
- Propagate mutation failures to the UI and preserve unsaved fields.

**Acceptance criteria:**

- Failure injection at insert, row-ID encryption, update, and cleanup leaves no
  placeholder row.
- Updating/deleting a missing ID returns false.
- Switching, locking, or deleting cannot silently discard an unsaved credential.

### GL-SEC-119: The bridge permits unbounded local buffering and request churn

**Severity:** Medium  
**Category:** Security  
**Locations:** `src/bridge/CredentialBridgeServer.cpp:22-25,77-112,150-173`;
`tools/grimledger_host/main.cpp:85-115,124-142`

**Description:** The server accepts an unlimited number of clients and appends
each client's `readAll()` data until a newline appears, with no per-line,
per-client, total-buffer, idle, request-count, or rate limit. One client can
therefore grow memory indefinitely; many clients can multiply the effect.

Response and socket writes are not checked. `list_matches` can build an
unbounded response from all matching credentials, while the native host has no
desktop-response size cap and ignores native-message write failure.

**Risk / impact:** Any process able to reach the local endpoint can consume
memory, hold connections, flood modal confirmations, or make the bridge/native
host fail unpredictably.

**Recommended remediation:**

- Enforce small framed request/response limits and disconnect on overflow.
- Cap concurrent clients, requests per client, pending confirmations, matches,
  and request rate.
- Add idle and end-to-end deadlines and check all socket/native writes.
- Coalesce or reject fill prompts while one is pending.

**Acceptance criteria:**

- Oversized or newline-free input is disconnected before material memory growth.
- Client, pending-prompt, rate, and response-size limits are deterministic.
- Short writes, disconnects, and timeout races return bounded errors without
  retaining plaintext.

### GL-SEC-120: Lock/unlock lifecycle leaves the browser bridge permanently offline

**Severity:** Medium  
**Category:** Bug/Reliability  
**Locations:** `src/ui/MainWindow.cpp:51-80,789-801,1171-1192`;
`src/App.cpp:38-70`

**Description:** `MainWindow` starts the bridge only while its UI is first
constructed. Normal lock calls `stopBridge()` and destroys the bridge object.
`App::showLogin()` hides but retains the `MainWindow`; after the next unlock,
`showMain()` reuses it and never calls `startBridge()`. The return value from the
initial `m_bridge->start()` is also ignored.

Successful restore directly calls `m_session.lock()` rather than
`onLockVault()`, so it takes the opposite path: the old bridge listener is not
explicitly stopped, although its unlocked checker rejects protected actions.

**Risk / impact:** Browser fill works only until the first normal lock, with no
visible error in the desktop UI. Restore and normal lock have inconsistent
listener lifecycles, contradicting the extension security documentation.

**Recommended remediation:**

- Tie bridge start/stop to explicit session `unlocked`/`locked` state, not window
  construction.
- Stop and cancel it on every lock path, including restore and self-destruct.
- Restart only after a successful unlock and surface listen failures.

**Acceptance criteria:**

- Repeated unlock/lock/unlock cycles consistently start, stop, and restart one
  authenticated endpoint.
- Restore, self-destruct, application shutdown, and failed unlock leave no live
  credential endpoint.
- Listen failure is visible and does not leave a misleading "available" state.

### GL-SEC-121: Native-host installation depends on a temporary manifest and build artifact

**Severity:** Medium  
**Category:** Bug/Reliability  
**Locations:** `native-host/install-windows.ps1:8-22,31-50`;
`native-host/com.grimledger.bridge.json:1-9`

**Description:** The installer writes the registered native-messaging manifest
to the shared user temp directory and points both Chrome and Edge registry keys
to that file. Temp cleanup can remove it. The manifest points directly to a
binary under `build/` by default, so cleaning or moving the build breaks the
registration. The extension ID is required but not syntactically validated.

The same ID is registered for Chrome and Edge even though unpacked extension IDs
may differ between installations. Exact browser behavior for the PowerShell
UTF-8 output encoding was not runtime tested.

**Risk / impact:** A working installation can silently stop after temp cleanup
or build cleanup. Users may repeatedly reinstall or weaken local permissions
while troubleshooting. A same-user attacker can already change HKCU registration,
but a predictable temp manifest unnecessarily increases the fragility.

**Recommended remediation:**

- Install the host binary and generated manifest into a stable per-user
  application directory with restrictive permissions.
- Validate extension IDs and support separate browser registrations.
- Add uninstall, repair, versioning, and manifest JSON validation.

**Acceptance criteria:**

- Temp and build-directory cleanup do not break an installed bridge.
- Invalid IDs fail before registry modification.
- Chrome and Edge can be registered independently and removed cleanly.

### GL-SEC-122: Clipboard auto-clear retains a secret and cannot guarantee erasure

**Severity:** Low  
**Category:** Security  
**Locations:** `src/utils/ClipboardUtils.cpp:9-23`;
`src/ui/CredentialEditor.cpp:27-29`; `src/ui/MainWindow.cpp:608-629`

**Description:** The timer lambda captures the copied `QString` by value for 20
seconds, retaining another plaintext copy in the application heap. It clears the
clipboard only if its current text still equals the captured text, which is the
correct way to avoid deleting newer user content, but means "clipboard clears"
is conditional. OS clipboard history, cloud clipboard, and clipboard managers
can retain the value after GrimLedger clears the current slot. Repeated copies
create multiple retained timer captures.

**Risk / impact:** Users may interpret the UI as a guarantee of erasure when it
only performs a best-effort current-clipboard clear. Plaintext remains in both
the OS clipboard ecosystem and application memory for at least the timer period.

**Recommended remediation:**

- Describe the behavior as best-effort and warn about clipboard history.
- Minimize timer-captured copies and offer configurable immediate/manual clear.
- Prefer browser fill over clipboard for high-value secrets once the bridge is
  secured.

**Acceptance criteria:**

- UI and documentation do not promise guaranteed erasure.
- Multiple copy operations have deterministic ownership and do not clear newer
  unrelated clipboard content.
- Clipboard-disabled, unavailable, changed, and history-enabled scenarios are
  tested/documented.

### GL-SEC-123: Password generation has modulo bias

**Severity:** Low  
**Category:** Security  
**Locations:** `src/utils/PasswordGenerator.cpp:7-31`

**Description:** Random bytes from libsodium are reduced with `% alphabetSize`.
Because 256 is not an exact multiple of the alphabet length, some characters
are slightly more likely than others. The default 20-character output still has
substantial entropy, so this is not an emergency weakness, but the generator is
not uniform.

**Risk / impact:** Generated passwords have avoidable statistical bias and no
explicit guarantee of character-class coverage where a site requires it.

**Recommended remediation:**

- Use rejection sampling or libsodium's uniform bounded random function for
  character selection.
- Add optional policy generation without reducing entropy.

**Acceptance criteria:**

- Every alphabet index is sampled uniformly.
- Tests cover length bounds and required-character policies without deterministic
  or biased fallback behavior.

## Browser Bridge Threat Model

Trust boundaries:

1. The web page and its DOM receive the final username/password.
2. The extension content script and popup hold plaintext credential values.
3. The extension service worker crosses into the registered native host.
4. The native host forwards JSON over a named local socket.
5. The desktop bridge decrypts credentials and asks the user for approval.
6. Only the desktop app should ever hold the vault master key. Current code
   satisfies this specific boundary.

Attacker capabilities:

- **Malicious extension ID / compromised allowed extension:** Code executing as
  the registered allowed extension can submit arbitrary actions, IDs, and origin
  strings. Desktop confirmation currently limits silent password release, but
  metadata enumeration, spoofed confirmation context, and the post-confirmation
  tab race remain. A different extension ID is restricted by the native
  messaging manifest unless the registration is changed or the local socket is
  contacted directly.
- **Local malware or untrusted same-user process:** It can attempt direct socket
  access, spoof caller origin, enumerate metadata, flood prompts, or impersonate
  the desktop endpoint. Strong local malware may also read process memory, which
  is a documented non-goal, but the unauthenticated socket grants useful access
  without requiring memory compromise.
- **Malicious website:** Ordinary page JavaScript cannot directly call native
  messaging in the reviewed code. It can read credentials after they are placed
  in its DOM, influence field selection, navigate during asynchronous fill, and
  benefit from HTTP/subdomain/port overmatching. No page-origin proof reaches
  the desktop.
- **Physical access while unlocked:** The attacker can use the visible
  credential UI, copy/export data, approve fill prompts, and read notes. The
  bridge should not be considered an additional protection in this state.

## Feature Implementation Checklist

| Feature | Status | Evidence / notes |
|---|---|---|
| Credential vault encrypted storage | **Implemented with integrity gaps** | All five fields use `grim:cred:<id>:<field>` AAD (`src/security/CryptoManager.cpp:240-241`; `src/storage/CredentialRepository.cpp:11-35`). Authentication failures are converted to empty strings. |
| Credential rotation | **Implemented, partially reliable** | Credentials are scanned, decrypted, re-encrypted, and updated (`src/storage/VaultRepository.cpp:621-637,710-736,801-822`). Scan is outside the transaction and row counts are unchecked. |
| Clipboard auto-clear | **Best effort** | Conditional clear after 20 seconds (`src/utils/ClipboardUtils.cpp:9-23`); timer retains a plaintext copy and OS history is outside application control. |
| Password generator quality | **Cryptographic source, biased mapping** | `randombytes_buf` is used, but byte modulo alphabet size is biased (`src/utils/PasswordGenerator.cpp:21-30`). |
| Bridge lock gating | **Partial / lifecycle broken** | Protected actions check `isUnlocked()` (`src/bridge/CredentialBridgeServer.cpp:135-148`). Normal lock stops the bridge, but it is not restarted after unlock and restore does not explicitly stop it. |
| Origin matching | **Unsafe** | Scheme/port are ignored and all subdomains are trusted (`src/bridge/OriginMatcher.cpp:25-36`). IDN edge cases were not runtime verified. |
| User confirmation before fill | **Present but racy** | MainWindow installs a confirmation callback (`src/ui/MainWindow.cpp:1176-1199`). The server API fails open if no handler is configured, and lock can re-enter during the modal dialog. |
| Native host input framing | **Partial** | Native input is capped at 1 MiB (`tools/grimledger_host/main.cpp:54-70`). Desktop socket framing, response size, and write results are not bounded/checked. |
| Native host install script | **Fragile** | Registry setup exists, but it points to a temp manifest and usually a build artifact (`native-host/install-windows.ps1:8-50`). |
| Extension credential handling in memory | **Plaintext by design, insufficiently scoped** | Password crosses native response, popup, runtime message, content script, and DOM (`browser-extension/popup.js:67-75`; `browser-extension/content.js:24-42`). No explicit clearing or document binding exists. |
| Master key isolation | **Implemented** | The bridge copies and uses the key inside the desktop process; responses contain username/password only (`src/bridge/CredentialBridgeServer.cpp:144-148,199-203`). |
| Automated security/regression tests | **Absent** | No `tests/` directory or CTest/test target was found in the current tree/CMake file. |

## Non-Security Bug List

| Bug / reliability issue | Evidence / repro hint |
|---|---|
| Bridge never returns after a normal lock/unlock | Construct app, verify bridge, lock, unlock, then call `ping`; `startBridge()` runs only in the original constructor (`src/ui/MainWindow.cpp:51-80,789-801`; `src/App.cpp:57-65`). |
| Desktop ignores bridge listen failure | Occupy the endpoint before launch; `m_bridge->start()` return is discarded (`src/ui/MainWindow.cpp:1171-1184`). |
| Extension may report "Filled" when no field was filled | `content.js` returns `{ok:false}` but popup does not inspect the response object (`browser-extension/popup.js:70-76`; `browser-extension/content.js:24-50`). |
| Fill chooses the first password/text field | Forms with sign-up, current/new-password, hidden/decoy, search, or multi-account fields can receive the wrong values (`browser-extension/content.js:1-21`). |
| Framework-controlled inputs may reject direct `.value` writes | React/Vue-style native setters are not used; only basic input/change events are dispatched (`browser-extension/content.js:24-42`). |
| Iframes and shadow DOM are not supported | Manifest does not enable all frames and selectors use only the top document (`browser-extension/manifest.json:15-20`; `browser-extension/content.js:1-21`). |
| External desktop confirmation may close the browser popup | Browser action popups commonly close when focus moves to another application. This likely interrupts `fillMatch()`, but it was not runtime verified under Chrome/Edge. |
| Credential switch can discard unsaved edits | `saveCurrentCredential(false)` failure is ignored before loading the next row (`src/ui/MainWindow.cpp:548-553`). |
| Credential delete failure is hidden | Repository result is ignored and the list is reloaded (`src/ui/MainWindow.cpp:582-595`). |
| Lock can discard unsaved note/credential changes | Both save return values are ignored before clearing the key (`src/ui/MainWindow.cpp:789-801`). |
| Auto-lock misses most child/modal activity | The event filter is installed only on `MainWindow`, not `QApplication` or child widgets (`src/ui/MainWindow.cpp:306-323`). Some explicit editor events reset it, but dialog, list, and other activity may not. |
| Auto-lock settings are not persisted | Settings widgets default to enabled/15 minutes each process; only self-destruct uses `QSettings` (`src/ui/SettingsWindow.cpp:61-73`; `src/utils/AppSettings.cpp:11-22`). |
| Line-number setting is a no-op | `setLineNumbersVisible()` discards its argument (`src/ui/NoteEditor.cpp:154-157`). |
| Attachment duplication can leave a partial copied note | Each attachment is inserted independently and partial `idMap` results are returned (`src/storage/AttachmentRepository.cpp:110-193`). |
| Bulk export can silently omit/overwrite content | Duplicate sanitized titles collide, image-aware export is bypassed, and all per-file results are ignored (`src/storage/NoteRepository.cpp:426-440`; `src/ui/MainWindow.cpp:1084-1091`). |
| README backup table describes obsolete behavior | It says backup uses the vault key and restore must use the same unlocked session (`README.md:152-160`), unlike current GRIMBKUP2 password-derived behavior. |
| Extension README overstates bridge lifecycle and origin safety | It says the bridge stops on lock and requests require origin match without documenting restart failure or broad host-only matching (`browser-extension/README.md:20-24`). |

## Verification Performed

- Read `SECURITY_AUDIT.md` and `SECURITY_AUDIT2.md`, including all findings,
  status claims, acceptance criteria, and prior remediation prompt.
- Re-traced vault create, unlock, normal lock, auto-lock, destructive reset,
  self-destruct, legacy migration, password rotation, GRIMBKUP1/GRIMBKUP2
  creation, restore, staged installation, and rollback.
- Re-traced note and attachment CRUD, AEAD identities, attachment duplication,
  image sanitation/preview, import, single export, and bulk export.
- Traced credential create/read/update/delete/search and all five field AAD
  values, plus credential re-encryption during master-password rotation.
- Traced bridge `ping`, `lock_status`, `list_matches`, `fill`, confirmation, and
  lock/unlock lifecycle.
- Traced extension popup to service worker to native host to local socket to
  desktop and back to content-script DOM fill.
- Reviewed the Windows native-host manifest and installer.
- Scanned source for dynamic SQL, process execution, unrestricted URL opening,
  logging of secrets, unchecked file/socket writes, `readAll()` use, file
  remove/rename operations, SQLite step handling, and row-count checks.
- No user-controlled SQL injection path was found. The dynamic note-list SQL is
  assembled from enums, booleans, and integer IDs (`src/storage/NoteRepository.cpp:72-100`).
- No logging of master passwords, keys, note plaintext, credential passwords, or
  attachment plaintext was found in the reviewed source.
- No test directory or application test target was found.
- Confirmed before creation that `SECURITY_AUDIT3.md` did not exist.
- Limitations: no build, runtime test, browser test, socket ACL inspection,
  failure injection, sanitizer, static-analysis tool, binary inspection, or CVE
  network scan was performed. Runtime-dependent statements are marked as such.

## Comparison to Audit 2

Improvements since audit 2:

- GL-SEC-102 and GL-SEC-108 are fixed.
- Rotation/migration now detect failed query completion, check transaction
  startup, and check the crypto-format marker.
- Password rotation now covers credentials.
- Attachment duplication now preserves AEAD identity by decrypting and
  re-encrypting for the new UUID.
- Restore no longer continues with the old session key; it locks and requires a
  fresh unlock.
- Reset/self-destruct deletion failures are checked and physical-erasure wording
  is more accurate.

Regressions/new exposure:

- The credential bridge creates a new local IPC and browser trust boundary
  without authenticated clients or authoritative browser-origin binding.
- Origin policy is weaker than browser same-origin policy.
- Fill authorization and delivery are separated by an unchecked tab race.
- Modal confirmation and lock teardown can re-enter and destroy an in-flight
  bridge object.
- Credential listing decrypts and caches all passwords, expanding locked-state
  plaintext retention.
- Bridge lifecycle is functionally broken after the first normal lock.

Still open from audit 2:

- Rotation/migration snapshot atomicity and exact row-count checks.
- Staged new-vault reset, complete restore schema validation, and checked
  rollback operations.
- Hard-link/repository-level backup path enforcement and completed-backup
  verification.
- Transactional note/attachment duplication.
- Effective PRAGMA verification, checked integer narrowing, image/aggregate
  quotas, reliable plaintext export, and explicit decryption errors.
- Immutable libsodium pinning and automated regression/failure-injection tests.

## Recommended Next Steps

Security fixes, in priority order:

1. Disable browser password fill by default until GL-SEC-112 through
   GL-SEC-115 are fixed and covered by adversarial integration tests.
2. Authenticate the native-host/desktop session with a per-launch capability,
   restrictive local endpoint, bounded protocol, and fail-closed request state.
3. Replace hostname-only matching with exact normalized origin matching and
   explicit subdomain policy.
4. Bind confirmation to one tab/document/origin and cancel on navigation,
   active-tab change, disconnect, timeout, or lock.
5. Make pending fill asynchronous and lock-generation-aware; never destroy a
   live handler or emit a secret after lock.
6. Stop decrypting passwords for list/search. Clear all credential/editor/cache
   state on lock and restore.
7. Return explicit integrity errors for note and credential fields.
8. Finish rotation/migration atomicity, restore rollback safety, path identity,
   image quotas, and reliable export from the still-open audit-2 findings.

Bug/reliability fixes, in priority order:

1. Tie bridge start/stop to session state and surface listen failures.
2. Make credential creation/mutation transactional and propagate UI failures.
3. Replace the temp/build native-host installation with stable per-user
   installed files and separate browser registration.
4. Improve extension field selection, verify content-script responses, and add
   iframe/shadow-DOM/framework-aware behavior.
5. Install application-wide activity tracking, persist auto-lock settings, and
   prevent lock when a required save failed without explicit user choice.
6. Add a real test target before claiming these workflows are production-safe.

## Optional Cursor Remediation Prompt

```text
You are fixing SECURITY_AUDIT3.md in GrimLedger, a Qt 6 / C++20 encrypted note
and credential vault with a Chrome/Edge native-messaging bridge.

Read SECURITY_AUDIT3.md, SECURITY_AUDIT2.md, SECURITY.md, README.md, and every
file cited by GL-SEC-112 through GL-SEC-123 before editing.

Preserve these product constraints:
- GL-SEC-001 is an accepted opt-in product risk. Do not remove it.
- Never expose the master key to the native host, extension, or web page.
- Do not weaken Argon2id validation, AEAD AAD binding, verifier fail-closed
  behavior, backup-header authentication, or existing size limits.
- Never log passwords, keys, credential plaintext, notes, or attachments.

Priority 1: secure the bridge (GL-SEC-112 through GL-SEC-115, GL-SEC-119)
- Replace the predictable unauthenticated socket protocol with a restrictive,
  user-specific endpoint plus high-entropy per-launch authenticated session.
- Fail closed if authentication or the confirmation handler is absent.
- Add strict request/response, connection, rate, prompt, and timeout limits.
- Use exact normalized origin matching: scheme, host, effective port. Never
  downgrade HTTPS to HTTP. Make subdomain sharing explicit per credential.
- Bind each request to one tab ID, frame/document identity, origin, and one-time
  nonce. Revalidate immediately before delivery and cancel on any change.
- Make confirmation asynchronous and request-state based. Lock must cancel all
  pending requests without destroying an executing object. Re-check vault
  generation/unlocked state after approval and before returning plaintext.
- Check every local-socket and native-message write.

Priority 2: reduce credential plaintext and fix integrity signaling
- Add a credential summary type/query that never decrypts password or notes.
- Decrypt password only for selected-editor display or one approved fill.
- Clear credential editor, caches, list models, IDs, pending requests, and
  search state on every lock/restore before clearing the session key.
- Return explicit authenticated success/error results for every note and
  credential field. Never turn AEAD failure into editable empty text.
- Make credential create/update/delete transactional and check exact row counts.

Priority 3: complete remaining audit-2 workflow fixes
- Start BEGIN IMMEDIATE before rotation/migration scans and verify exact updated
  rows and marker/verifier changes.
- Stage and validate new-vault reset before replacing the old vault.
- Validate full restore schema, foreign keys, versions, metadata, integrity, and
  verifier; check every install/rollback remove/rename and preserve recovery.
- Enforce backup path identity in the repository, including hard links where
  supported; use SQLite backup API and reopen/verify completed backups.
- Make note plus attachment duplication one transaction.
- Add decoded-pixel and repository aggregate quotas.
- Use QSaveFile and detailed failure results for every plaintext export.

Priority 4: lifecycle and extension reliability
- Start/stop the bridge from session unlock/lock signals and surface failures.
- Install stable per-user native-host files; validate IDs; support independent
  Chrome/Edge install, repair, and uninstall.
- Verify content-script fill results; support correct field selection,
  framework-controlled inputs, frames, and shadow DOM without widening origin
  authority.
- Use application-wide activity tracking, persist auto-lock settings, and
  handle failed saves before lock/switch/delete.
- Replace password-generator modulo with uniform bounded sampling.
- Make clipboard wording explicitly best-effort.

Required tests:
- Direct unauthenticated socket clients and fake local servers are rejected.
- Oversize, newline-free, many-client, flood, disconnect, and short-write cases.
- HTTP/HTTPS, ports, subdomains, IDN/punycode, trailing dots, IPv4/IPv6, userinfo,
  malformed URL, and public-suffix origin cases.
- Navigation, redirect, tab switch, frame replacement, popup teardown, timeout,
  manual lock, and auto-lock at every fill boundary.
- No credential response after lock; no use-after-free under sanitizers.
- Listing/search does not decrypt passwords; lock/restore clears credential UI.
- Credential/note corrupted, swapped, truncated, wrong-AAD, wrong-key, and valid
  empty fields.
- Failure injection for credential CRUD, rotation/migration, backup, restore,
  rollback, duplication, and export.
- Repeated unlock/lock/unlock bridge lifecycle.

Run a clean build, all tests, sanitizers/static analysis, and browser integration
tests. Report each audit finding as fixed, partial, or deferred with exact code
and test references. Do not claim a security property without a passing test.
```
