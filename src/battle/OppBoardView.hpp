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

struct BattleHud {
  int pending_incoming = 0;
  int last_sent = 0;
  int last_cancelled = 0;
  bool offline = true;
  std::string_view banner{};
};

}  // namespace tp

