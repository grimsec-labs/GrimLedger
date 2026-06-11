#!/usr/bin/env bash
# Build and stage GrimLedger for Linux (Ubuntu, Kali, Debian, etc.)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build-linux}"
OUT_DIR="${OUT_DIR:-$REPO_ROOT/dist/GrimLedger-linux}"
CONFIG="${CONFIG:-Release}"
RUN_TESTS="${RUN_TESTS:-1}"
BUNDLE_QT="${BUNDLE_QT:-auto}"

usage() {
    cat <<'EOF'
Usage: installer/build-linux.sh [options]

Options:
  --build-dir PATH    CMake build directory (default: build-linux)
  --out-dir PATH      Staged release folder (default: dist/GrimLedger-linux)
  --no-tests          Skip ctest after build
  --bundle-qt         Run linuxdeployqt when available
  --no-bundle-qt      Do not bundle Qt libraries (smaller stage; needs system Qt6)
  -h, --help          Show this help

Prerequisites (Debian/Ubuntu/Kali):
  sudo apt install build-essential cmake ninja-build \
    qt6-base-dev qt6-tools-dev libgl1-mesa-dev

Optional (self-contained bundle):
  linuxdeployqt from https://github.com/probonopd/linuxdeployqt
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --out-dir) OUT_DIR="$2"; shift 2 ;;
        --no-tests) RUN_TESTS=0; shift ;;
        --bundle-qt) BUNDLE_QT=1; shift ;;
        --no-bundle-qt) BUNDLE_QT=0; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
    esac
done

find_qt_bin() {
    if [[ -n "${QT_BIN:-}" && -d "$QT_BIN" ]]; then
        echo "$QT_BIN"
        return
    fi
    if command -v qmake6 >/dev/null 2>&1; then
        qmake6 -query QT_INSTALL_BINS
        return
    fi
    if command -v qmake >/dev/null 2>&1; then
        qmake -query QT_INSTALL_BINS
        return
    fi
    return 1
}

echo "==> Configuring ($CONFIG) in $BUILD_DIR"
GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
    GENERATOR_ARGS=(-G Ninja)
fi
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" "${GENERATOR_ARGS[@]}" \
    -DCMAKE_BUILD_TYPE="$CONFIG"

echo "==> Building"
cmake --build "$BUILD_DIR" --parallel "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

APP_BIN="$BUILD_DIR/GrimLedger"
HOST_BIN="$BUILD_DIR/grimledger_host"
if [[ ! -x "$APP_BIN" ]]; then
    echo "GrimLedger binary not found at $APP_BIN" >&2
    exit 1
fi
if [[ ! -x "$HOST_BIN" ]]; then
    echo "grimledger_host binary not found at $HOST_BIN" >&2
    exit 1
fi

if [[ "$RUN_TESTS" == "1" ]]; then
    echo "==> Running tests"
    ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

echo "==> Staging release at $OUT_DIR"
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

cp "$APP_BIN" "$OUT_DIR/"
cp "$HOST_BIN" "$OUT_DIR/"
chmod 755 "$OUT_DIR/GrimLedger" "$OUT_DIR/grimledger_host"

cp -r "$REPO_ROOT/browser-extension" "$OUT_DIR/"
mkdir -p "$OUT_DIR/native-host"
cp -r "$REPO_ROOT/native-host/." "$OUT_DIR/native-host/"
cp "$SCRIPT_DIR/install-linux.sh" "$OUT_DIR/install.sh"
cp "$SCRIPT_DIR/extension-id.txt" "$OUT_DIR/"
cp "$SCRIPT_DIR/README-linux.md" "$OUT_DIR/README.txt"

if [[ "$BUNDLE_QT" == "auto" ]]; then
    if command -v linuxdeployqt >/dev/null 2>&1; then
        BUNDLE_QT=1
    else
        BUNDLE_QT=0
    fi
fi

if [[ "$BUNDLE_QT" == "1" ]]; then
    if ! command -v linuxdeployqt >/dev/null 2>&1; then
        echo "linuxdeployqt not found; staged build requires system Qt6 packages." >&2
    else
        echo "==> Bundling Qt libraries with linuxdeployqt"
        linuxdeployqt "$OUT_DIR/GrimLedger" -always-overwrite \
            -executable="$OUT_DIR/grimledger_host" \
            -no-translations
    fi
fi

cat > "$OUT_DIR/GrimLedger.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=GrimLedger
Comment=Encrypted markdown vault and password manager
Exec=$OUT_DIR/GrimLedger
Icon=grimledger
Terminal=false
Categories=Office;Security;
EOF

echo ""
echo "Build complete."
echo "  Binary:  $APP_BIN"
echo "  Staged:  $OUT_DIR"
echo ""
echo "Run from build tree:"
echo "  $APP_BIN"
echo ""
echo "Install for current user (~/.local):"
echo "  cd \"$OUT_DIR\" && ./install.sh"
