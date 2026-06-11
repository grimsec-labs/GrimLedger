# GrimLedger — Linux build & install

## Prerequisites

**Debian / Ubuntu / Kali:**

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build \
  qt6-base-dev qt6-tools-dev libgl1-mesa-dev
```

(`sudo` is only for **system packages**, not for GrimLedger install.)

## Build

From the repository root:

```bash
chmod +x installer/build-linux.sh installer/install-linux.sh
./installer/build-linux.sh
```

Output:

| Path | Contents |
|------|----------|
| `build-linux/GrimLedger` | Release binary |
| `dist/GrimLedger-linux/` | Staged folder (app + extension + install script) |

### Options

```bash
./installer/build-linux.sh --no-tests          # skip ctest
./installer/build-linux.sh --bundle-qt         # force linuxdeployqt bundling
./installer/build-linux.sh --no-bundle-qt      # rely on system Qt6 libs
```

If `linuxdeployqt` is installed, it is used automatically to bundle Qt libraries into `dist/GrimLedger-linux/`.

## Run without installing

```bash
./build-linux/GrimLedger
```

## Install (current user)

```bash
cd dist/GrimLedger-linux
./install.sh
```

**Do not run `./install.sh` with sudo or as root.** GrimLedger installs per-user under `~/.local`. Running as root places files under `/root/.local/...` while Chrome runs as your user — the browser bridge will not work.

Installs to:

- App: `~/.local/share/GrimLedger/app/`
- CLI symlink: `~/.local/bin/grimledger`
- Desktop entry: `~/.local/share/applications/grimledger.desktop`
- Browser extension files: `~/.local/share/GrimLedger/browser-extension/`
- Dev symlink: `~/GrimLedger-browser-extension` → extension folder (easy **Load unpacked** path)

Ensure `~/.local/bin` is on your `PATH`.

### Install options

```bash
./install.sh --extension-id ldehlncibafipkfjhfkihkonmcllhjen --browser all
./install.sh --skip-native-host
./install.sh --no-dev-extension-link
```

The installer prompts for the extension ID and defaults to the Chrome Web Store ID from `extension-id.txt`.

## Browser bridge

1. **Load unpacked** in Chrome/Chromium/Brave/Edge/Opera:
   - Prefer **`~/GrimLedger-browser-extension`** (visible symlink), or
   - `~/.local/share/GrimLedger/browser-extension/` (press **Ctrl+H** in the file picker to show hidden `.local` folders).
2. Extension ID (store / dev): **`ldehlncibafipkfjhfkihkonmcllhjen`**
3. The installer registers the native host when you accept the default ID at the prompt. Manual re-run:

```bash
~/.local/share/GrimLedger/app/native-host/install-linux.sh \
  --extension-id ldehlncibafipkfjhfkihkonmcllhjen \
  --host-exe ~/.local/share/GrimLedger/app/grimledger_host \
  --browser all
```

`--browser` accepts: `chrome`, `chromium`, `brave`, `opera`, `edge`, `both` (chrome+chromium), `all`.

4. Enable **browser bridge** in GrimLedger Settings → Save Settings.

When the Chrome Web Store listing is live, install the extension from the store instead of loading unpacked.

## Vault location

`~/.local/share/GrimLedger/vault.grim`

## Tests

```bash
ctest --test-dir build-linux --output-on-failure
```

## Firefox

Not supported yet. See [README-firefox.md](README-firefox.md).
