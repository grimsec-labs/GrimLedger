# GrimLedger Bridge (Phase B)

Connects Chromium browsers (Chrome, Edge, Opera, Brave) to the unlocked GrimLedger desktop app via native messaging.

**Store extension ID:** `ldehlncibafipkfjhfkihkonmcllhjen` (fixed via `manifest.json` `key` field)

## Setup

### Option A — Chrome Web Store (recommended when published)

1. Install GrimLedger desktop ([Windows Setup.exe](../installer/README.md) or [Linux install](../installer/README-linux.md)).
2. Install **GrimLedger Bridge** from the Chrome Web Store (link TBD after publication — see [STORE_LISTING.md](STORE_LISTING.md)).
3. Run the desktop installer’s native-host step (pre-filled with the store extension ID on Windows/Linux/macOS).
4. In GrimLedger **Settings**, enable **browser bridge** and click **Save Settings**.

### Option B — Windows Setup.exe (unpacked until store is live)

1. Build: `.\installer\build-installer.ps1` from repo root (requires Inno Setup 6).
2. Run `dist\GrimLedger-Setup.exe` and follow the wizard (extension ID pre-filled).
3. Load unpacked extension from the folder the installer opens, **or** use the store ID with unpacked load from the same folder.
4. In GrimLedger **Settings**, enable **browser bridge** and click **Save Settings**.

### Option C — Linux / macOS install

1. Build and run `install.sh` from `dist/GrimLedger-linux` or `dist/GrimLedger-macos`.
2. **Do not use sudo** — installs per-user under `~/.local` or `~/Library`.
3. Load unpacked from **`~/GrimLedger-browser-extension`** (symlink created by install) or the path printed by the installer.
4. Press Enter at the native-host prompt to accept the default store extension ID.
5. Enable browser bridge in Settings.

### Option D — Developer / manual

1. Build GrimLedger (`grimledger_host` next to the app binary).
2. Load unpacked → select this `browser-extension` folder (extension ID will be `ldehlncibafipkfjhfkihkonmcllhjen` because of the manifest `key`).
3. Register native host:

```powershell
# Windows
.\native-host\install-windows.ps1 -ExtensionId ldehlncibafipkfjhfkihkonmcllhjen
```

```bash
# Linux
./native-host/install-linux.sh --extension-id ldehlncibafipkfjhfkihkonmcllhjen --browser all
```

```bash
# macOS
./native-host/install-macos.sh --extension-id ldehlncibafipkfjhfkihkonmcllhjen
```

## Packaging for the store

```bash
./installer/package-extension.sh    # → dist/grimledger-bridge-1.0.0.zip
```

See [STORE_LISTING.md](STORE_LISTING.md) and [PRIVACY.md](PRIVACY.md).

## Firefox

Not supported in v1. See [installer/README-firefox.md](../installer/README-firefox.md).

## Security

- Enable the bridge only when you need browser fill. It is **off by default**.
- GrimLedger must be **unlocked**; the bridge stops when you lock the vault.
- Fill requests require **exact origin match** (HTTPS credentials never fill on HTTP).
- GrimLedger shows a **confirmation dialog** before sending a password to the browser.
- The native host adds a per-session **hex-encoded token** and random local endpoint name; only the running GrimLedger instance can answer bridge requests.
- Fill responses include a **nonce** echoed from the request; the background service worker rejects mismatches.
- **Scheme matching is strict**: HTTPS credentials never fill on HTTP pages, even with subdomain allowance enabled.
- Residual threat: another process running as the same OS user may connect to the local socket if the bridge is enabled. Keep the bridge off unless needed.

## Troubleshooting

### Linux / Kali

- **`Specified native messaging host not found`** — You likely ran `install.sh` with `sudo`. Reinstall as your normal user (not root). Chrome reads manifests from `~/.config/...`, not `/root/.config/...`.
- **Cannot pick extension folder** — Use `~/GrimLedger-browser-extension` (symlink from install) instead of navigating into hidden `.local` folders. In the file picker, Ctrl+H shows hidden files.
- **Native host not found after install** — Re-run with explicit paths:
  `~/.local/share/GrimLedger/app/native-host/install-linux.sh --extension-id ldehlncibafipkfjhfkihkonmcllhjen --host-exe ~/.local/share/GrimLedger/app/grimledger_host --browser all`

### General

- `Specified native messaging host not found` → re-run native-host install with extension ID `ldehlncibafipkfjhfkihkonmcllhjen`.
- `Vault is locked` → unlock GrimLedger.
- No matches → ensure the vault key URL host matches the page (e.g. `github.com`).
