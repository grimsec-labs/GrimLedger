<p align="center">
<pre>
          _____                    _____                    _____  
         /\    \                  /\    \                  /\    \ 
        /::\    \                /::\    \                /::\____\
       /::::\    \              /::::\    \              /:::/    /
      /::::::\    \            /::::::\    \            /:::/    / 
     /:::/\:::\    \          /:::/\:::\    \          /:::/    /  
    /:::/  \:::\    \        /:::/__\:::\    \        /:::/    /   
   /:::/    \:::\    \       \:::\   \:::\    \      /:::/    /    
  /:::/    / \:::\    \    ___\:::\   \:::\    \    /:::/    /     
 /:::/    /   \:::\ ___\  /\   \:::\   \:::\    \  /:::/    /      
/:::/____/  ___\:::|    |/::\   \:::\   \:::\____\/:::/____/       
\:::\    \ /\  /:::|____|\:::\   \:::\   \::/    /\:::\    \       
 \:::\    /::\ \::/    /  \:::\   \:::\   \/____/  \:::\    \      
  \:::\   \:::\ \/____/    \:::\   \:::\    \       \:::\    \     
   \:::\   \:::\____\       \:::\   \:::\____\       \:::\    \    
    \:::\  /:::/    /        \:::\  /:::/    /        \:::\    \   
     \:::\/:::/    /          \:::\/:::/    /          \:::\    \  
      \::::::/    /            \::::::/    /            \:::\    \ 
       \::::/    /              \::::/    /              \:::\____\
        \::/____/                \::/    /                \::/    /
                                  \/____/                  \/____/            


</pre>
</p>

# GrimLedger

**GrimLedger** is a local-first, password-protected encrypted Markdown note manager with a dark infernal / hacker aesthetic. It is designed for programmers, cybersecurity students, and power users who want to store Markdown notes, code snippets, terminal commands, and research behind a single master password.

Homepage: [github.com/grimsec-labs/GrimLedger](https://github.com/grimsec-labs/GrimLedger)

No cloud accounts. No online sync. Your vault stays on your machine.

---

## Main Features

- Master password login with vault creation
- Encrypted local SQLite vault (per-field encryption)
- Markdown editor with live preview (split / editor / preview modes)
- Syntax highlighting for 25+ languages in editor and preview
- Full-text search across titles, tags, and note bodies
- Folders, tags, favorites, and recent notes
- Encrypted credential vault with browser bridge (opt-in, disabled by default)
- Import/export Markdown files with honest bulk-export reporting
- In-note image attachments with automatic metadata purging (EXIF stripped, re-sealed as PNG)
- Encrypted image storage in the vault (per-attachment AEAD, `grim://attachment/` URLs)
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
| **Qt 6** (Core, Gui, Widgets, Network) | GUI framework + bridge IPC |
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

### Linux (Ubuntu / Kali / Debian)

```bash
sudo apt install build-essential cmake ninja-build \
  qt6-base-dev qt6-tools-dev libgl1-mesa-dev

chmod +x installer/build-linux.sh
./installer/build-linux.sh

# Run from build tree
./build-linux/GrimLedger

# Or install for current user (~/.local)
cd dist/GrimLedger-linux && ./install.sh
```

See [installer/README-linux.md](installer/README-linux.md) for browser bridge setup and options.

### macOS

```bash
brew install qt cmake ninja
export CMAKE_PREFIX_PATH="$(brew --prefix qt)"

chmod +x installer/build-macos.sh
./installer/build-macos.sh

# Run from build tree
open build-macos/GrimLedger.app

# Or install to ~/Applications
cd dist/GrimLedger-macos && ./install.sh
```

See [installer/README-macos.md](installer/README-macos.md) for browser bridge setup and code signing notes.

### Windows installer (Setup.exe)

Build a per-user installer with the desktop app, Qt runtime, and browser extension bundle:

```powershell
# Requires Qt windeployqt + Inno Setup 6
.\installer\build-installer.ps1
# → dist\GrimLedger-Setup.exe
```

See [installer/README.md](installer/README.md) for details.

### Run tests

```bash
# Linux / macOS (after build-linux.sh or build-macos.sh)
ctest --test-dir build-linux --output-on-failure

# Windows
ctest --test-dir build -C Release --output-on-failure
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
| **Export All Notes** | Plaintext `.md` files in a folder (reports partial failures) |
| **Export Encrypted Archive** | Same as encrypted backup |

⚠ Exported Markdown files are **not encrypted**.

---

## Image Attachments

GrimLedger embeds images in Markdown notes. Every inserted image is sanitized for privacy, then encrypted at rest like note titles and bodies.

### Metadata purging

When you insert an image (editor toolbar or **Insert Image**), GrimLedger:

1. Decodes the source file (PNG, JPEG, BMP, WebP, GIF — up to 15 MB input).
2. Strips embedded metadata (EXIF, GPS, camera info, etc.) by decoding pixels and re-encoding without sidecar data.
3. Re-seals the result as **PNG** (max 8192×8192 pixels, 10 MB output cap).

The file picker reflects this workflow: *metadata purged · re-sealed as PNG · encrypted in vault*.

Implementation: `ImageSanitizer` decodes via Qt, normalizes to RGB/RGBA, and writes a fresh PNG — no EXIF or other metadata survives.

### Encrypted storage

Sanitized PNG bytes are encrypted with **XChaCha20-Poly1305 AEAD** (`crypto_aead_xchacha20poly1305_ietf`), the same scheme used for note fields. Each attachment receives a unique UUID; the ciphertext is bound to that attachment id in associated data, which prevents undetected swapping between attachments or notes.

- Stored in the `note_attachments` SQLite table as encrypted BLOBs only.
- Referenced in note bodies as `![alt text](grim://attachment/<uuid>)`.
- Preview decrypts on demand while the vault is unlocked; decrypted image data is cached only for the current session.
- Included in **GRIMBKUP2** encrypted backups with the rest of the vault.

Aggregate attachment limit: 100 MB plaintext per vault.

---

## Project Structure

```
GrimLedger/
  CMakeLists.txt
  README.md
  core/          — Portable crypto + storage (grimledger_core static library)
  mobile/        — Qt Quick UI + GrimVaultController (Android)
  android/       — Manifest, Kotlin autofill/biometric, JNI bridge
  src/
    main.cpp, App.cpp
    ui/          — Qt widgets (login, main window, editor, preview, settings)
    security/    — CryptoManager, PasswordManager, VaultSession, PlatformBiometricUnlock
    storage/     — Database, NoteRepository, AttachmentRepository, CredentialRepository, VaultRepository
    bridge/      — CredentialBridgeServer, BridgeFillCoordinator, BridgeAuth
    browser-extension/ — Chrome/Edge fill helper (native messaging)
    tests/       — Security regression tests (run via `ctest`)
    models/      — Note, Folder, Tag
    search/      — SearchEngine
    markdown/    — MarkdownRenderer, SyntaxHighlighter
    utils/       — TimeUtils, SecureStringUtils, ImageSanitizer
  resources/
    styles/grimledger_dark.qss
  installer/
    build-linux.sh, build-macos.sh, build-release.ps1, grimledger.iss
```

---

## License

This project is provided as-is for educational and personal use. Review the security limitations before storing highly sensitive data.

---

> ⚠ **Lost master passwords cannot be recovered.**
