#include "game/Piece.hpp"

namespace tp {
namespace {

// Orientations with y+ down. Values are offsets from the piece origin (x,y).
// Layouts follow common SRS shapes (I uses a 4-wide bar; O a 2x2).
constexpr Offset kCells[7][4][4] = {
    // I
    {
        {{0, 1}, {1, 1}, {2, 1}, {3, 1}},
        {{2, 0}, {2, 1}, {2, 2}, {2, 3}},
        {{0, 2}, {1, 2}, {2, 2}, {3, 2}},
        {{1, 0}, {1, 1}, {1, 2}, {1, 3}},
    },
    // O
    {
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
    },
    // T
    {
        {{1, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {1, 2}},
        {{1, 0}, {0, 1}, {1, 1}, {1, 2}},
    },
    // S
    {
        {{1, 0}, {2, 0}, {0, 1}, {1, 1}},
        {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
        {{0, 0}, {0, 1}, {1, 1}, {1, 2}},
    },
    // Z
    {
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
        {{2, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
        {{1, 0}, {0, 1}, {1, 1}, {0, 2}},
    },
    // J
    {
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 0}, {1, 1}, {0, 2}, {1, 2}},
    },
    // L
    {
        {{2, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {0, 2}},
        {{0, 0}, {1, 0}, {1, 1}, {1, 2}},
    },
};

}  // namespace

void piece_cells(PieceType type, int rotation, Offset out[4]) {
  const int t = static_cast<int>(type);
  const int r = ((rotation % 4) + 4) % 4;
  for (int i = 0; i < 4; ++i) {
    out[i] = kCells[t][r][i];
  }
}

int piece_color(PieceType type) {
  switch (type) {
    case PieceType::I:
      return 6;  // cyan
    case PieceType::O:
      return 3;  // yellow
    case PieceType::T:
      return 5;  // magenta
    case PieceType::S:
      return 2;  // green
    case PieceType::Z:
      return 1;  // red
    case PieceType::J:
      return 4;  // blue
    case PieceType::L:
      return 3;  // yellow/orange stand-in
    case PieceType::Count:
      break;
  }
  return 7;
}

const char* piece_name(PieceType type) {
  switch (type) {
    case PieceType::I:
      return "I";
    case PieceType::O:
      return "O";
    case PieceType::T:
      return "T";
    case PieceType::S:
      return "S";
    case PieceType::Z:
      return "Z";
    case PieceType::J:
      return "J";
    case PieceType::L:
      return "L";
    case PieceType::Count:
      break;
  }
  return "?";
}

}  // namespace tp
