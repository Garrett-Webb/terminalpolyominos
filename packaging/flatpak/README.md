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

Settings and high scores use the sandbox XDG directories (not the host
`~/.config/tpoly` or `~/.local/share/tpoly` paths used by native/AppImage
installs):

- `~/.var/app/io.github.garrett_webb.terminalpolyominos/config/tpoly/.tpolyrc`
- `~/.var/app/io.github.garrett_webb.terminalpolyominos/data/tpoly/scores/`

## Sandbox

The Flatpak is offline and does not request network access. Config and data
stay inside the per-app sandbox via default XDG paths (`XDG_CONFIG_HOME` /
`XDG_DATA_HOME`).

| Permission | Purpose |
|---|---|
| Wayland / X11 | GTK window for `terminalpolyominos-gui` |

Kitty keyboard protocol works when running `terminalpolyominos` in a capable
external terminal; the embedded VTE window uses legacy input (expected).

## Upstream

- Homepage: https://github.com/Garrett-Webb/terminalpolyominos
- License: GPL-3.0-or-later
- Issue tracker: https://github.com/Garrett-Webb/terminalpolyominos/issues
