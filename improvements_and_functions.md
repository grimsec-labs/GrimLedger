# GrimLedger: Possible Improvements and New Functions

This document is a planning list only. It does not change the application code.

## Recommended Priorities

1. Finish security and data-integrity work before expanding browser fill.
2. Add automated tests for destructive and cryptographic workflows.
3. Improve credential handling and backup reliability.
4. Polish core note-management workflows.
5. Add larger optional features only after the foundations are stable.

## Security Improvements

- **Authenticated browser bridge**
  - Give each application session a random bridge token.
  - Reject clients that cannot prove they are the registered native host.
  - Use a user-specific, unpredictable local endpoint.

- **Exact origin matching**
  - Match scheme, hostname, and effective port.
  - Never offer an HTTPS credential to an HTTP page.
  - Make subdomain sharing an explicit credential option.

- **Fill request binding**
  - Bind approval to one browser tab, document, frame, origin, and request nonce.
  - Cancel filling if the page navigates or the active tab changes.

- **Lock-safe bridge requests**
  - Cancel pending browser requests when the vault locks.
  - Re-check lock state immediately before releasing a password.
  - Prevent bridge objects from being destroyed during active callbacks.

- **Reduced plaintext lifetime**
  - Load credential passwords only when selected or approved for filling.
  - Keep list and search results limited to safe summary fields.
  - Clear editors, caches, selections, and pending requests on lock.

- **Explicit integrity errors**
  - Distinguish valid empty fields from failed decryption.
  - Prevent corrupted notes or credentials from being edited and overwritten.
  - Offer a read-only recovery/error screen for damaged records.

- **Improved clipboard protection**
  - Make the clear delay configurable.
  - Add a manual "Clear Clipboard Now" action.
  - Warn that operating-system clipboard history may retain copied secrets.

- **Uniform password generation**
  - Remove modulo bias from character selection.
  - Support configurable length and character policies.
  - Offer passphrase generation using random words.

- **Optional biometric unlock**
  - Integrate with Windows Hello, Touch ID, or platform key stores.
  - Store only a wrapped vault key, never the master password.
  - Keep the master password available as a recovery unlock method.

- **Sensitive-screen protection**
  - Automatically hide passwords when the application loses focus.
  - Add an optional screenshot-blocking mode where supported by the OS.
  - Add a privacy overlay after a configurable idle period.

## Reliability and Data-Integrity Improvements

- **Fully atomic password rotation**
  - Scan and update records inside one verified transaction.
  - Check that every expected row was updated.
  - Leave the old password fully usable after any failure.

- **Transactional credential creation**
  - Avoid temporary rows encrypted for credential ID zero.
  - Ensure failed creation leaves no incomplete or unreadable record.

- **Transactional note duplication**
  - Duplicate the note, attachments, and attachment URLs together.
  - Roll everything back if any attachment cannot be copied.

- **Safer restore**
  - Validate every required table, column, version, and foreign key.
  - Preserve a recoverable original until the restored vault passes all checks.
  - Report the exact location of any recovery copy.

- **Safer vault reset**
  - Create and validate the replacement vault before removing the old one.
  - Require a verified backup or an additional confirmation phrase.

- **Reliable backups**
  - Use SQLite's backup API for a consistent snapshot.
  - Reopen and verify a completed backup before reporting success.
  - Detect hard links and other aliases to the live vault.

- **Backup history**
  - Support a configurable rotating set of encrypted backups.
  - Display creation date, vault version, size, and verification status.
  - Allow users to test a backup without restoring it.

- **Recovery diagnostics**
  - Add a read-only vault health check.
  - Check database integrity, schema, metadata, verifier, and encrypted records.
  - Never include plaintext or secrets in diagnostic output.

- **Reliable plaintext export**
  - Use atomic file writes.
  - Prevent duplicate titles from overwriting each other.
  - Export referenced images and report partial failures.

- **Storage quotas**
  - Add per-image, per-note, and total-vault limits.
  - Warn before the vault becomes too large for supported backup operations.

## Credential Vault Functions

- **Credential summary model**
  - List only label, username, URL, and modification date.
  - Decrypt passwords and private notes only on demand.

- **Credential folders and tags**
  - Group credentials by work, personal, finance, development, and other labels.
  - Support favorites and recently used credentials.

- **Credential types**
  - Support login, secure note, API key, software license, identity, payment card,
    Wi-Fi password, and database connection templates.

- **Custom fields**
  - Let users add encrypted key/value fields.
  - Mark individual fields as concealed, searchable, or copyable.

