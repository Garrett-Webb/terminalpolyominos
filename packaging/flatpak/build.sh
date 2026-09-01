#!/usr/bin/env bash
# Build and optionally install a local Flatpak for terminalpolyominos.
#
# Usage (from repo root):
#   ./packaging/flatpak/build.sh          # build only
#   ./packaging/flatpak/build.sh --install
#   ./packaging/flatpak/build.sh --run    # build, install, launch
#
# Prerequisites:
#   flatpak, flatpak-builder  (Fedora: sudo dnf install flatpak flatpak-builder)
#   org.gnome.Platform//48
#   org.gnome.Sdk//48
#
# Icon: place packaging/flatpak/icons/io.github.garrett_webb.terminalpolyominos.png
#       (256×256 PNG or SVG). A placeholder is generated if missing.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
FLATPAK_DIR="$ROOT/packaging/flatpak"
APP_ID="io.github.garrett_webb.terminalpolyominos"
MANIFEST="$FLATPAK_DIR/${APP_ID}.yml"
ICON_DIR="$FLATPAK_DIR/icons"
ICON_PNG="$ICON_DIR/${APP_ID}.png"
ICON_SVG="$ICON_DIR/${APP_ID}.svg"
BUILD_DIR="$ROOT/build-flatpak"
REPO_DIR="$ROOT/dist/flatpak-repo"
STATE_DIR="$ROOT/.flatpak-builder"

INSTALL=0
RUN=0
for arg in "$@"; do
  case "$arg" in
    --install) INSTALL=1 ;;
    --run) INSTALL=1; RUN=1 ;;
    -h|--help)
      sed -n '2,20p' "$0"
      exit 0
      ;;
    *)
      echo "Unknown option: $arg" >&2
      exit 1
      ;;
  esac
done

ensure_runtime() {
  if ! flatpak info org.gnome.Platform//48 >/dev/null 2>&1; then
    echo "==> Installing Flatpak runtime org.gnome.Platform//48"
    flatpak install -y flathub org.gnome.Platform//48 org.gnome.Sdk//48
  fi
}

ensure_icon() {
  if [[ -f "$ICON_SVG" || -f "$ICON_PNG" ]]; then
    return
  fi
  echo "==> No icon found — generating placeholder PNG (replace before Flathub submission)"
  mkdir -p "$ICON_DIR"
  python3 - "$ICON_PNG" <<'PY'
import struct, sys, zlib
path = sys.argv[1]
w = h = 256
# Dark blue tile with a simple cyan T-shape hint.
raw = bytearray()
for y in range(h):
    raw.append(0)
    for x in range(w):
        r, g, b = 11, 18, 32
        if 40 <= x < 216 and 40 <= y < 216:
            r, g, b = 18, 28, 48
        # cyan cross (rough T-piece silhouette)
        cx, cy = 128, 128
        if abs(x - cx) < 18 and 70 <= y < 186:
            r, g, b = 0, 180, 200
        if abs(y - 100) < 18 and 70 <= x < 186:
            r, g, b = 0, 180, 200
        raw.extend((r, g, b))
compressed = zlib.compress(bytes(raw), 9)

def chunk(tag: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

png = b"\x89PNG\r\n\x1a\n"
png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
png += chunk(b"IDAT", compressed)
png += chunk(b"IEND", b"")
open(path, "wb").write(png)
PY
}

ensure_runtime
ensure_icon

echo "==> flatpak-builder"
flatpak-builder \
  --force-clean \
  --state-dir="$STATE_DIR" \
  --repo="$REPO_DIR" \
  "$BUILD_DIR" \
  "$MANIFEST"

if [[ "$INSTALL" -eq 1 ]]; then
  echo "==> flatpak install --user (reinstall if already present)"
  flatpak install -y --reinstall --user "$REPO_DIR" "$APP_ID"
fi

if [[ "$RUN" -eq 1 ]]; then
  echo "==> flatpak run $APP_ID"
  exec flatpak run "$APP_ID"
fi

echo "==> Done. Repo: $REPO_DIR"
echo "    Install: flatpak install --user $REPO_DIR $APP_ID"
echo "    Run:     flatpak run $APP_ID"
