# App icon for Flatpak / desktop

Drop your icon file(s) here before building or submitting to Flathub.

## Required (pick one)

| Format | Size | Filename |
|---|---|---|
| **PNG (recommended)** | **256×256** (square) | `io.github.garrett_webb.terminalpolyominos.png` |
| **SVG (also fine)** | scalable | `io.github.garrett_webb.terminalpolyominos.svg` |

Flathub minimum: **256×256 square PNG** or **SVG**.

## Optional (look sharper in menus)

If you only make one PNG, the build script can resize from 256×256. Or export these yourself:

| Size | Filename |
|---|---|
| 128×128 | `io.github.garrett_webb.terminalpolyominos-128.png` |
| 512×512 | `io.github.garrett_webb.terminalpolyominos-512.png` |

## Design tips (Flathub quality guidelines)

- Square canvas; **transparent background** works well.
- Leave a little margin — don’t fill edge-to-edge.
- No baked-in drop shadow (the desktop draws shadows).
- Keep it readable at 64×64 (launcher size).
- Avoid tiny text.

## Tools

Inkscape, GIMP, Krita, or any editor that exports PNG/SVG.

If no icon is present, `build.sh` generates a temporary placeholder PNG for local testing only — **replace before Flathub submission**.
