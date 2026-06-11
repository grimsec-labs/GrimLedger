# Firefox / AMO (deferred)

GrimLedger Bridge v1 targets **Chromium** browsers (Chrome Web Store, Edge, Opera, Brave). Firefox support is planned as a follow-up — not included in the initial store release.

## Why Firefox is separate

| Area | Chromium (current) | Firefox (needed) |
|------|-------------------|------------------|
| Extension API | `chrome.*` in `background.js`, `content.js`, `popup.js` | [`webextension-polyfill`](https://github.com/mozilla/webextension-polyfill) or dual-namespace shim |
| Native host manifest | `allowed_origins: ["chrome-extension://<id>/"]` | `allowed_extensions: ["<id>@addons.mozilla.org"]` |
| Host install path (Linux) | `~/.config/google-chrome/NativeMessagingHosts/` | `~/.mozilla/native-messaging-hosts/` |
| Host install path (macOS) | `~/Library/Application Support/Google/Chrome/NativeMessagingHosts/` | Same Mozilla folder under Application Support |
| Host install path (Windows) | Registry under `Google\Chrome\NativeMessagingHosts` | Registry under `Mozilla\NativeMessagingHosts` |
| Distribution | Chrome Web Store (+ Edge partner portal optional) | [Firefox Add-ons (AMO)](https://addons.mozilla.org/developers/) — stricter review for native messaging |

## Planned implementation sketch

1. Add `browser-extension/manifest.firefox.json` overlay or build step merging `browser_specific_settings.gecko.id`.
2. Vendor `browser-polyfill` and import it from background/content/popup scripts.
3. Add `native-host/com.grimledger.bridge.firefox.json` template with `allowed_extensions`.
4. Add `native-host/install-firefox.sh` (and `.ps1`) registering under Mozilla paths.
5. Submit to AMO with the same privacy disclosures as [`browser-extension/PRIVACY.md`](../browser-extension/PRIVACY.md).

## Interim workaround

None officially supported. Use GrimLedger desktop copy/paste or a Chromium browser with the published Chrome Web Store extension.

## Extension ID note

The fixed Chromium ID (`installer/extension-id.txt`) is **not** the Firefox add-on ID. AMO assigns an ID like `grimledger-bridge@grimsec-labs` at submission time.
