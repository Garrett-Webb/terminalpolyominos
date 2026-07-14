#include "game/Board.hpp"

#include <cstddef>

namespace tp {

Board::Board() { clear(); }

void Board::clear() {
  for (auto& c : cells_) {
    c = Cell{};
  }
}

bool Board::inside(int x, int y) const {
  return x >= 0 && x < kBoardWidth && y >= 0 && y < kBoardHeight;
}

bool Board::occupied(int x, int y) const {
  if (!inside(x, y)) {
    return true;
  }
  return cells_[static_cast<std::size_t>(index(x, y))].filled;
}

const Cell& Board::at(int x, int y) const {
  return cells_[static_cast<std::size_t>(index(x, y))];
}

Cell& Board::at(int x, int y) {
  return cells_[static_cast<std::size_t>(index(x, y))];
}

void Board::set(int x, int y, PieceType type, int color) {
  Cell& c = at(x, y);
  c.filled = true;
  c.type = type;
  if (color < 0) {
    c.color = 7;
  } else if (color > 255) {
    c.color = 15;
  } else {
    c.color = static_cast<std::uint8_t>(color);
  }
}

void Board::clear_cell(int x, int y) {
  at(x, y) = Cell{};
}

bool Board::line_full(int y) const {
  if (y < 0 || y >= kBoardHeight) {
    return false;
  }
  for (int x = 0; x < kBoardWidth; ++x) {
    if (!at(x, y).filled) {
      return false;
    }
  }
  return true;
}

int Board::clear_full_lines() {
  int cleared = 0;
  for (int y = kBoardHeight - 1; y >= 0;) {
    if (!line_full(y)) {
      --y;
      continue;
    }
    ++cleared;
    for (int yy = y; yy > 0; --yy) {
      for (int x = 0; x < kBoardWidth; ++x) {
        at(x, yy) = at(x, yy - 1);
      }
    }
    for (int x = 0; x < kBoardWidth; ++x) {
      clear_cell(x, 0);
    }
    // Stay on same y to re-check the row that fell into place.
  }
  return cleared;
}

}  // namespace tp
