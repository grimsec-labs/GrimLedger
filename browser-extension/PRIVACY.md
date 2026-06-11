# GrimLedger Bridge — Privacy Policy

**Last updated:** June 2025

GrimLedger Bridge is a browser extension that connects Chromium-based browsers to the **GrimLedger desktop application** on your computer. It does not operate as a standalone password manager and does not send your vault data to GrimLedger servers — there are no GrimLedger cloud accounts or sync services.

## What the extension does

- On pages you visit, the extension can offer to fill login forms using credentials stored in your **local GrimLedger vault**.
- Communication with your vault happens only through a **native messaging host** (`grimledger_host`) installed alongside the GrimLedger desktop app on the same machine.
- Fill requests are subject to GrimLedger’s own security rules: the vault must be unlocked, the browser bridge must be enabled in Settings, origins must match vault entries, and GrimLedger shows a **confirmation dialog** before sending a password to the browser.

## Data collection

The extension **does not collect, transmit, or sell** personal data to GrimLedger or third parties.

| Data | Handled how |
|------|-------------|
| Vault passwords / notes | Stay on your device inside GrimLedger; not uploaded by the extension |
| Browsing history | Not collected or stored by the extension |
| Page URLs | Used locally to match vault entries and fill forms; not sent to external servers |
| Analytics / telemetry | None |

## Permissions

| Permission | Why |
|------------|-----|
| `nativeMessaging` | Talk to the local GrimLedger native host on your computer |
| `activeTab` | Inspect the current tab to find login fields when you request a fill |
| `scripting` | Insert credentials into matched login fields after you confirm |
| `host_permissions` (`http://*/*`, `https://*/*`) | Required to run on login pages across sites you choose to fill |

## Third parties

The extension does not embed third-party analytics, advertising, or remote configuration SDKs.

## Children

GrimLedger Bridge is not directed at children under 13.

## Changes

Material changes to this policy will be reflected in the extension’s Chrome Web Store listing and this document in the repository.

## Contact

Project home: [github.com/grimsec-labs/GrimLedger](https://github.com/grimsec-labs/GrimLedger)

For privacy questions, open an issue on the GitHub repository.
