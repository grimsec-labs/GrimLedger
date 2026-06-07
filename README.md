# GrimLedger

**GrimLedger** is a local-first, password-protected encrypted Markdown note manager with a dark infernal / hacker aesthetic. It is designed for programmers, cybersecurity students, and power users who want to store Markdown notes, code snippets, terminal commands, and research behind a single master password.

No cloud accounts. No online sync. Your vault stays on your machine.

---

## Main Features

- Master password login with vault creation
- Encrypted local SQLite vault (per-field encryption)
- Markdown editor with live preview (split / editor / preview modes)
- Syntax highlighting for 25+ languages in editor and preview
- Full-text search across titles, tags, and note bodies
- Folders, tags, favorites, and recent notes
- Import/export Markdown files
- Encrypted backup and archive export
- Auto-lock after inactivity + manual lock
- Change master password
- Customizable accent colors and editor settings

---

## Security Model

GrimLedger uses **libsodium** for all cryptographic operations. No custom cryptography is implemented.

| Layer | Algorithm |
|-------|-----------|
| Password hashing / KDF | **Argon2id** (`crypto_pwhash`) |
| Field encryption (current) | **XChaCha20-Poly1305 AEAD** (`crypto_aead_xchacha20poly1305_ietf`) with domain-bound associated data |
| Field encryption (legacy) | **XSalsa20-Poly1305** (`crypto_secretbox`) — migrated on unlock |
| Standalone backups (GRIMBKUP2) | **XChaCha20-Poly1305 AEAD** with password-derived key and header metadata |
| Random salts / nonces | `randombytes_buf` |

### How it works

1. On vault creation, a random 16-byte salt is generated and stored in `vault_metadata`.
2. The master password is derived into a 32-byte key using Argon2id with moderate cost parameters (validated on load).
3. Each note title, body, and attachment is encrypted with a unique nonce and domain-bound AEAD context (note id, field name, or attachment id).
4. Only encrypted BLOBs are written to disk — **no plaintext notes** are stored in the database file.
5. The derived key lives in `VaultSession` while unlocked and is zeroed on lock.
6. A verification token (`GRIMLEDGER_OK`) encrypted with the master key validates unlock attempts without storing the password.
7. **GRIMBKUP2** backups embed KDF metadata in the file header so a backup plus password can restore on a clean machine.

### Security-sensitive design rules

- UI code is separate from encryption logic
- Encryption logic is separate from database access
- Prepared SQL statements only (no string-concatenated queries)
- Notes are decrypted on demand, not all at once
- Passwords, keys, and decrypted note bodies are never logged
- Error messages avoid leaking internal crypto details

---

## Known Security Limitations

- **Forgotten master passwords cannot be recovered.** There is no backdoor, reset, or key escrow.
- Decrypted note content exists in RAM while editing. GrimLedger zeroes keys on lock, but full memory-scrubbing of all plaintext fragments is not guaranteed (Qt `QString` is immutable/shared).
- The SQLite database file itself is not SQLCipher-encrypted; security relies on per-field encryption of note content.
- **GRIMBKUP2** backups use the master password at backup time; legacy **GRIMBKUP1** backups require the same unlocked session key. Store backups securely.
- Optional **self-destruct after 3 failed logins** is an explicit anti-coercion feature — anyone with login access can trigger it. Back up your vault first.
- Exporting notes as `.md` writes **plaintext** to disk by design.
- Auto-lock uses application-level idle detection, not OS-level screen lock.
- No protection against local malware, keyloggers, or cold-boot attacks.
- Syntax highlighting uses regex grammars — not a full parser — and is unrelated to vault security.

---

## Dependencies

| Dependency | Purpose |
|------------|---------|
| **C++20** compiler | MSVC 2019+, GCC 10+, or Clang 12+ |
| **CMake** 3.21+ | Build system |
| **Qt 6** (Core, Gui, Widgets) | GUI framework |
| **libsodium** 1.0.20+ | Argon2id + XChaCha20-Poly1305 (fetched by CMake) |
| **SQLite** amalgamation | Local database (fetched by CMake) |

---

## Build Instructions

### Windows (MSVC + Qt 6)

```powershell
# Install Qt 6 via the Qt Online Installer (Widgets module).
# Ensure cmake and MSVC are available in PATH.

cd GrimLedger
cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2019_64"
cmake --build build --config Release

# Run
.\build\Release\GrimLedger.exe
```

### Linux

```bash
sudo apt install qt6-base-dev cmake build-essential

cd GrimLedger
cmake -B build
cmake --build build

./build/GrimLedger
```

### macOS

```bash
brew install qt cmake

cd GrimLedger
cmake -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build

./build/GrimLedger.app/Contents/MacOS/GrimLedger
```

---

## Creating a Vault

1. Launch GrimLedger.
2. On first run, the login screen prompts you to **Create Vault**.
3. Enter a master password (minimum 8 characters). The strength meter guides you.
4. Confirm the password and click **Create Vault**.
5. **Save your password somewhere safe.** It cannot be recovered if lost.

The vault file is stored at:

- **Windows:** `%APPDATA%\GrimLedger\vault.grim`
- **Linux:** `~/.local/share/GrimLedger/vault.grim`
- **macOS:** `~/Library/Application Support/GrimLedger/vault.grim`

---

## Unlocking a Vault

1. Launch GrimLedger.
2. Enter your master password at the `> enter master key:` prompt.
3. Click **Unlock Vault**.

After 15 minutes of inactivity (configurable in Settings), the vault auto-locks. Use **Lock Vault** in the sidebar or status bar to lock manually.

---

## Backup and Export

| Action | Result |
|--------|--------|
| **Backup Encrypted Vault** | GRIMBKUP2 standalone envelope (password-verified) → `.grimbak` file |
| **Restore Encrypted Backup** | Staged restore; unlock afterward with the backup password |
| **Export Selected Note** | Plaintext `.md` file |
| **Export All Notes** | Plaintext `.md` files in a folder |
| **Export Encrypted Archive** | Same as encrypted backup |

⚠ Exported Markdown files are **not encrypted**.

---

## Project Structure

```
GrimLedger/
  CMakeLists.txt
  README.md
  src/
    main.cpp, App.cpp
    ui/          — Qt widgets (login, main window, editor, preview, settings)
    security/    — CryptoManager, PasswordManager, VaultSession
    storage/     — Database, NoteRepository, VaultRepository
    models/      — Note, Folder, Tag
    search/      — SearchEngine
    markdown/    — MarkdownRenderer, SyntaxHighlighter
    utils/       — TimeUtils, SecureStringUtils
  resources/
    styles/grimledger_dark.qss
```

---

## License

This project is provided as-is for educational and personal use. Review the security limitations before storing highly sensitive data.

---

> ⚠ **Lost master passwords cannot be recovered.**
