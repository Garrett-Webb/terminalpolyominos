#pragma once

#include "game/SrsKicks.hpp"

namespace tp {

struct AttackInput {
  int lines_cleared = 0;
  SpinType spin = SpinType::None;
  bool b2b_bonus = false;       // difficult clear while B2B was already armed
  int combo_after_clear = 0;    // combo counter after this clear (solo semantics)
  bool perfect_clear = false;   // board empty after collapse
};

struct AttackResult {
  int base = 0;
  int b2b_bonus = 0;
  int combo_bonus = 0;
  int pc_bonus = 0;
  int total_send = 0;
};

[[nodiscard]] AttackResult compute_send(const AttackInput& in);

// cancelled = min(pending_in, send); returns outgoing = send - cancelled.
[[nodiscard]] int apply_cancel(int pending_in, int send, int* cancelled_out = nullptr);

[[nodiscard]] int combo_attack_bonus(int combo_after_clear);

}  // namespace tp
