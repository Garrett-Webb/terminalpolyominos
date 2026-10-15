#include "game/BattleAttack.hpp"

#include <algorithm>

namespace tp {

int combo_attack_bonus(int combo_after_clear) {
  if (combo_after_clear <= 1) {
    return 0;
  }
  if (combo_after_clear <= 3) {
    return 1;
  }
  if (combo_after_clear <= 5) {
    return 2;
  }
  if (combo_after_clear <= 7) {
    return 3;
  }
  return 4;
}

namespace {

int base_send(int lines_cleared, SpinType spin) {
  if (lines_cleared <= 0) {
    return 0;
  }
  if (spin == SpinType::Mini) {
    switch (lines_cleared) {
      case 1:
        return 1;
      case 2:
        return 2;
      default:
        break;
    }
    return 0;
  }
  if (spin == SpinType::Full) {
    switch (lines_cleared) {
      case 1:
        return 2;
      case 2:
        return 4;
      case 3:
        return 6;
      default:
        break;
    }
    return 0;
  }
  // Ordinary clear (no spin).
  switch (lines_cleared) {
    case 1:
      return 0;
    case 2:
      return 1;
    case 3:
      return 2;
    case 4:
    case 5:
      return 4;
    default:
      break;
  }
  return 0;
}

}  // namespace

AttackResult compute_send(const AttackInput& in) {
  AttackResult r;
  r.base = base_send(in.lines_cleared, in.spin);
  if (in.b2b_bonus && r.base > 0) {
    r.b2b_bonus = 1;
  }
  r.combo_bonus = combo_attack_bonus(in.combo_after_clear);
  if (in.perfect_clear && in.lines_cleared > 0) {
    r.pc_bonus = 6;
  }
  r.total_send = r.base + r.b2b_bonus + r.combo_bonus + r.pc_bonus;
  return r;
}

int apply_cancel(int pending_in, int send, int* cancelled_out) {
  const int pend = std::max(0, pending_in);
  const int snd = std::max(0, send);
  const int cancelled = std::min(pend, snd);
  if (cancelled_out != nullptr) {
    *cancelled_out = cancelled;
  }
  return snd - cancelled;
}

}  // namespace tp
