#!/usr/bin/env bash
# Build a macOS binary tarball from a Release CMake build.
#
# Usage:
#   ./packaging/macos/build.sh
#   ./packaging/macos/build.sh /path/to/build-dir
#
# Env:
#   OUT_DIR     output directory (default: dist/)
#   MAC_ARCH    arm64 or x86_64 (default: host uname -m)
#   CMAKE_OSX_ARCHITECTURES  optional; set for cross-compile (e.g. x86_64 on arm64 host)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${1:-"$ROOT/build-release"}"
OUT_DIR="${OUT_DIR:-"$ROOT/dist"}"

HOST_ARCH="$(uname -m)"
case "$HOST_ARCH" in
  x86_64|amd64) HOST_ARCH=x86_64 ;;
  arm64|aarch64) HOST_ARCH=arm64 ;;
esac
MAC_ARCH="${MAC_ARCH:-$HOST_ARCH}"
case "$MAC_ARCH" in
  x86_64|arm64) ;;
  *)
    echo "Unsupported macOS arch: $MAC_ARCH" >&2
    exit 1
    ;;
esac

VERSION="$(grep -E '^[[:space:]]*VERSION[[:space:]]+[0-9]' "$ROOT/CMakeLists.txt" | head -1 | awk '{print $2}')"
VERSION="${VERSION:-0.0.0}"
STAGE="$OUT_DIR/terminalpolyominos-macos-${MAC_ARCH}"
NAME="terminalpolyominos-${VERSION}-macos-${MAC_ARCH}.tar.gz"

CMAKE_EXTRA=()
if [[ -n "${CMAKE_OSX_ARCHITECTURES:-}" ]]; then
  CMAKE_EXTRA+=(-DCMAKE_OSX_ARCHITECTURES="$CMAKE_OSX_ARCHITECTURES")
elif [[ "$MAC_ARCH" != "$HOST_ARCH" ]]; then
  CMAKE_EXTRA+=(-DCMAKE_OSX_ARCHITECTURES="$MAC_ARCH")
fi

echo "==> Configuring Release build in $BUILD_DIR (macos-${MAC_ARCH})"
# macOS ships Bash 3.2; with `set -u`, empty "${arr[@]}" is an unbound variable.
cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release \
  ${CMAKE_EXTRA[@]+"${CMAKE_EXTRA[@]}"}
cmake --build "$BUILD_DIR" --target terminalpolyominos -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

BIN="$BUILD_DIR/terminalpolyominos"
if [[ ! -x "$BIN" ]]; then
  echo "Missing binary: $BIN" >&2
  exit 1
fi

echo "==> Packaging $NAME"
rm -rf "$STAGE"
mkdir -p "$STAGE"
install -m755 "$BIN" "$STAGE/terminalpolyominos"
ln -sf terminalpolyominos "$STAGE/tpoly"

mkdir -p "$OUT_DIR"
tar -C "$STAGE" -czf "$OUT_DIR/$NAME" terminalpolyominos tpoly
rm -rf "$STAGE"

echo "==> Built $OUT_DIR/$NAME"
ls -lh "$OUT_DIR/$NAME"
file "$BIN"
