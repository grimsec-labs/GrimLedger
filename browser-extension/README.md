# GrimLedger Browser Bridge (Phase B)

Connects Chrome/Edge to the unlocked GrimLedger desktop app via native messaging.

## Setup

### Option A — Windows Setup.exe (recommended)

1. Build: `.\installer\build-installer.ps1` from repo root (requires Inno Setup 6).
2. Run `dist\GrimLedger-Setup.exe` and follow the wizard.
3. Load unpacked extension from the folder the installer opens.
4. In GrimLedger **Settings**, enable **browser bridge** and click **Save Settings**.

### Option B — Developer / manual

1. Build GrimLedger (`grimledger_host.exe` next to `GrimLedger.exe` — e.g. `build/` on MinGW or `build/Release/` on MSVC).
2. In Chrome, open `chrome://extensions`, enable **Developer mode**, **Load unpacked** → select this `browser-extension` folder.
3. Copy the extension ID from the extensions page.
4. From repo root (PowerShell):

```powershell
.\native-host\install-windows.ps1 -ExtensionId YOUR_EXTENSION_ID_HERE
```

5. In GrimLedger **Settings → Save Settings** after enabling **browser bridge** (disabled by default).
6. Unlock GrimLedger and keep it running. The bridge restarts after each unlock.
7. Add vault keys with an exact **URL** origin (scheme + host + port). Enable **Allow subdomains** only when needed.
8. On a login page, open the GrimLedger extension popup and click a match to fill.

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

- `Specified native messaging host not found` → re-run `install-windows.ps1` with the correct extension ID.
- `Vault is locked` → unlock GrimLedger.
- No matches → ensure the vault key URL host matches the page (e.g. `github.com`).
