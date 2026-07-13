#pragma once

#include "game/Types.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace tp {

// Piece randomizer with an explicit seed for tests/replays.
class Bag {
 public:
  explicit Bag(std::uint64_t seed = 1, Randomizer mode = Randomizer::SevenBag);

  void set_randomizer(Randomizer mode);
  [[nodiscard]] Randomizer randomizer() const { return mode_; }

  void reseed(std::uint64_t seed);
  PieceSpec next();

  // Test helper: force refill and expose current bag contents.
  void refill_for_test();
  [[nodiscard]] int bag_size() const { return size_; }
  [[nodiscard]] PieceSpec bag_at(int i) const { return bag_[static_cast<std::size_t>(i)]; }

 private:
  void refill();

  Randomizer mode_ = Randomizer::SevenBag;
  std::mt19937_64 rng_;
  std::array<PieceSpec, kMaxBagSize> bag_{};
  int size_ = 7;
  int index_ = 7;
};

}  // namespace tp
