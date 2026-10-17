#include "battle/OppBoardView.hpp"

#include "game/Game.hpp"
#include "game/Piece.hpp"

#include <algorithm>

namespace tp {

OppBoardView make_opp_view(const GameState& state) {
  OppBoardView v;
  int height = 0;
  for (int vr = 0; vr < kVisibleRows; ++vr) {
    const int by = vr + kBufferRows;
    v.clear_flash_row[static_cast<std::size_t>(vr)] =
        state.clear_flash_ms > 0 && state.clear_rows[static_cast<std::size_t>(by)];
    bool row_filled = false;
    for (int x = 0; x < kBoardWidth; ++x) {
      const Cell& c = state.board.at(x, by);
      v.occupied[static_cast<std::size_t>(vr)][static_cast<std::size_t>(x)] = c.filled;
      v.color[static_cast<std::size_t>(vr)][static_cast<std::size_t>(x)] = c.color;
      if (c.filled) {
        row_filled = true;
      }
    }
    if (row_filled) {
      height = std::max(height, kVisibleRows - vr);
    }
  }
  v.height = height;
  v.combo = state.combo;
  v.b2b = state.b2b_ready;

  if (state.active.alive) {
    Offset cells[kMaxPieceCells];
    int n = 0;
    piece_cells(state.active.spec, state.active.rotation, cells, n);
    for (int i = 0; i < n; ++i) {
      const int x = state.active.x + cells[i].x;
      const int y = state.active.y + cells[i].y;
      const int vr = y - kBufferRows;
      if (x >= 0 && x < kBoardWidth && vr >= 0 && vr < kVisibleRows) {
        v.silhouette[static_cast<std::size_t>(vr)][static_cast<std::size_t>(x)] = true;
      }
    }
  }
  return v;
}

void pack_stack_bits(const GameState& state, std::uint8_t out[kStackSnapBytes]) {
  for (int i = 0; i < kStackSnapBytes; ++i) {
    out[i] = 0;
  }
  for (int vr = 0; vr < kVisibleRows; ++vr) {
    const int by = vr + kBufferRows;
    for (int x = 0; x < kBoardWidth; ++x) {
      if (!state.board.at(x, by).filled) {
        continue;
      }
      const int bit_index = vr * kBoardWidth + x;
      const int byte_i = bit_index / 8;
      const int bit = 7 - (bit_index % 8);
      out[byte_i] = static_cast<std::uint8_t>(out[byte_i] | (1u << bit));
    }
  }
}

OppBoardView opp_view_from_stack_bits(const std::uint8_t bits[kStackSnapBytes]) {
  OppBoardView v;
  const std::uint8_t mono = static_cast<std::uint8_t>(piece_color(PieceType::Garbage, true));
  int height = 0;
  for (int vr = 0; vr < kVisibleRows; ++vr) {
    bool row_filled = false;
    for (int x = 0; x < kBoardWidth; ++x) {
      const int bit_index = vr * kBoardWidth + x;
      const int byte_i = bit_index / 8;
      const int bit = 7 - (bit_index % 8);
      const bool occ = (bits[byte_i] & (1u << bit)) != 0;
      v.occupied[static_cast<std::size_t>(vr)][static_cast<std::size_t>(x)] = occ;
      v.silhouette[static_cast<std::size_t>(vr)][static_cast<std::size_t>(x)] = false;
      if (occ) {
        v.color[static_cast<std::size_t>(vr)][static_cast<std::size_t>(x)] = mono;
        row_filled = true;
      }
    }
    if (row_filled) {
      height = std::max(height, kVisibleRows - vr);
    }
  }
  v.height = height;
  return v;
}

}  // namespace tp
