#include "game/Bag.hpp"

#include <algorithm>

namespace tp {
namespace {

constexpr std::array<PieceType, 7> kAllPieces = {
    PieceType::I, PieceType::O, PieceType::T, PieceType::S,
    PieceType::Z, PieceType::J, PieceType::L,
};

}  // namespace

Bag::Bag(std::uint64_t seed, Randomizer mode) : mode_(mode) { reseed(seed); }

void Bag::set_randomizer(Randomizer mode) {
  mode_ = mode;
  // Force a refill on the next draw so the new mode takes effect cleanly.
  index_ = size_;
}

void Bag::reseed(std::uint64_t seed) {
  rng_.seed(seed);
  index_ = 8;  // force refill for bag modes
  if (mode_ != Randomizer::FullRandom) {
    refill();
  }
}

void Bag::refill() {
  for (int i = 0; i < 7; ++i) {
    bag_[static_cast<std::size_t>(i)] = kAllPieces[static_cast<std::size_t>(i)];
  }
  size_ = 7;

  if (mode_ == Randomizer::SevenPlusOne) {
    std::uniform_int_distribution<int> pick(0, 6);
    bag_[7] = kAllPieces[static_cast<std::size_t>(pick(rng_))];
    size_ = 8;
  }

  std::shuffle(bag_.begin(), bag_.begin() + size_, rng_);
  index_ = 0;
}

PieceType Bag::next() {
  if (mode_ == Randomizer::FullRandom) {
    std::uniform_int_distribution<int> pick(0, 6);
    return kAllPieces[static_cast<std::size_t>(pick(rng_))];
  }
  if (index_ >= size_) {
    refill();
  }
  return bag_[static_cast<std::size_t>(index_++)];
}

}  // namespace tp
