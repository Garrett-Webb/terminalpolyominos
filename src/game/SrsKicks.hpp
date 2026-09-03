#pragma once

#include "game/Board.hpp"
#include "game/Types.hpp"

namespace tp {

inline constexpr int kMaxKickTests = 32;

struct Kick {
  int dx = 0;
  int dy = 0;

  friend bool operator==(const Kick& a, const Kick& b) {
    return a.dx == b.dx && a.dy == b.dy;
  }
};

enum class SpinType : std::uint8_t {
  None = 0,
  Mini,
  Full,
};


int srs_kick_tests(PieceType kind, int from_rot, int to_rot, Kick out[kMaxKickTests]);

[[nodiscard]] SpinType classify_tspin(const Board& board, int piece_x, int piece_y, int rotation,
                                      int kick_dx, int kick_dy);

}  // namespace tp
