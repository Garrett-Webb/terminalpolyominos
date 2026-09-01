# terminalpolyominos (Flatpak)

**App ID:** `io.github.garrett_webb.terminalpolyominos`

Colored polyomino stacking game for Unix terminals. Classic SRS rotation, hold,
ghost piece, lock delay, local high scores, and configurable keys and timing.

This package wraps the upstream `terminalpolyominos` binary (also available as
`tpoly`). It is a **terminal application**: launch it from a desktop entry or
`flatpak run`, and it opens inside the host terminal emulator.

Not affiliated with or endorsed by The Tetris Company.

## Install

Once published on Flathub:

```bash
flatpak install flathub io.github.garrett_webb.terminalpolyominos
```

## Run

```bash
flatpak run io.github.garrett_webb.terminalpolyominos
```

Or start **terminalpolyominos** from the application menu (category: Games).

Settings are stored in `~/.config/tpoly/.tpolyrc`. High scores are stored in
`~/.local/share/tpoly/scores/`.

## Sandbox

The Flatpak is offline and does not request network access.

| Permission | Purpose |
|---|---|
| `xdg-config/tpoly` | User settings (`.tpolyrc`) |
| `xdg-data/tpoly` | Local high-score files |

Kitty keyboard protocol support depends on the host terminal emulator; the
sandbox does not add extra input permissions beyond running in a terminal.

## Build locally

From the repository root:

```bash
./packaging/flatpak/build.sh          # build
./packaging/flatpak/build.sh --run    # build, install to user, launch
```

Requires `flatpak`, `flatpak-builder`, and the Freedesktop 24.08 runtime/SDK.
The build script installs the runtime on first run if it is missing.

Manifest: `io.github.garrett_webb.terminalpolyominos.yml`  
Desktop entry and AppStream metadata are in this directory. Icon:
`icons/io.github.garrett_webb.terminalpolyominos.png`. Screenshots:
`../screenshots/`.

## Upstream

- Homepage: https://github.com/Garrett-Webb/terminalpolyominos
- License: GPL-3.0-or-later
- Issue tracker: https://github.com/Garrett-Webb/terminalpolyominos/issues
