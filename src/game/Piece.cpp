#include "game/Piece.hpp"

#include <algorithm>
#include <cstdint>

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

void custom_cells(const PieceSpec& spec, int rotation, Offset out[], int& out_n) {
  out_n = std::max(1, std::min(spec.n, kMaxPieceCells));
  const int r = ((rotation % 4) + 4) % 4;

  // Integer rotation around the local origin. Floating-point pivots + rounding
  // can split edge-connected polyominoes (especially L/J/T-like shapes).
  for (int i = 0; i < out_n; ++i) {
    int x = spec.cells[static_cast<std::size_t>(i)].x;
    int y = spec.cells[static_cast<std::size_t>(i)].y;
    for (int k = 0; k < r; ++k) {
      // Clockwise with y+ down: (x, y) -> (-y, x)
      const int nx = -y;
      const int ny = x;
      x = nx;
      y = ny;
    }
    out[i] = {x, y};
  }
}

}  // namespace

void piece_cells(const PieceSpec& spec, int rotation, Offset out[], int& out_n) {
  if (spec.is_custom()) {
    custom_cells(spec, rotation, out, out_n);
    return;
  }
  const int t = static_cast<int>(spec.kind);
  if (t < 0 || t >= 7) {
    out_n = 0;
    return;
  }
  out_n = 4;
  const int r = ((rotation % 4) + 4) % 4;
  for (int i = 0; i < 4; ++i) {
    out[i] = kCells[t][r][i];
  }
}

void piece_cells(PieceType type, int rotation, Offset out[4]) {
  int n = 0;
  piece_cells(PieceSpec::classic(type), rotation, out, n);
  (void)n;
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
    case PieceType::Custom:
      return 7;  // white (shape-specific color uses PieceSpec overload)
    case PieceType::Count:
      break;
  }
  return 7;
}

int piece_color(const PieceSpec& spec, bool freak_colors) {
  if (!spec.is_custom()) {
    return piece_color(spec.kind);
  }
  if (!freak_colors) {
    return 7;
  }

  // FNV-1a over sorted cell keys so equal shapes match regardless of array order.
  constexpr std::uint32_t kOffset = 2166136261u;
  constexpr std::uint32_t kPrime = 16777619u;
  std::uint8_t keys[kMaxPieceCells];
  const int n = std::max(0, std::min(spec.n, kMaxPieceCells));
  for (int i = 0; i < n; ++i) {
    const int x = spec.cells[static_cast<std::size_t>(i)].x & 7;
    const int y = spec.cells[static_cast<std::size_t>(i)].y & 7;
    keys[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(x | (y << 3));
  }
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      if (keys[static_cast<std::size_t>(j)] < keys[static_cast<std::size_t>(i)]) {
        const std::uint8_t tmp = keys[static_cast<std::size_t>(i)];
        keys[static_cast<std::size_t>(i)] = keys[static_cast<std::size_t>(j)];
        keys[static_cast<std::size_t>(j)] = tmp;
      }
    }
  }

  std::uint32_t h = kOffset;
  h ^= static_cast<std::uint32_t>(n);
  h *= kPrime;
  for (int i = 0; i < n; ++i) {
    h ^= keys[static_cast<std::size_t>(i)];
    h *= kPrime;
  }
  return 8 + static_cast<int>(h % 8);  // bright 8–15
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
    case PieceType::Custom:
      return "?";
    case PieceType::Count:
      break;
  }
  return "?";
}

void piece_bbox(const PieceSpec& spec, int rotation, int& w, int& h) {
  Offset cells[kMaxPieceCells];
  int n = 0;
  piece_cells(spec, rotation, cells, n);
  if (n <= 0) {
    w = 0;
    h = 0;
    return;
  }
  int min_x = cells[0].x;
  int max_x = cells[0].x;
  int min_y = cells[0].y;
  int max_y = cells[0].y;
  for (int i = 1; i < n; ++i) {
    min_x = std::min(min_x, cells[i].x);
    max_x = std::max(max_x, cells[i].x);
    min_y = std::min(min_y, cells[i].y);
    max_y = std::max(max_y, cells[i].y);
  }
  w = max_x - min_x + 1;
  h = max_y - min_y + 1;
}

}  // namespace tp
