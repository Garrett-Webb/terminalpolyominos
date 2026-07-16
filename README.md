# terminalpolyominos

A lightweight polyomino stacking game for Unix terminals. Written in C++20 with
CMake, **GPL-3.0-or-later**, and **no third-party libraries** - just the C++
standard library and POSIX termios. The AppImage is about **1 MB**.

Binary: **`terminalpolyominos`** · short name: **`tpoly`**

![Gameplay: hold, playfield with ghost piece, next queue, and score](docs/gameplay.png)

Run it **inside a terminal** (real TTY required). It scales the playfield to your
window, keeps redraws low-flicker, and aims for snappy input without busy-spinning
the CPU.

**Gameplay highlights**

- Hold, ghost piece, lock delay, soft / hard / sonic drop
- Classic **SRS** wall-kick rotation with **T-piece spin** Mini / Full scoring
- **Back-to-back** (×1.5 on difficult clears) and **combo** (`50 × n × level`)
- Remappable keys and timing via an in-game settings menu (`o`)
- Six piece randomizers - from classic 7-bag to generated “freak” polyominoes
- Play modes: **Endless**, **Marathon** (150 lines), **Sprint** (40 lines)
- **Local high scores** per randomizer × mode (`h` on title; `~/.local/share/tpoly/scores`)
- NEXT queue length 1–5; line-clear and hard-drop flash polish
- **256-color** piece palette when `TERM` supports it (orange L / yellow O); 16-color fallback; `NO_COLOR` / `TERM=dumb` respected
- Config under `~/.config/tpoly/.tpolyrc` (XDG)

## Play - download and run

