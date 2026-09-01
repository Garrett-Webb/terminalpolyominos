# Flatpak / Flathub

App ID: **`io.github.garrett_webb.terminalpolyominos`**

## Icon (you provide)

See [icons/README.md](icons/README.md). Minimum: **256×256 PNG** named
`io.github.garrett_webb.terminalpolyominos.png` in `packaging/flatpak/icons/`.

## Local smoke test

```bash
# From repo root
chmod +x packaging/flatpak/build.sh
./packaging/flatpak/build.sh --run
```

Requires `flatpak` and `flatpak-builder` (on Fedora: `sudo dnf install flatpak flatpak-builder`), plus the Freedesktop 24.08 runtime/SDK
(installs automatically on first run).

## Flathub submission

1. Screenshots live in `packaging/screenshots/` and are referenced from AppStream metainfo.
2. Fork [flathub/flathub](https://github.com/flathub/flathub) and open a PR using
   [NEW_APPLICATION.md](https://github.com/flathub/flathub/blob/master/NEW_APPLICATION.md).
3. In the **flathub-data** repo (created by the PR), use a manifest like:

```yaml
app-id: io.github.garrett_webb.terminalpolyominos
runtime: org.freedesktop.Platform
runtime-version: '24.08'
sdk: org.freedesktop.Sdk
command: terminalpolyominos
finish-args:
  - --filesystem=xdg-config/tpoly:create
  - --filesystem=xdg-data/tpoly:create
modules:
  - name: terminalpolyominos
    buildsystem: cmake-ninja
    config-opts:
      - -DCMAKE_BUILD_TYPE=Release
    sources:
      - type: git
        url: https://github.com/Garrett-Webb/terminalpolyominos.git
        tag: v0.5.2
        commit: 386e9dc23e99ba0775fd19f3ddf0ce099f6500b5
    post-install:
      # (same post-install block as io.github.garrett_webb.terminalpolyominos.yml)
```

4. Include `flathub.json` from this directory.
5. Wait for bot build + human review.

After acceptance, bump the `tag` / `commit` in the Flathub manifest for each release.

## Permissions

| Permission | Purpose |
|---|---|
| `xdg-config/tpoly` | Settings file `.tpolyrc` |
| `xdg-data/tpoly` | High scores |

No network socket. Kitty keyboard protocol depends on the host terminal emulator.

## Related

- [Packaging overview](../appimage/../packaging.md) (wiki)
- [AppImage build](../appimage/build.sh)
