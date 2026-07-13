#pragma once

#include "game/Board.hpp"
#include "game/Types.hpp"

namespace tp {

// Classic SRS uses 5 tests; custom polyominoes need a larger freestyle set.
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

// Kick tests in *code* coordinates (y+ down).
// Classic: SRS JLSTZ / I kick tables. O → (0,0) only.
// Custom: freestyle wall/floor/ceiling kicks (no spin scoring).
int srs_kick_tests(PieceType kind, int from_rot, int to_rot, Kick out[kMaxKickTests]);

// 3-corner T rule + Mini/Full facing; 1×2 kick forces Full.
[[nodiscard]] SpinType classify_tspin(const Board& board, int piece_x, int piece_y, int rotation,
                                      int kick_dx, int kick_dy);

}  // namespace tp