- **Password history**
  - Retain a limited encrypted history of previous passwords.
  - Require an explicit user action to reveal old values.

- **Password-health dashboard**
  - Detect reused, empty, short, or old passwords locally.
  - Never send passwords or hashes to an external service.

- **Expiration reminders**
  - Add optional password-expiry dates and local reminders.
  - Display credentials that are due for rotation.

- **TOTP support**
  - Store TOTP seeds as encrypted fields.
  - Generate time-based codes locally.
  - Require an additional reveal action before displaying or copying the seed.

- **Credential import**
  - Import common CSV formats through a preview and field-mapping screen.
  - Validate all rows before committing the import.
  - Clearly warn that source CSV files remain plaintext.

- **Credential export**
  - Default to an encrypted portable archive.
  - Require re-authentication and explicit confirmation for plaintext export.

- **Per-credential browser policy**
  - Allow exact origins, optional subdomains, disabled browser fill, or
    confirmation-every-time.
  - Show the saved policy beside the credential URL.

- **Credential usage history**
  - Record local non-secret events such as "filled on example.com at 14:20."
  - Do not record usernames, passwords, or page contents.

## Browser Extension Functions

- **Reliable bridge status**
  - Show disconnected, locked, unlocked, awaiting approval, denied, and timed-out
    states distinctly.

- **Safe field preview**
  - Highlight the fields that will be filled before requesting the password.
  - Let the user choose among multiple login forms.

- **Username-only fill**
  - Allow filling a username without requesting or exposing the password.

- **Copy without fill**
  - Provide optional copy actions with the same desktop confirmation policy.

- **Iframe and shadow-DOM support**
  - Support embedded login forms without widening origin authorization.
  - Treat each frame origin independently.

- **Framework-compatible input**
  - Support React, Vue, Angular, and other controlled input implementations.
  - Verify that the page accepted the value before reporting success.

- **Browser-specific installation**
  - Support separate Chrome and Edge extension IDs.
  - Add install, repair, status, and uninstall commands.

- **Stable native-host installation**
  - Install the manifest and executable in a stable per-user application folder.
  - Avoid registry entries that point to temporary or build directories.

## Note Management Functions

- **Note history**
  - Store encrypted revisions with timestamps.
  - Allow preview, comparison, and restoration of an older revision.

- **Trash and recovery**
  - Move deleted notes to an encrypted trash folder.
  - Permanently remove them only after a retention period or explicit action.

- **Pinned notes**
  - Pin important notes above normal sorting.

- **Nested folders**
  - Complete parent/child folder navigation and drag-and-drop organization.

- **Saved searches**
  - Save frequently used search and filter combinations.

- **Advanced filters**
  - Filter by folder, tag, favorite, creation date, modified date, attachment,
    and content type.

- **Backlinks**
  - Support links between notes and show incoming references.
  - Add an orphan-note view.

- **Wiki-style links**
  - Recognize `[[Note Title]]` and provide completion suggestions.

- **Templates**
  - Create reusable encrypted templates for journals, meeting notes, research,
    incident reports, and checklists.

- **Daily notes**
  - Create or open a date-based note from a shortcut.

- **Table of contents**
  - Generate navigation from Markdown headings.

- **Outline panel**
  - Display headings and allow quick movement through long notes.

- **Improved Markdown support**
  - Add tables, footnotes, strikethrough, task-list interaction, and fenced-code
    options while keeping raw HTML disabled or sanitized.

- **Attachment manager**
  - List all note attachments, sizes, and references.
  - Find and safely remove orphaned attachments.

- **Attachment types**
  - Optionally support encrypted PDFs and text files with strict size limits.
  - Open them only through controlled temporary-file handling.

- **Image optimization**
  - Offer safe resize and compression settings before encryption.
  - Cache unchanged preview images instead of decrypting them repeatedly.

- **Conflict-safe editing**
  - Detect when the same record changed after it was loaded.
  - Offer compare, overwrite, or save-as-copy choices.

## Search Improvements

- **Indexed encrypted search**
  - Build an encrypted or session-only search index to avoid repeatedly
    decrypting every note.

- **Search operators**
  - Support queries such as `tag:security`, `folder:work`, `is:favorite`,
    `before:2026-01-01`, and quoted phrases.

- **Search within current note**
  - Add find, replace, case sensitivity, and regular-expression options.

- **Credential-safe search**
  - Search credential summaries without decrypting password fields.
  - Make searching private notes an explicit option.

- **Recent searches**
  - Store recent queries only while unlocked, with an option to disable them.

## User Experience Improvements

