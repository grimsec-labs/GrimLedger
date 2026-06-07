# GrimLedger Browser Bridge (Phase B)

Connects Chrome/Edge to the unlocked GrimLedger desktop app via native messaging.

## Setup

1. Build GrimLedger (`grimledger_host.exe` next to `GrimLedger.exe` — e.g. `build/` on MinGW or `build/Release/` on MSVC).
2. In Chrome, open `chrome://extensions`, enable **Developer mode**, **Load unpacked** → select this `browser-extension` folder.
3. Copy the extension ID from the extensions page.
4. From repo root (PowerShell):

```powershell
.\native-host\install-windows.ps1 -ExtensionId YOUR_EXTENSION_ID_HERE
```

5. Unlock GrimLedger and keep it running.
6. Add vault keys with a **URL** that matches the site (e.g. `https://github.com`).
7. On a login page, open the GrimLedger extension popup and click a match to fill.

## Security

- GrimLedger must be **unlocked**; the bridge stops when you lock the vault.
- Fill requests require **origin match** against the stored credential URL.
- GrimLedger shows a **confirmation dialog** before sending a password to the browser.

## Troubleshooting

- `Specified native messaging host not found` → re-run `install-windows.ps1` with the correct extension ID.
- `Vault is locked` → unlock GrimLedger.
- No matches → ensure the vault key URL host matches the page (e.g. `github.com`).