1. Grab the build for your OS from
   **[Releases](https://github.com/Garrett-Webb/terminalpolyominos/releases/latest)**:
   - **Linux:** `.AppImage` (`x86_64` or `aarch64`)
   - **macOS:** `.tar.gz` (`macos-arm64` or `macos-x86_64`)
   - **Windows:** use WSL + the Linux AppImage (see below)
2. Run it **inside a terminal**:

**Linux**

```bash
chmod +x terminalpolyominos-*-$(uname -m).AppImage
./terminalpolyominos-*-$(uname -m).AppImage
```

**macOS**

```bash
tar -xzf terminalpolyominos-*-macos-$(uname -m).tar.gz
chmod +x terminalpolyominos
./terminalpolyominos
```

If macOS Gatekeeper blocks the binary: right-click -> Open, or
`xattr -dr com.apple.quarantine terminalpolyominos`.

**Windows**

There is not now and will never be a native Windows build - use **[WSL](https://learn.microsoft.com/windows/wsl/install)** (Ubuntu is fine). In a WSL terminal, download the matching Linux AppImage from
**[Releases](https://github.com/Garrett-Webb/terminalpolyominos/releases/latest)** (`x86_64` or `aarch64`), then:

```bash
chmod +x terminalpolyominos-*-$(uname -m).AppImage
./terminalpolyominos-*-$(uname -m).AppImage
```

Run it inside the WSL terminal (Windows Terminal -> your distro), not from `cmd.exe` / PowerShell alone. If the AppImage fails to mount, build from source in WSL instead (see Develop below).

---

### Controls (defaults)

| Key | Action |
|---|---|
| Enter / r | Start / restart |
| o | Settings |
| ←->↓ / a d s / h l j | Move / soft drop |
| ↑ | Hard drop (locks) |
| Space | Sonic drop (no lock) |
| w k x | Rotate CW |
| z | Rotate CCW |
| c | Hold |
| p / Esc | Pause |
| q | Quit (title / pause / game over) |

Settings and keybinds are editable in-game (`o`) and saved to `~/.config/tpoly/.tpolyrc`.  
Set `NO_COLOR=1` or use `TERM=dumb` to disable color.

### Movement feel (DAS / ARR)

Holding left/right/soft-drop uses three timing knobs (Settings -> **Move interval**, **Soft-drop interval**, **DAS / release**):

| Setting | Default | What it does |
|---|---|---|
| **DAS / release** (`release_ms`) | **90 ms** | How long you must hold before auto-shift starts |
| **Move interval** (`move_interval_ms`) | **35 ms** | How fast it keeps shifting once auto-shift is going |
| **Soft-drop interval** (`soft_drop_interval_ms`) | **35 ms** | Same idea for holding soft drop |

**On a Kitty-capable terminal** (direct Alacritty/Kitty/Ghostty/… - not tmux/Zellij):

1. Tap -> **one** cell, always.
2. Keep holding -> after **DAS / release**, cells keep coming every **move** (or **soft-drop**) **interval**.
3. Let go -> stop immediately (true key-up).
4. **Rotate (or other taps) while holding** still work - the held shift/soft-drop keeps going because the terminal reports each key’s press and release separately.

So: raise **DAS / release** if single taps accidentally turn into a slide; lower it if the hold feels sluggish to “catch.” Raise **move / soft-drop interval** for a slower charge; lower for a snappier ARR. The two are independent - a long DAS with a fast interval is “patient start, then zip.”

**Without** that keyboard protocol (or inside most multiplexers): the OS only repeats the last key, so hold-to-move is approximate (`release_ms` is a silence timeout, not a clean DAS), and tapping rotate while holding usually cancels the slide. Prefer running outside tmux/Zellij for the full feel.

---

### Randomizers

Pick one in Settings (`o`) or set `randomizer=` in the rc file:

| Mode | Token | What you get |
|---|---|---|
| **7-bag** (default) | `7bag` | Shuffle of all seven classic pieces; refill when empty |
| **7+1 bag** | `7+1` | All seven plus one extra classic, shuffle eight |
| **Full random** | `random` | Each piece picked independently from the seven |
| **Torture** | `torture` | Bag of 25: **10×S**, **10×Z**, and **5** from `{I,O,T,J,L}` |
| **Funk** | `funk` | Seven classics plus **one generated** shape, shuffled |
| **Freak** | `freak` | **Every** piece is freshly generated |

**Generated shapes** (funk / freak) are edge-connected polyominoes with **3–5** cells (about **30% / 40% / 30%**), kept inside a **4×4** box so they still feel standard-sized. They paint white and show as `?` on the game-over stats.

---

### Rotation & scoring

Classic **I O T S Z J L** use **SRS** wall-kick tables (separate **JLSTZ** and **I** tests; **O** does not kick). Generated **Custom** pieces (funk / freak) use a larger freestyle kick set - wall, floor, and ceiling offsets up to 3 cells, ordered small-first with CW/CCW bias - but **no** spin score multipliers.

Custom piece colors (when **Freak colors** is on, default): each shape hashes onto a
**mid/bright** 256-color cube index (dark cube corners are skipped) when 256-color is
available, else bright ANSI 8–15. Turn the setting **off** for plain white customs if
they clash with your terminal theme. Ghost pieces draw as a colored outline without
SGR dimming so they stay visible on transparent terminals.

**T-piece spins** (T only): if the last successful action before lock was a rotate, and at least three of the four diagonal corners around the T’s center are filled (walls count):

| | Condition |
|---|---|
| **Full** | Both front corners filled, **or** the successful kick was a 1×2 offset |
| **Mini** | Otherwise (typically one front + two back) |

A move, soft/sonic/hard drop after the rotate cancels the spin (even with 0 cells of travel).

| Clear | Base × level | Notes |
|---|---|---|
| Single / Double / Triple / Quad | 100 / 300 / 500 / 800 | Ordinary |
| Mini T-piece spin (0 / 1 / 2 lines) | 100 / 200 / 400 | Difficult if lines > 0 |
| T-piece spin (0 / 1 / 2 / 3 lines) | 400 / 800 / 1200 / 1600 | Difficult if lines > 0 |
| Soft / sonic / hard drop | +1 / +2 per cell | Not multiplied by B2B |

**Difficult** clears (**Quad**, or any T-piece spin that clears lines) arm **B2B**: the next difficult clear scores **×1.5**. Ordinary singles/doubles/triples break the chain; 0-line locks do not. **Combo** awards `50 × combo × level` on consecutive line-clearing locks (first clear in a streak has no combo bonus).

HUD shows **Combo** and a **B2B** tag when the chain is armed.




## Develop - build and modify

### Requirements

- C++20 compiler (g++ or clang++)
- CMake ≥ 3.20
- Unix / Linux TTY

### Build & run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/terminalpolyominos
```

### Tests

```bash
ctest --test-dir build --output-on-failure
# or: ./build/tp_tests
```

### Install (optional)

```bash
cmake --install build --prefix ~/.local
# installs terminalpolyominos and a tpoly symlink into ~/.local/bin
```

### Layout

| Path | Role |
|---|---|
| `src/game/` | Pure engine (`tp_game`) - bag, kick tables, T-piece spin / B2B / combo, board |
| `src/app/` | Main loop, screens, settings menu |
| `src/terminal/` | Raw TTY, ANSI, key decode |
| `src/input/` | Keybinds -> actions |
| `src/render/` | Layout + diff canvas |
| `src/util/` | XDG paths, `.tpolyrc` |
| `tests/` | Engine / input / settings tests |
| `packaging/appimage/` | AppImage build script + metadata |

### Config file

```text
~/.config/tpoly/.tpolyrc
```

(`$XDG_CONFIG_HOME/tpoly/.tpolyrc` if set.) Legacy `~/.tpolyrc` is read if the XDG file is missing.

Timing keys: `move_interval_ms`, `soft_drop_interval_ms`, `release_ms`, `lock_delay_ms`, `lines_per_level`, `next_count` (1–5), `randomizer` (`7bag`, `7+1`, `random`, `torture`, `funk`, `freak`), `play_mode` (`endless`, `marathon`, `sprint`), `freak_colors` (`on` / `off`), `keyboard_protocol` (`auto` / `kitty` / `legacy`).

Keybind keys (comma-separated tokens, e.g. `left,a`): `key_left`, `key_right`, `key_soft_drop`, `key_hard_drop`, `key_sonic_drop`, `key_rotate_cw`, `key_rotate_ccw`, `key_hold`, `key_pause`, `key_quit`, `key_restart`, `key_settings`, `key_scores` (default `h`).

High scores (separate from rc): `~/.local/share/tpoly/scores` - top **5** per **randomizer × play mode** (sections like `7bag`, `7bag:marathon`, `7bag:sprint`). Title **`h`** (or your `key_scores` bind) opens a per-randomizer page listing Endless / Marathon / Sprint (Left/Right cycles randomizers). Enter on the title always starts a run; remapped play keys do not steal that. Qualifying game overs / clears prompt for a name (prefilled from `$USER`; Enter saves, Esc discards). Settings -> **Clear high scores** requires typing **`yes`**.

**Input note:** See [Movement feel (DAS / ARR)](#movement-feel-das--arr) above. Kitty keyboard protocol (`keyboard_protocol=auto`) enables true press/release when the terminal supports it; set `legacy` to force the OS-repeat model.

**Multiplexers (tmux / Zellij):** Enhanced keys need the protocol end-to-end. **tmux** (as of common releases) does **not** fully implement Kitty progressive enhancement for pane apps, so tpoly falls back to legacy OS-repeat inside tmux - even when the outer terminal is Alacritty/Kitty. Same idea for Zellij unless it fully forwards KKP. For hold-move + rotate DAS, run tpoly **directly in the terminal**.

### Display scaling

The play layout (HOLD + field + NEXT + hint row) picks the **largest integer scale**
from **1…12** that fits the current terminal size. Each board cell is drawn as a
block of **`3×scale` columns by `scale` rows** (3:1 aspect so squares look square in
a typical monospace font). Side panels grow with the same scale (inner width at least
`max(4×cell_w, 14)`).

| Scale | Cell (cols×rows) | Needs at least (cols×rows) |
|---:|---|---|
| 1 | 3×1 | **68×24** (minimum) |
| 2 | 6×2 | 118×44 |
| 3 | 9×3 | 172×64 |
| 4 | 12×4 | 226×84 |
| … | … | … |
| 12 | 36×12 | 658×244 (maximum) |

Below **68×24** the game shows a “too small” screen (resize or `q` to quit). There is
no scale above 12 even on huge terminals; extra space just centers the layout.
Title / scores / settings screens use the same size gate.

---

## License

[GPL-3.0-or-later](LICENSE) - modifications you distribute must stay open source under the same license.
