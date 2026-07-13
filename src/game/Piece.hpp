#pragma once

#include "game/Types.hpp"

namespace tp {

// Fills out[0..out_n) with cell offsets for the given rotation.
void piece_cells(const PieceSpec& spec, int rotation, Offset out[], int& out_n);

// Classic-only helper (always 4 cells).
void piece_cells(PieceType type, int rotation, Offset out[4]);

// ANSI 16-color index (0–7 normal, 8–15 bright).
int piece_color(PieceType type);
// Custom shapes: hash → bright 8–15 when freak_colors, else white (7).
int piece_color(const PieceSpec& spec, bool freak_colors = true);

const char* piece_name(PieceType type);

// Width/height of the piece's axis-aligned bbox at rotation 0 (for spawn centering).
void piece_bbox(const PieceSpec& spec, int rotation, int& w, int& h);

}  // namespace tp
