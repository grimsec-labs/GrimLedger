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
| Native messaging host | Registered if you paste extension ID in the wizard |

Per-user install (`PrivilegesRequired=lowest`) — no admin required.

## After running Setup.exe

1. Load unpacked extension from the folder the wizard opens
2. Enable browser bridge in GrimLedger Settings → **Save Settings**
3. Keep GrimLedger unlocked while filling logins

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

## Future: Chrome Web Store

Publishing the extension gives a fixed ID so the installer can register the native host without user input. Option 2 (unpacked) is used until then.
