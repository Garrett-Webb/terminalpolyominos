#pragma once

#include "game/Types.hpp"

#include <random>

namespace tp {

PieceSpec generate_shape(std::mt19937_64& rng);

}  // namespace tp
