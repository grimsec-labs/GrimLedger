#!/usr/bin/env bash
# Build and stage GrimLedger for macOS (.app bundle)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build-macos}"
OUT_DIR="${OUT_DIR:-$REPO_ROOT/dist/GrimLedger-macos}"
CONFIG="${CONFIG:-Release}"
RUN_TESTS="${RUN_TESTS:-1}"

usage() {
    cat <<'EOF'
Usage: installer/build-macos.sh [options]

Options:
  --build-dir PATH    CMake build directory (default: build-macos)
  --out-dir PATH      Staged release folder (default: dist/GrimLedger-macos)
  --no-tests          Skip ctest after build
  -h, --help          Show this help

Prerequisites:
  brew install qt cmake ninja
  export CMAKE_PREFIX_PATH="$(brew --prefix qt)"
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --out-dir) OUT_DIR="$2"; shift 2 ;;
        --no-tests) RUN_TESTS=0; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
    esac
done

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must run on macOS." >&2
    exit 1
fi

if [[ -z "${CMAKE_PREFIX_PATH:-}" ]]; then
    if command -v brew >/dev/null 2>&1; then
        export CMAKE_PREFIX_PATH="$(brew --prefix qt)"
        echo "Using CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH"
    fi
fi

echo "==> Configuring ($CONFIG) in $BUILD_DIR"
GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
    GENERATOR_ARGS=(-G Ninja)
fi
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" "${GENERATOR_ARGS[@]}" \
    -DCMAKE_BUILD_TYPE="$CONFIG"

echo "==> Building"
cmake --build "$BUILD_DIR" --parallel "$(sysctl -n hw.ncpu)"

APP_BUNDLE="$BUILD_DIR/GrimLedger.app"
HOST_BIN="$BUILD_DIR/grimledger_host"
if [[ ! -d "$APP_BUNDLE" ]]; then
    echo "GrimLedger.app not found at $APP_BUNDLE" >&2
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

MACDEPLOYQT=""
for candidate in macdeployqt "$(brew --prefix qt 2>/dev/null)/bin/macdeployqt"; do
    if command -v "$candidate" >/dev/null 2>&1; then
        MACDEPLOYQT="$candidate"
        break
    fi
    if [[ -x "$candidate" ]]; then
        MACDEPLOYQT="$candidate"
        break
    fi
done

if [[ -n "$MACDEPLOYQT" ]]; then
    echo "==> Bundling Qt frameworks with macdeployqt"
    "$MACDEPLOYQT" "$APP_BUNDLE" -always-overwrite -no-strip
else
    echo "Warning: macdeployqt not found; .app may require Homebrew Qt at runtime." >&2
fi

echo "==> Staging release at $OUT_DIR"
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

cp -R "$APP_BUNDLE" "$OUT_DIR/"
cp "$HOST_BIN" "$OUT_DIR/"
chmod 755 "$OUT_DIR/grimledger_host"

cp -r "$REPO_ROOT/browser-extension" "$OUT_DIR/"
mkdir -p "$OUT_DIR/native-host"
cp -r "$REPO_ROOT/native-host/." "$OUT_DIR/native-host/"
cp "$SCRIPT_DIR/install-macos.sh" "$OUT_DIR/install.sh"
cp "$SCRIPT_DIR/extension-id.txt" "$OUT_DIR/"
cp "$SCRIPT_DIR/README-macos.md" "$OUT_DIR/README.txt"

echo ""
echo "Build complete."
echo "  App:     $APP_BUNDLE"
echo "  Staged:  $OUT_DIR"
echo ""
echo "Run from build tree:"
echo "  open \"$APP_BUNDLE\""
echo ""
echo "Install to ~/Applications:"
echo "  cd \"$OUT_DIR\" && ./install.sh"
