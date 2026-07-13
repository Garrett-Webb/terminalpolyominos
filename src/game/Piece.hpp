#pragma once

#include "game/Types.hpp"

namespace tp {

// Fills out[0..out_n) with cell offsets for the given rotation.
void piece_cells(const PieceSpec& spec, int rotation, Offset out[], int& out_n);

// Classic-only helper (always 4 cells).
void piece_cells(PieceType type, int rotation, Offset out[4]);

// ANSI 16-color index suggestion for rendering (0–7 / bright).
int piece_color(PieceType type);

const char* piece_name(PieceType type);

// Width/height of the piece's axis-aligned bbox at rotation 0 (for spawn centering).
void piece_bbox(const PieceSpec& spec, int rotation, int& w, int& h);

}  // namespace tp
