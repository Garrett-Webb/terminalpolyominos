#pragma once

#include "game/Types.hpp"

#include <random>

namespace tp {

// Edge-connected polyomino: 3–5 cells (weights 30/40/30), bbox ≤ 4×4.
PieceSpec generate_shape(std::mt19937_64& rng);

}  // namespace tp
