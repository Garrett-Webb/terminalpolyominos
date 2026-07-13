#pragma once

#include <cstdint>

namespace tp {

inline constexpr int kBoardWidth = 10;
inline constexpr int kBoardHeight = 22;   // 2 hidden + 20 visible
inline constexpr int kBufferRows = 2;
inline constexpr int kVisibleRows = 20;
inline constexpr int kNextQueueMax = 5;   // NEXT panel fits 5 when packed tightly
inline constexpr int kNextQueueDefault = 3;
inline constexpr int kLinesPerLevel = 10;
inline constexpr int kLockDelayMs = 500;
inline constexpr int kClearFlashMs = 100;  // white flash before rows collapse
inline constexpr int kHardDropFlashMs = 100;  // white flash on hard-drop lock (color only)

enum class Randomizer : std::uint8_t {
  SevenBag = 0,      // classic 7-bag
  SevenPlusOne = 1,  // 7-bag + 1 extra random piece (bag of 8)
  FullRandom = 2,    // independent uniform pick each time (no bag)
};

enum class PieceType : std::uint8_t {
  I = 0,
  O,
  T,
  S,
  Z,
  J,
  L,
  Count,
};

enum class Action : std::uint8_t {
  None = 0,
  Left,
  Right,
  SoftDrop,
  SonicDrop,  // drop to floor, do not lock
  HardDrop,   // drop to floor and lock
  RotateCW,
  RotateCCW,
  Hold,
  Pause,
  Quit,
  Restart,
  Settings,  // open settings menu (title / pause)
};

enum class Phase : std::uint8_t {
  Ready = 0,
  Playing,
  Paused,
  GameOver,
};

struct Cell {
  bool filled = false;
  PieceType type = PieceType::I;
};

struct ActivePiece {
  PieceType type = PieceType::I;
  int x = 0;
  int y = 0;
  int rotation = 0;  // 0..3
  bool alive = false;
};

struct GameConfig {
  int lock_delay_ms = kLockDelayMs;
  int lines_per_level = kLinesPerLevel;
  int next_count = kNextQueueDefault;
  // 0 = collapse instantly (useful in tests).
  int clear_flash_ms = kClearFlashMs;
  // 0 = no hard-drop flash (also forced off when color is disabled).
  int hard_drop_flash_ms = kHardDropFlashMs;
  Randomizer randomizer = Randomizer::SevenBag;
};

}  // namespace tp
