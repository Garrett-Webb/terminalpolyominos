#pragma once

#include "game/Types.hpp"

#include <cstdint>
#include <string_view>

namespace tp {

struct GameState;

// Compact opponent board for Battle (visible 10×20 only).
struct OppBoardView {
  bool occupied[kVisibleRows][kBoardWidth]{};
  std::uint8_t color[kVisibleRows][kBoardWidth]{};
  bool silhouette[kVisibleRows][kBoardWidth]{};
  bool clear_flash_row[kVisibleRows]{};
  int combo = 0;
  bool b2b = false;
  int height = 0;  // highest occupied visible row distance from bottom
};

[[nodiscard]] OppBoardView make_opp_view(const GameState& state);

inline constexpr int kStackSnapBytes = 25;  // 10×20 bitpack

// Locked visible stack only (no active piece). Wire layout: row-major top→bottom, MSB-first.
void pack_stack_bits(const GameState& state, std::uint8_t out[kStackSnapBytes]);
[[nodiscard]] OppBoardView opp_view_from_stack_bits(const std::uint8_t bits[kStackSnapBytes]);

struct BattleHud {
  int pending_incoming = 0;
  int last_sent = 0;
  int last_cancelled = 0;
  bool offline = true;
  std::string_view banner{};
};

}  // namespace tp

