#pragma once

#include "game/Types.hpp"

namespace tp {

// Fills out[0..out_n) with cell offsets for the given rotation.
void piece_cells(const PieceSpec& spec, int rotation, Offset out[], int& out_n);

// Classic-only helper (always 4 cells).
void piece_cells(PieceType type, int rotation, Offset out[4]);

// Palette index for classic pieces. colors_256 selects 256 vs 16-color map.
int piece_color(PieceType type, bool colors_256 = true);
// Custom: hash into palette when freak_colors; else white (15 / 7).
int piece_color(const PieceSpec& spec, bool freak_colors = true, bool colors_256 = true);

// White flash index for the active palette.
[[nodiscard]] inline int flash_white(bool colors_256) { return colors_256 ? 15 : 7; }

const char* piece_name(PieceType type);

// Width/height of the piece's axis-aligned bbox at rotation 0 (for spawn centering).
void piece_bbox(const PieceSpec& spec, int rotation, int& w, int& h);

}  // namespace tp
