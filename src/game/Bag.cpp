#include "game/Bag.hpp"

#include "game/ShapeGen.hpp"

#include <algorithm>

namespace tp {
namespace {

constexpr std::array<PieceType, 7> kAllPieces = {
    PieceType::I, PieceType::O, PieceType::T, PieceType::S,
    PieceType::Z, PieceType::J, PieceType::L,
};

constexpr std::array<PieceType, 5> kTortureFill = {
    PieceType::I, PieceType::O, PieceType::T, PieceType::J, PieceType::L,
};

bool is_bagless(Randomizer mode) {
  return mode == Randomizer::FullRandom || mode == Randomizer::Freak;
}

}  // namespace

Bag::Bag(std::uint64_t seed, Randomizer mode) : mode_(mode) { reseed(seed); }

void Bag::set_randomizer(Randomizer mode) {
  mode_ = mode;
  index_ = size_;
}

void Bag::reseed(std::uint64_t seed) {
  rng_.seed(seed);
  index_ = kMaxBagSize;  // force refill for bag modes
  if (!is_bagless(mode_)) {
    refill();
  }
}

void Bag::refill_for_test() { refill(); }

void Bag::refill() {
  size_ = 0;

  if (mode_ == Randomizer::Torture) {
    for (int i = 0; i < 10; ++i) {
      bag_[static_cast<std::size_t>(size_++)] = PieceSpec::classic(PieceType::S);
    }
    for (int i = 0; i < 10; ++i) {
      bag_[static_cast<std::size_t>(size_++)] = PieceSpec::classic(PieceType::Z);
    }
    std::uniform_int_distribution<int> pick(0, 4);
    for (int i = 0; i < 5; ++i) {
      bag_[static_cast<std::size_t>(size_++)] =
          PieceSpec::classic(kTortureFill[static_cast<std::size_t>(pick(rng_))]);
    }
  } else if (mode_ == Randomizer::Funk) {
    for (int i = 0; i < 7; ++i) {
      bag_[static_cast<std::size_t>(size_++)] =
          PieceSpec::classic(kAllPieces[static_cast<std::size_t>(i)]);
    }
    bag_[static_cast<std::size_t>(size_++)] = generate_shape(rng_);
  } else {
    // SevenBag / SevenPlusOne (and safe default)
    for (int i = 0; i < 7; ++i) {
      bag_[static_cast<std::size_t>(size_++)] =
          PieceSpec::classic(kAllPieces[static_cast<std::size_t>(i)]);
    }
    if (mode_ == Randomizer::SevenPlusOne) {
      std::uniform_int_distribution<int> pick(0, 6);
      bag_[static_cast<std::size_t>(size_++)] =
          PieceSpec::classic(kAllPieces[static_cast<std::size_t>(pick(rng_))]);
    }
  }

  std::shuffle(bag_.begin(), bag_.begin() + size_, rng_);
  index_ = 0;
}

PieceSpec Bag::next() {
  if (mode_ == Randomizer::FullRandom) {
    std::uniform_int_distribution<int> pick(0, 6);
    return PieceSpec::classic(kAllPieces[static_cast<std::size_t>(pick(rng_))]);
  }
  if (mode_ == Randomizer::Freak) {
    return generate_shape(rng_);
  }
  if (index_ >= size_) {
    refill();
  }
  return bag_[static_cast<std::size_t>(index_++)];
}

}  // namespace tp
