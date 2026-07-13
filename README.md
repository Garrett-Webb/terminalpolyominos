# terminalpolyominos

Lightweight, colored polyomino stacking game for Unix terminals. Written in C++20, built with CMake. Ships as an **AppImage**.

Binary: **`terminalpolyominos`** · alias: **`tpoly`**

> Playable with software hold-move, low-flicker synced frames, and a playfield that scales with the terminal size.

Planning docs: `~/wiki/terminalpolyominos`.

## Download

From [GitHub Releases](https://github.com/Garrett-Webb/terminalpolyominos/releases/latest):

```bash
chmod +x terminalpolyominos-*-$(uname -m).AppImage
./terminalpolyominos-*-$(uname -m).AppImage   # run inside a terminal
```

Or build an AppImage locally: `./packaging/appimage/build.sh` → `dist/`.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/terminalpolyominos
```

Quit with `q` from the title, pause, or game-over screens (`p`/`Esc` to pause while playing).

## Controls

| Key | Action |
|---|---|
| Enter / r | Start / restart |
| o | Settings |
| ↑ | Hard drop (lock) |
| ←→↓ / a d s / h l j | Move / soft drop |
| w k x | Rotate CW |
| z | Rotate CCW |
| Space | Sonic drop (no lock) |
| c | Hold |
| p / Esc | Pause |
| q | Quit (title / pause / game over) |

## Tests

```bash
cmake --build build
ctest --test-dir build --output-on-failure
# or: ./build/tp_tests
```

## Install (optional)

```bash
cmake --install build --prefix ~/.local
# puts terminalpolyominos and symlink tpoly into ~/.local/bin
```

## Requirements

- C++20 compiler (g++ or clang++)
- CMake ≥ 3.20
- A Unix TTY (no extra libraries)

Honor `NO_COLOR` to disable ANSI colors. `TERM=dumb` also disables color.

## Config

Settings live in an rc file (created with defaults on first run):

```text
~/.config/tpoly/.tpolyrc
```

(`$XDG_CONFIG_HOME/tpoly/.tpolyrc` if set.) A legacy `~/.tpolyrc` is read if the XDG file is missing. High scores will use `~/.local/share/tpoly/` later.

Timing: `move_interval_ms`, `soft_drop_interval_ms`, `release_ms`, `lock_delay_ms`, `lines_per_level`, `next_count` (1–5), `randomizer` (`7bag`, `7+1`, or `random`).

Keybinds (comma-separated tokens like `left,a,h`): `key_left`, `key_right`, `key_soft_drop`, `key_hard_drop`, `key_sonic_drop`, `key_rotate_cw`, `key_rotate_ccw`, `key_hold`, `key_pause`, `key_quit`, `key_restart`, `key_settings`.

In-game: press **`o`** from the title screen or pause to open the settings menu (Save writes the rc).

## License

GPL-3.0-or-later — see [LICENSE](LICENSE).
