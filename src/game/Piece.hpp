#pragma once

#include "game/Types.hpp"

namespace tp {

void piece_cells(const PieceSpec& spec, int rotation, Offset out[], int& out_n);

void piece_cells(PieceType type, int rotation, Offset out[4]);

int piece_color(PieceType type, bool colors_256 = true);

int piece_color(const PieceSpec& spec, bool freak_colors = true, bool colors_256 = true);

[[nodiscard]] inline int flash_white(bool colors_256) { return colors_256 ? 15 : 7; }

const char* piece_name(PieceType type);

void piece_bbox(const PieceSpec& spec, int rotation, int& w, int& h);

}  // namespace tp
