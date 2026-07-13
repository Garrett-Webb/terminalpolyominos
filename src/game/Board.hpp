#pragma once

#include "game/Types.hpp"

#include <array>

namespace tp {

class Board {
 public:
  Board();

  void clear();

  [[nodiscard]] bool inside(int x, int y) const;
  [[nodiscard]] bool occupied(int x, int y) const;
  [[nodiscard]] const Cell& at(int x, int y) const;
  Cell& at(int x, int y);

  void set(int x, int y, PieceType type, int color = -1);
  void clear_cell(int x, int y);

  // Returns number of lines cleared (0–4).
  int clear_full_lines();

  [[nodiscard]] bool line_full(int y) const;

  [[nodiscard]] static constexpr int width() { return kBoardWidth; }
  [[nodiscard]] static constexpr int height() { return kBoardHeight; }

 private:
  std::array<Cell, kBoardWidth * kBoardHeight> cells_{};

  [[nodiscard]] int index(int x, int y) const { return y * kBoardWidth + x; }
};

}  // namespace tp