- **First-run security guide**
  - Explain the master password, backups, exports, auto-lock, clipboard history,
    and the browser bridge threat model.

- **Persistent settings**
  - Save auto-lock, editor layout, word wrap, line numbers, theme, and preferred
    sorting.

- **Application-wide idle tracking**
  - Count activity in child widgets and dialogs.
  - Optionally lock when the workstation locks or the system sleeps.

- **Lock confirmation for unsaved data**
  - If saving fails, let the user retry, discard, or cancel locking.

- **Keyboard navigation**
  - Add shortcuts for new note, new credential, search, lock, preview mode,
    password reveal, and copy actions.

- **Command palette**
  - Provide searchable access to actions without exposing secret values.

- **Accessibility**
  - Improve screen-reader labels, keyboard focus order, contrast, scalable text,
    and reduced-motion behavior.

- **Clear operation results**
  - Report partial success for imports, exports, backup, restore, and deletion.
  - Include non-secret details about which records failed.

- **Undo support**
  - Support undo for note moves, tag changes, and non-destructive deletion.

## Privacy and Platform Functions

- **Portable mode**
  - Support an explicitly selected local data directory.
  - Clearly communicate the security implications of removable storage.

- **Multiple vaults**
  - Open separate vault files with independent passwords and settings.
  - Keep keys and caches isolated between vaults.

- **Profile separation**
  - Allow work and personal profiles without sharing recent items or bridge
    permissions.

- **OS lock integration**
  - Lock GrimLedger when the operating system session locks.

- **Crash-safe privacy**
  - Disable sensitive crash dumps where supported.
  - Ensure diagnostic files never contain decrypted content.

- **Secure temporary-file manager**
  - Centralize temporary exports and attachment viewing.
  - Track cleanup and warn when guaranteed erasure is not possible.

## Testing and Maintenance Improvements

- **Automated unit tests**
  - Cover cryptography wrappers, AAD identities, URL matching, password
    generation, path safety, and backup header parsing.

- **Repository tests**
  - Cover valid empty fields, corruption, wrong keys, missing rows, and exact
    update counts.

- **Failure-injection tests**
  - Interrupt rotation, migration, backup, restore, duplication, and credential
    creation at every important step.

- **Browser integration tests**
  - Test navigation races, tab switches, frames, redirects, lock during approval,
    fake clients, oversized messages, and disconnects.

- **Sanitizer builds**
  - Add AddressSanitizer and UndefinedBehaviorSanitizer configurations where
    supported.

- **Static analysis**
  - Add compiler warnings, clang-tidy, and source security checks.

- **Dependency verification**
  - Pin all fetched dependencies immutably.
  - Document a regular dependency and vulnerability review process.

- **Migration tests**
  - Keep sample vaults from each supported schema and crypto format.
  - Verify forward migration and recovery from interrupted migration.

- **Release checklist**
  - Require backup/restore, rotation, lock lifecycle, export, and browser bridge
    tests before release.

## Optional Advanced Functions

- **Encrypted local API**
  - Provide a disabled-by-default automation API with scoped capabilities and
    explicit per-client approval.

- **Hardware-backed key wrapping**
  - Optionally wrap the derived vault key using TPM, Secure Enclave, or platform
    key storage.

- **Encrypted synchronization**
  - Synchronize only already-encrypted vault data.
  - Design conflict handling and key management before implementation.

- **Shared vaults**
  - Use per-user public-key encryption and signed changes.
  - Treat this as a separate security architecture, not a small extension of the
    single-user master-password model.

- **Plugin system**
  - Keep plugins disabled by default and isolated from decrypted content.
  - Require explicit capabilities for notes, credentials, files, and network.

## Suggested Release Sequence

### Release 1: Security Foundation

- Authenticated and bounded browser bridge
- Exact origin policy
- Lock-safe fill lifecycle
- Credential summary model and lock cleanup
- Explicit decryption errors
- Automated security tests

### Release 2: Data Reliability

- Atomic rotation and credential creation
- Full restore validation and rollback
- Verified SQLite backups
- Reliable exports
- Transactional note duplication
- Attachment quotas

### Release 3: Core Productivity

- Trash and note history
- Templates and daily notes
- Advanced search and saved filters
- Backlinks and wiki links
- Attachment manager
- Persistent settings and keyboard navigation

### Release 4: Credential Productivity

- Custom fields and credential types
- Password-health dashboard
- TOTP
- Secure import/export
- Per-credential browser policy

### Release 5: Optional Platform Features

- Biometric unlock
- Multiple vaults
- OS lock integration
- Portable mode
- Hardware-backed key wrapping
