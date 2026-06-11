# GrimLedger — macOS build & install

## Prerequisites

```bash
brew install qt cmake ninja
export CMAKE_PREFIX_PATH="$(brew --prefix qt)"
```

## Build

From the repository root:

```bash
chmod +x installer/build-macos.sh installer/install-macos.sh
./installer/build-macos.sh
```

Output:

| Path | Contents |
|------|----------|
| `build-macos/GrimLedger.app` | Release app bundle |
| `dist/GrimLedger-macos/` | Staged folder (app + extension + install script) |

The build script runs `macdeployqt` when available so the `.app` includes Qt frameworks.

## Run without installing

```bash
open build-macos/GrimLedger.app
```

## Install (current user)

```bash
cd dist/GrimLedger-macos
./install.sh
```

Installs `GrimLedger.app` to `~/Applications/` and copies browser-extension files to `~/Library/Application Support/GrimLedger/`.

## Browser bridge

1. Load unpacked from **`~/GrimLedger-browser-extension`** (symlink) or `~/Library/Application Support/GrimLedger/browser-extension/`.
2. Store extension ID: **`ldehlncibafipkfjhfkihkonmcllhjen`**
3. `install.sh` prompts for the extension ID and registers the native host by default. Manual re-run:

```bash
~/Library/Application\ Support/GrimLedger/native-host/install-macos.sh \
  --extension-id ldehlncibafipkfjhfkihkonmcllhjen \
  --host-exe ~/Library/Application\ Support/GrimLedger/native-host/grimledger_host
```

4. Enable **browser bridge** in GrimLedger Settings → Save Settings.

When the Chrome Web Store listing is live, install the extension from the store instead of loading unpacked.

## Firefox

Not supported yet. See [README-firefox.md](README-firefox.md).

## Vault location

`~/Library/Application Support/GrimLedger/vault.grim`

## Tests

```bash
ctest --test-dir build-macos --output-on-failure
```

## Gatekeeper

Unsigned builds may require **Right-click → Open** the first time, or:

```bash
xattr -cr ~/Applications/GrimLedger.app
```

For public distribution, sign and notarize the app with an Apple Developer ID.
