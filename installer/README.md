# GrimLedger installers

| Platform | Build script | Install guide |
|----------|--------------|---------------|
| **Windows** | `build-installer.ps1` | This file |
| **Linux** | `build-linux.sh` | [README-linux.md](README-linux.md) |
| **macOS** | `build-macos.sh` | [README-macos.md](README-macos.md) |

---

# GrimLedger Windows installer

**Recommended:** single `Setup.exe` built with **Inno Setup 6** (free, widely used for Qt apps).

## Build Setup.exe (one command)

Prerequisites:

- Qt 6 with `windeployqt` on PATH (or `$env:QT_BIN`)
- [Inno Setup 6](https://jrsoftware.org/isinfo.php)

From repo root:

```powershell
.\installer\build-installer.ps1
```

Output: **`dist\GrimLedger-Setup.exe`**

Also copied next to the installer in `dist\`:

| File | When to read |
|------|----------------|
| `READ-BEFORE-INSTALL.txt` | Before running Setup.exe |
| `AFTER-INSTALL.txt` | After setup (browser bridge steps) |

The same text is shown inside the Setup wizard and saved in the app folder after install.

Stage only (no Setup.exe):

```powershell
.\installer\build-release.ps1 -StageOnly
```

## What Setup.exe installs

| Component | Location |
|-----------|----------|
| GrimLedger + Qt runtime | `%LOCALAPPDATA%\GrimLedger\app\` |
| Browser extension files | `%LOCALAPPDATA%\GrimLedger\browser-extension\` |
| Start Menu shortcut | `GrimLedger` |
| Native messaging host | Registered with store extension ID (pre-filled in wizard) |

Per-user install (`PrivilegesRequired=lowest`) — no admin required.

Before installation, Setup.exe shows **READ BEFORE INSTALL** (requirements, security notes, install locations). After setup, a short browser-bridge checklist is shown. Both files are also copied to the app folder as `READ-BEFORE-INSTALL.txt` and the post-install notes reference `INSTALL.txt`.

## After running Setup.exe

1. Install **GrimLedger Bridge** from the Chrome Web Store when available, or load unpacked from the folder the wizard opens
2. Native messaging host is registered automatically if you kept the pre-filled extension ID
3. Enable browser bridge in GrimLedger Settings → **Save Settings**
4. Keep GrimLedger unlocked while filling logins

## Manual / developer install

```powershell
.\installer\build-release.ps1 -StageOnly
cd dist\GrimLedger
.\install.ps1
```

## Uninstall

Windows Settings → Apps → GrimLedger, or Start Menu → Uninstall GrimLedger.

Removes the app and native-host registry keys. Vault database under `%APPDATA%\GrimLedger\` is kept unless you delete it.

## Code signing (optional, for public release)

Unsigned `Setup.exe` may trigger SmartScreen warnings. For distribution outside your machine, sign with an Authenticode certificate:

```powershell
signtool sign /fd SHA256 /a /tr http://timestamp.digicert.com /td SHA256 dist\GrimLedger-Setup.exe
```

## Chrome Web Store extension

**Extension ID:** `ldehlncibafipkfjhfkihkonmcllhjen` (fixed via manifest public key in `browser-extension/manifest.json`)

**Store URL:** TBD after publication — see [browser-extension/STORE_LISTING.md](../browser-extension/STORE_LISTING.md)

Package for upload:

```powershell
.\installer\package-extension.ps1
# → dist\grimledger-bridge-1.0.0.zip
```

Privacy policy for the listing: [browser-extension/PRIVACY.md](../browser-extension/PRIVACY.md)

Windows Setup.exe and `install.ps1` default to the store extension ID for native-host registration. Linux/macOS `install.sh` prompts with the same default.

**Opera / Edge:** Install from the Chrome Web Store (Opera: enable Chrome extensions). Native host: Windows via `install-windows.ps1`; Linux via `install-linux.sh --browser all`.

**Firefox:** Deferred — see [README-firefox.md](README-firefox.md).
