#!/usr/bin/env bash
# Build a terminalpolyominos AppImage from a Release CMake build.
#
# Usage:
#   ./packaging/appimage/build.sh
#   ./packaging/appimage/build.sh /path/to/build-dir
#
# Env:
#   APPIMAGETOOL  path to appimagetool (optional; downloaded if missing)
#   OUT_DIR       output directory (default: dist/)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${1:-"$ROOT/build-release"}"
OUT_DIR="${OUT_DIR:-"$ROOT/dist"}"
ARCH="$(uname -m)"
case "$ARCH" in
  x86_64|amd64) APPIMAGE_ARCH=x86_64 ;;
  aarch64|arm64) APPIMAGE_ARCH=aarch64 ;;
  *)
    echo "Unsupported arch: $ARCH" >&2
    exit 1
    ;;
esac

VERSION="$(grep -E '^[[:space:]]*VERSION[[:space:]]+[0-9]' "$ROOT/CMakeLists.txt" | head -1 | awk '{print $2}')"
VERSION="${VERSION:-0.0.0}"
APPDIR="$OUT_DIR/terminalpolyominos.AppDir"
NAME="terminalpolyominos-${VERSION}-${APPIMAGE_ARCH}.AppImage"

echo "==> Configuring Release build in $BUILD_DIR"
# Optional: STATIC_LIBS=1 if libstdc++-static / libgcc is installed (wider portability).
CMAKE_EXTRA=()
if [[ "${STATIC_LIBS:-0}" == "1" ]]; then
  CMAKE_EXTRA+=(-DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc")
fi
cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release "${CMAKE_EXTRA[@]}"
cmake --build "$BUILD_DIR" --target terminalpolyominos -j"$(nproc)"

BIN="$BUILD_DIR/terminalpolyominos"
if [[ ! -x "$BIN" ]]; then
  echo "Missing binary: $BIN" >&2
  exit 1
fi

echo "==> Assembling AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/applications" \
  "$APPDIR/usr/share/metainfo"

install -m755 "$BIN" "$APPDIR/usr/bin/terminalpolyominos"
ln -sf terminalpolyominos "$APPDIR/usr/bin/tpoly"
install -m755 "$ROOT/packaging/appimage/AppRun" "$APPDIR/AppRun"
install -m644 "$ROOT/packaging/appimage/terminalpolyominos.desktop" "$APPDIR/terminalpolyominos.desktop"
install -m644 "$ROOT/packaging/appimage/terminalpolyominos.desktop" \
  "$APPDIR/usr/share/applications/terminalpolyominos.desktop"

# Optional AppStream (harmless locally; useful if you later add Flatpak).
if [[ -f "$ROOT/packaging/appimage/terminalpolyominos.metainfo.xml" ]]; then
  install -m644 "$ROOT/packaging/appimage/terminalpolyominos.metainfo.xml" \
    "$APPDIR/usr/share/metainfo/terminalpolyominos.metainfo.xml"
  # appimagetool historically looks for *.appdata.xml
  install -m644 "$ROOT/packaging/appimage/terminalpolyominos.metainfo.xml" \
    "$APPDIR/usr/share/metainfo/terminalpolyominos.appdata.xml"
fi

mkdir -p "$OUT_DIR"
TOOL="${APPIMAGETOOL:-}"
if [[ -z "$TOOL" ]]; then
  TOOL="$OUT_DIR/appimagetool-${APPIMAGE_ARCH}.AppImage"
  if [[ ! -x "$TOOL" ]]; then
    echo "==> Downloading appimagetool (${APPIMAGE_ARCH})"
    URL="https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${APPIMAGE_ARCH}.AppImage"
    curl -fsSL -o "$TOOL" "$URL"
    chmod +x "$TOOL"
  fi
fi

echo "==> Running appimagetool"
# Extracted appimagetool avoids FUSE requirement on some hosts.
if "$TOOL" --appimage-extract-and-run --version >/dev/null 2>&1; then
  RUN=("$TOOL" --appimage-extract-and-run)
else
  RUN=("$TOOL")
fi

ARCH="$APPIMAGE_ARCH" "${RUN[@]}" "$APPDIR" "$OUT_DIR/$NAME"

echo "==> Built $OUT_DIR/$NAME"
ls -lh "$OUT_DIR/$NAME"
