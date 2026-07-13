#include "game/ShapeGen.hpp"

#include <algorithm>
#include <vector>

namespace tp {
namespace {

constexpr int kDx[4] = {1, -1, 0, 0};
constexpr int kDy[4] = {0, 0, 1, -1};

int pick_cell_count(std::mt19937_64& rng) {
  // 30% / 40% / 30% for 3 / 4 / 5
  std::uniform_int_distribution<int> roll(0, 99);
  const int r = roll(rng);
  if (r < 30) {
    return 3;
  }
  if (r < 70) {
    return 4;
  }
  return 5;
}

bool contains(const std::vector<Offset>& cells, int x, int y) {
  for (const Offset& c : cells) {
    if (c.x == x && c.y == y) {
      return true;
    }
  }
  return false;
}

bool try_grow(std::mt19937_64& rng, int n, PieceSpec& out) {
  std::vector<Offset> cells;
  cells.reserve(static_cast<std::size_t>(n));
  cells.push_back({0, 0});

  while (static_cast<int>(cells.size()) < n) {
    std::vector<Offset> candidates;
    for (const Offset& c : cells) {
      for (int d = 0; d < 4; ++d) {
        const int nx = c.x + kDx[d];
        const int ny = c.y + kDy[d];
        if (!contains(cells, nx, ny) && !contains(candidates, nx, ny)) {
          candidates.push_back({nx, ny});
        }
      }
    }
    if (candidates.empty()) {
      return false;
    }
    std::uniform_int_distribution<std::size_t> pick(0, candidates.size() - 1);
    cells.push_back(candidates[pick(rng)]);
  }

  int min_x = cells[0].x;
  int min_y = cells[0].y;
  int max_x = cells[0].x;
  int max_y = cells[0].y;
  for (const Offset& c : cells) {
    min_x = std::min(min_x, c.x);
    min_y = std::min(min_y, c.y);
    max_x = std::max(max_x, c.x);
    max_y = std::max(max_y, c.y);
  }
  if (max_x - min_x + 1 > 4 || max_y - min_y + 1 > 4) {
    return false;
  }

  out.kind = PieceType::Custom;
  out.n = n;
  out.cells = {};
  for (int i = 0; i < n; ++i) {
    out.cells[static_cast<std::size_t>(i)] = {cells[static_cast<std::size_t>(i)].x - min_x,
                                              cells[static_cast<std::size_t>(i)].y - min_y};
  }
  return true;
}

}  // namespace

PieceSpec generate_shape(std::mt19937_64& rng) {
  constexpr int kMaxAttempts = 64;
  for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
    const int n = pick_cell_count(rng);
    PieceSpec spec;
    if (try_grow(rng, n, spec)) {
      return spec;
    }
  }
  // Extremely unlikely fallback: a 2×2 square (O-like custom).
  PieceSpec fallback;
  fallback.kind = PieceType::Custom;
  fallback.n = 4;
  fallback.cells[0] = {0, 0};
  fallback.cells[1] = {1, 0};
  fallback.cells[2] = {0, 1};
  fallback.cells[3] = {1, 1};
  return fallback;
}

}  // namespace tp
