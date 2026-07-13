#pragma once

#include "game/Types.hpp"

namespace tp {

struct Offset {
  int x = 0;
  int y = 0;
};

// Four cells for the given type/rotation (SRS-like orientations).
void piece_cells(PieceType type, int rotation, Offset out[4]);

// ANSI 16-color index suggestion for rendering (0–7).
int piece_color(PieceType type);

const char* piece_name(PieceType type);

}  // namespace tp
