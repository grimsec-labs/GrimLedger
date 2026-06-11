# Chrome Web Store listing copy (GrimLedger Bridge)

Use this text when publishing. Replace `STORE_URL` after approval.

## Extension ID

```
ldehlncibafipkfjhfkihkonmcllhjen
```

Deterministic from the public key in `manifest.json`. Installers register the native host against this ID.

## Package

```bash
# Linux / macOS
./installer/package-extension.sh

# Windows
.\installer\package-extension.ps1
```

Upload `dist/grimledger-bridge-1.0.0.zip` to the [Chrome Web Store Developer Dashboard](https://chrome.google.com/webstore/devconsole) ($5 one-time developer registration).

## Short description (132 chars max)

Fill logins from your local GrimLedger vault. Requires the GrimLedger desktop app; no cloud, no accounts.

## Detailed description

GrimLedger Bridge connects Chrome, Edge, Opera, and other Chromium browsers to the **GrimLedger desktop app** on your computer.

**Requires GrimLedger desktop** — Install GrimLedger from [github.com/grimsec-labs/GrimLedger](https://github.com/grimsec-labs/GrimLedger). The extension alone cannot store or unlock your vault.

**Local-only** — No cloud accounts. No online sync. Credentials stay encrypted in your GrimLedger vault on disk.

**Opt-in bridge** — Browser fill is **disabled by default**. Enable it in GrimLedger Settings when you need it.

**Confirmation before fill** — GrimLedger asks you to confirm before sending a password to the browser.

**Origin matching** — Vault entries must match the page origin. HTTPS credentials never fill on HTTP pages.

### Setup

1. Install GrimLedger desktop and run the installer’s native-host registration (or install from `dist/`).
2. Install this extension from the Chrome Web Store.
3. In GrimLedger: Settings → enable **browser bridge** → Save Settings.
4. Unlock GrimLedger while filling logins.

### Permissions explained

- **Native messaging** — Talks to the GrimLedger app on your machine only.
- **Access to all sites** — Needed to offer fill on login pages you visit; no data is sent to GrimLedger servers.

Privacy policy: host `PRIVACY.md` from the repo on GitHub Pages or link to  
`https://github.com/grimsec-labs/GrimLedger/blob/main/browser-extension/PRIVACY.md`

## Single purpose

Password-manager-style login fill from a local desktop vault via native messaging.

## Category

Productivity

## Privacy practices (dashboard)

- Single purpose: Yes
- No remote code
- No data collection declared (extension does not collect user data)
- Privacy policy URL: (GitHub link above)

## Edge Add-ons

Submit the same ZIP to [Microsoft Edge Add-ons](https://partner.microsoft.com/dashboard/microsoftedge/) or document that Edge users may install from the Chrome Web Store.

## Opera

Opera users can enable “Install Chrome extensions” and install from the Chrome Web Store. Register the native host for Opera on Linux via `install-linux.sh --browser opera` or `--browser all`.

## After publication

1. Update `STORE_URL` in docs when the listing is live.
2. Update [`installer/README.md`](../installer/README.md) with the store link.
3. Windows Setup.exe already pre-fills extension ID `{#StoreExtensionId}` in the Inno wizard.
