#include "game/SrsKicks.hpp"

#include <cstdlib>

namespace tp {
namespace {

// Wiki tables use y+ up. Stored here with y negated (code y+ down).
// Indexed [from][to] for the eight adjacent transitions; unused pairs unused.

constexpr Kick kJlstz[4][4][5] = {
    // from 0
    {
        {},
        {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
        {},
        {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
    },
    // from R (1)
    {
        {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
        {},
        {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
        {},
    },
    // from 2
    {
        {},
        {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
        {},
        {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
    },
    // from L (3)
    {
        {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
        {},
        {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
        {},
    },
};

constexpr Kick kI[4][4][5] = {
    {
        {},
        {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
        {},
        {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}},
    },
    {
        {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
        {},
        {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}},
        {},
    },
    {
        {},
        {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
        {},
        {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
    },
    {
        {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
        {},
        {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
        {},
    },
};

bool adjacent(int from, int to) {
  return ((from + 1) & 3) == to || ((from + 3) & 3) == to;
}

void copy_n(const Kick* src, int count, Kick out[kMaxKickTests]) {
  for (int i = 0; i < count; ++i) {
    out[i] = src[i];
  }
}

// Freestyle kicks for arbitrary polyominoes (≤4×4). Tries small wall/floor/ceiling
// offsets before larger ones. CW prefers leftward first; CCW prefers rightward —
// similar bias to JLSTZ — and floor (y+) before ceiling (y−).
int fill_custom_kicks(bool cw, Kick out[kMaxKickTests]) {
  const int hx = cw ? -1 : 1;  // preferred horizontal sign
  int n = 0;

  auto already = [&](int dx, int dy) {
    for (int i = 0; i < n; ++i) {
      if (out[i].dx == dx && out[i].dy == dy) {
        return true;
      }
    }
    return false;
  };

  auto push = [&](int dx, int dy) {
    if (n >= kMaxKickTests || already(dx, dy)) {
      return;
    }
    out[n++] = {dx, dy};
  };

  push(0, 0);

  // Manhattan rings 1..4, axis extent capped at 3 (enough for 4×4 bbox pivots).
  for (int man = 1; man <= 4; ++man) {
    Kick ring[64];
    int rn = 0;
    for (int dx = -3; dx <= 3; ++dx) {
      for (int dy = -3; dy <= 3; ++dy) {
        if (dx == 0 && dy == 0) {
          continue;
        }
        if (std::abs(dx) + std::abs(dy) != man) {
          continue;
        }
        ring[rn++] = {dx, dy};
      }
    }

    // Sort ring: prefer pure horizontal, then pure vertical, then diagonals;
    // hx-matching dx first; positive dy (floor) before negative (ceiling).
    for (int i = 0; i < rn; ++i) {
      for (int j = i + 1; j < rn; ++j) {
        auto key = [&](const Kick& k) {
          const bool horiz = k.dy == 0;
          const bool vert = k.dx == 0;
          const int axis = horiz ? 0 : (vert ? 1 : 2);
          const int hx_pref = (k.dx == 0) ? 0 : ((k.dx * hx > 0) ? 0 : 1);
          const int floor_pref = (k.dy > 0) ? 0 : ((k.dy < 0) ? 1 : 0);
          return axis * 1000 + hx_pref * 100 + floor_pref * 10 + std::abs(k.dx) + std::abs(k.dy);
        };
        if (key(ring[j]) < key(ring[i])) {
          const Kick tmp = ring[i];
          ring[i] = ring[j];
          ring[j] = tmp;
        }
      }
    }

    for (int i = 0; i < rn; ++i) {
      push(ring[i].dx, ring[i].dy);
    }
  }

  return n;
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
    const bool cw = ((from + 1) & 3) == to;
    return fill_custom_kicks(cw, out);
  }
  if (from == to || !adjacent(from, to)) {
    out[0] = {0, 0};
    return 1;
  }

  if (kind == PieceType::I) {
    copy_n(kI[from][to], 5, out);
    return 5;
  }

  copy_n(kJlstz[from][to], 5, out);
  return 5;
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
