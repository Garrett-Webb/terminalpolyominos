#pragma once

#include <array>
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
inline constexpr int kMaxPieceCells = 5;
inline constexpr int kMaxBagSize = 25;

struct Offset {
  int x = 0;
  int y = 0;

  friend bool operator==(const Offset& a, const Offset& b) {
    return a.x == b.x && a.y == b.y;
  }
};

enum class Randomizer : std::uint8_t {
  SevenBag = 0,      // classic 7-bag
  SevenPlusOne = 1,  // 7-bag + 1 extra random classic (bag of 8)
  FullRandom = 2,    // independent uniform pick among the 7
  Torture = 3,       // 25-bag: 10 S + 10 Z + 5 from {I,O,T,J,L}
  Funk = 4,          // 7 classics + 1 generated shape
  Freak = 5,         // every piece independently generated
};

enum class PlayMode : std::uint8_t {
  Endless = 0,
  Marathon = 1,  // clear 150 lines -> win
  Sprint = 2,    // clear 40 lines -> win
};

inline constexpr int kPlayModeCount = 3;
inline constexpr int kMarathonLineGoal = 150;
inline constexpr int kSprintLineGoal = 40;

[[nodiscard]] inline int play_mode_line_goal(PlayMode mode) {
  switch (mode) {
    case PlayMode::Marathon:
      return kMarathonLineGoal;
    case PlayMode::Sprint:
      return kSprintLineGoal;
    case PlayMode::Endless:
      break;
  }
  return 0;
}

[[nodiscard]] inline const char* play_mode_token(PlayMode mode) {
  switch (mode) {
    case PlayMode::Marathon:
      return "marathon";
    case PlayMode::Sprint:
      return "sprint";
    case PlayMode::Endless:
      break;
  }
  return "endless";
}

enum class PieceType : std::uint8_t {
  I = 0,
  O,
  T,
  S,
  Z,
  J,
  L,
  Custom,
  Count,
};

// Classic: kind in I..L, n=4, cells unused (SRS tables).
// Custom: kind=Custom, n in 3..5, cells[0..n) = base orientation (min x,y = 0).
struct PieceSpec {
  PieceType kind = PieceType::I;
  int n = 4;
  std::array<Offset, kMaxPieceCells> cells{};

  static PieceSpec classic(PieceType type) {
    PieceSpec s;
    s.kind = type;
    s.n = 4;
    return s;
  }

  [[nodiscard]] bool is_custom() const { return kind == PieceType::Custom; }

  friend bool operator==(const PieceSpec& a, const PieceSpec& b) {
    if (a.kind != b.kind || a.n != b.n) {
      return false;
    }
    if (!a.is_custom()) {
      return true;
    }
    for (int i = 0; i < a.n; ++i) {
      if (a.cells[static_cast<std::size_t>(i)] != b.cells[static_cast<std::size_t>(i)]) {
        return false;
      }
    }
    return true;
  }
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
  Scores,    // open high scores (title)
};

enum class Phase : std::uint8_t {
  Ready = 0,
  Playing,
  Paused,
  GameOver,
  Finished,  // Marathon / Sprint line goal reached
};

struct Cell {
  bool filled = false;
  PieceType type = PieceType::I;
  // Palette index used when drawing this cell (0–255). Set at lock time.
  std::uint8_t color = 7;
};

struct ActivePiece {
  PieceSpec spec{};
  int x = 0;
  int y = 0;
  int rotation = 0;  // 0..3
  bool alive = false;

  [[nodiscard]] PieceType kind() const { return spec.kind; }
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
  PlayMode play_mode = PlayMode::Endless;
  // Hash custom/funk/freak shapes onto palette indices. Off -> white.
  bool freak_colors = true;
  // When true, piece colors use 256-color indices; else classic 16-color.
  bool colors_256 = true;
};

}  // namespace tp
