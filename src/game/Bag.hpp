#pragma once

#include "game/Types.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace tp {

// Piece randomizer with an explicit seed for tests/replays.
// - SevenBag: shuffle of all 7 types
// - SevenPlusOne: all 7 plus one extra random type, then shuffle (bag of 8)
// - FullRandom: each piece independently uniform among the 7 (no bag)
class Bag {
 public:
  explicit Bag(std::uint64_t seed = 1, Randomizer mode = Randomizer::SevenBag);

  void set_randomizer(Randomizer mode);
  [[nodiscard]] Randomizer randomizer() const { return mode_; }

  void reseed(std::uint64_t seed);
  PieceType next();

 private:
  void refill();

  Randomizer mode_ = Randomizer::SevenBag;
  std::mt19937_64 rng_;
  std::array<PieceType, 8> bag_{};
  int size_ = 7;
  int index_ = 7;
};

}  // namespace tp
