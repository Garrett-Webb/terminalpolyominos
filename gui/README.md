# terminalpolyominos-gui

Optional **GTK 4 + VTE** launcher for graphical installs (e.g. Flathub). Runs the
unchanged `terminalpolyominos` game binary inside an embedded terminal window.

The core game (`terminalpolyominos`) is still **stdlib-only** with no GTK/VTE
dependencies. Use your own terminal for tmux/zellij workflows.

## Build

```bash
cmake -B build-gui -DTP_BUILD_GUI=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-gui
```

Requires development packages: `gtk4-devel`, `vte291-gtk4-devel` (Fedora names).

Binaries:

- `build-gui/terminalpolyominos` — TUI game (unchanged)
- `build-gui/gui/terminalpolyominos-gui` — graphical launcher

## Run

```bash
./build-gui/gui/terminalpolyominos-gui
```

Or run `./build-gui/terminalpolyominos` inside any terminal emulator.

Default build (`TP_BUILD_GUI=OFF`) does not compile this directory.
