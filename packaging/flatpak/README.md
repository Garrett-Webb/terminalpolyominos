# terminalpolyominos (Flatpak)

**App ID:** `io.github.garrett_webb.terminalpolyominos`

Colored polyomino stacking game for Unix terminals. Classic SRS rotation, hold,
ghost piece, lock delay, local high scores, and configurable keys and timing.

The Flatpak ships two menu entries:

| Desktop file | Launches |
|---|---|
| `io.github.garrett_webb.terminalpolyominos.desktop` | **`terminalpolyominos-gui`** — built-in GTK/VTE window (primary) |
| `io.github.garrett_webb.terminalpolyominos-terminal.desktop` | **`terminalpolyominos`** in your default terminal emulator (`Terminal=true`) |

## Install

Once published on Flathub:

```bash
flatpak install flathub io.github.garrett_webb.terminalpolyominos
```

## Run

```bash
flatpak run io.github.garrett_webb.terminalpolyominos
```

Or start **terminalpolyominos** from the application menu:

- **terminalpolyominos** — built-in window (GUI)
- **terminalpolyominos (terminal)** — your default terminal emulator (Kitty, tmux, etc.)

Advanced users can also run:

```bash
flatpak run --command=terminalpolyominos io.github.garrett_webb.terminalpolyominos
```

Settings are stored in `~/.config/tpoly/.tpolyrc`. High scores are stored in
`~/.local/share/tpoly/scores/`.

## Sandbox

The Flatpak is offline and does not request network access.

| Permission | Purpose |
|---|---|
| `xdg-config/tpoly` | User settings (`.tpolyrc`) |
| `xdg-data/tpoly` | Local high-score files |
| Wayland / X11 | GTK window for `terminalpolyominos-gui` |

The GUI launcher links against **libvte** (GTK4), which is not part of the GNOME
SDK; the Flatpak manifest builds and bundles it as a module.

Kitty keyboard protocol works when running `terminalpolyominos` in a capable
external terminal; the embedded VTE window uses legacy input (expected).

## Build locally

From the repository root:

```bash
./packaging/flatpak/build.sh          # build
./packaging/flatpak/build.sh --run    # build, install to user, launch
```

Requires `flatpak`, `flatpak-builder`, and the GNOME 48 runtime/SDK.
The build script installs the runtime on first run if it is missing.

Manifest: `io.github.garrett_webb.terminalpolyominos.yml`  
Desktop entry and AppStream metadata are in this directory. Icon:
`icons/io.github.garrett_webb.terminalpolyominos.png`. Screenshots:
`../screenshots/`.

## Upstream

- Homepage: https://github.com/Garrett-Webb/terminalpolyominos
- License: GPL-3.0-or-later
- Issue tracker: https://github.com/Garrett-Webb/terminalpolyominos/issues
- GUI launcher sources: `gui/README.md`
