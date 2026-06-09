# GrimLedger — Linux build & install

## Prerequisites

**Debian / Ubuntu / Kali:**

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build \
  qt6-base-dev qt6-tools-dev libgl1-mesa-dev
```

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

Installs to:

- App: `~/.local/share/GrimLedger/app/`
- CLI symlink: `~/.local/bin/grimledger`
- Desktop entry: `~/.local/share/applications/grimledger.desktop`
- Browser extension files: `~/.local/share/GrimLedger/browser-extension/`

Ensure `~/.local/bin` is on your `PATH`.

## Browser bridge

1. Load unpacked extension from the extension folder above.
2. Copy the extension ID from `chrome://extensions`.
3. Register the native host:

```bash
~/.local/share/GrimLedger/app/native-host/install-linux.sh \
  --extension-id YOUR_EXTENSION_ID \
  --host-exe ~/.local/share/GrimLedger/app/grimledger_host
```

## Vault location

`~/.local/share/GrimLedger/vault.grim`

## Tests

```bash
ctest --test-dir build-linux --output-on-failure
```
