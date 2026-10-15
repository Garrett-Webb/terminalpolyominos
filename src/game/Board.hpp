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

  int clear_full_lines();

  // Shift stack toward the top (y=0); bottom row cleared. Cells in row 0 are discarded.
  void shift_up(int rows = 1);

  // One nearly-full garbage row at the bottom with a single hole. Clamps hole_col to 0..9.
  void inject_garbage_row(int hole_col, int color = -1);

  [[nodiscard]] bool line_full(int y) const;
  [[nodiscard]] bool empty() const;

  [[nodiscard]] static constexpr int width() { return kBoardWidth; }
  [[nodiscard]] static constexpr int height() { return kBoardHeight; }

 private:
  std::array<Cell, kBoardWidth * kBoardHeight> cells_{};

  [[nodiscard]] int index(int x, int y) const { return y * kBoardWidth + x; }
};

}  // namespace tp
