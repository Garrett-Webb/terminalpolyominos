#include "game/SrsKicks.hpp"

namespace tp {
namespace {

// Wiki tables use y+ up. Stored here with y negated (code y+ down).
// Indexed [from][to] for the eight adjacent transitions; unused pairs unused.

constexpr Kick kJlstz[4][4][kMaxKickTests] = {
    // from 0
    {
        {},  // 0→0 unused
        // 0→R (wiki: (0,0) (-1,0) (-1,+1) (0,-2) (-1,-2))
        {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
        {},
        // 0→L
        {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
    },
    // from R (1)
    {
        // R→0
        {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
        {},
        // R→2
        {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
        {},
    },
    // from 2
    {
        {},
        // 2→R
        {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
        {},
        // 2→L
        {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
    },
    // from L (3)
    {
        // L→0
        {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
        {},
        // L→2
        {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
        {},
    },
};

constexpr Kick kI[4][4][kMaxKickTests] = {
    {
        {},
        // 0→R (wiki: (0,0) (-2,0) (+1,0) (-2,-1) (+1,+2))
        {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
        {},
        // 0→L
        {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}},
    },
    {
        // R→0
        {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
        {},
        // R→2
        {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}},
        {},
    },
    {
        {},
        // 2→R
        {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
        {},
        // 2→L
        {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
    },
    {
        // L→0
        {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
        {},
        // L→2
        {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
        {},
    },
};

constexpr Kick kCustomHorizontal[kMaxKickTests] = {
    {0, 0}, {-1, 0}, {1, 0}, {-2, 0}, {2, 0},
};

bool adjacent(int from, int to) {
  return ((from + 1) & 3) == to || ((from + 3) & 3) == to;
}

void copy_five(const Kick src[kMaxKickTests], Kick out[kMaxKickTests]) {
  for (int i = 0; i < kMaxKickTests; ++i) {
    out[i] = src[i];
  }
}

}  // namespace

int srs_kick_tests(PieceType kind, int from_rot, int to_rot, Kick out[kMaxKickTests]) {
  const int from = ((from_rot % 4) + 4) % 4;
  const int to = ((to_rot % 4) + 4) % 4;

  if (kind == PieceType::O) {
    out[0] = {0, 0};
    return 1;
  }
  if (kind == PieceType::Custom) {
    copy_five(kCustomHorizontal, out);
    return kMaxKickTests;
  }
  if (from == to || !adjacent(from, to)) {
    out[0] = {0, 0};
    return 1;
  }

  if (kind == PieceType::I) {
    copy_five(kI[from][to], out);
    return kMaxKickTests;
  }

  // J L S T Z (and any other classic)
  copy_five(kJlstz[from][to], out);
  return kMaxKickTests;
}

SpinType classify_tspin(const Board& board, int piece_x, int piece_y, int rotation,
                        int kick_dx, int kick_dy) {
  // SRS T center is always local (1,1).
  const int cx = piece_x + 1;
  const int cy = piece_y + 1;

  // Four diagonal corners: A=NW, B=NE, C=SW, D=SE in code coords (y+ down).
  const bool a = board.occupied(cx - 1, cy - 1);  // up-left
  const bool b = board.occupied(cx + 1, cy - 1);  // up-right
  const bool c = board.occupied(cx - 1, cy + 1);  // down-left
  const bool d = board.occupied(cx + 1, cy + 1);  // down-right
  const int corners = static_cast<int>(a) + static_cast<int>(b) + static_cast<int>(c) +
                      static_cast<int>(d);
  if (corners < 3) {
    return SpinType::None;
  }

  const int r = ((rotation % 4) + 4) % 4;
  bool front0 = false;
  bool front1 = false;
  switch (r) {
    case 0:  // stem up → front is top pair
      front0 = a;
      front1 = b;
      break;
    case 1:  // stem right
      front0 = b;
      front1 = d;
      break;
    case 2:  // stem down
      front0 = c;
      front1 = d;
      break;
    default:  // stem left
      front0 = a;
      front1 = c;
      break;
  }

  const bool kick_1x2 = (kick_dx == 1 || kick_dx == -1) && (kick_dy == 2 || kick_dy == -2);
  if (kick_1x2) {
    return SpinType::Full;
  }
  if (front0 && front1) {
    return SpinType::Full;
  }
  return SpinType::Mini;
}

}  // namespace tp
