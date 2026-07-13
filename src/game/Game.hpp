#pragma once

#include "game/Bag.hpp"
#include "game/Board.hpp"
#include "game/Types.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace tp {

struct GameState {
  Board board;
  ActivePiece active;
  ActivePiece ghost;
  std::optional<PieceType> hold;
  std::array<PieceType, kNextQueueMax> next{};
  int next_count = kNextQueueDefault;
  int score = 0;
  int level = 1;
  int lines = 0;
  Phase phase = Phase::Ready;
  bool hold_used = false;
  // While > 0, matching rows in clear_rows flash before collapsing.
  int clear_flash_ms = 0;
  std::array<bool, kBoardHeight> clear_rows{};
  // Hard-drop lock flash: white cells for the just-locked piece.
  int lock_flash_ms = 0;
  ActivePiece lock_flash{};
  // Tetrominoes locked this game (indexed by PieceType).
  std::array<int, static_cast<std::size_t>(PieceType::Count)> pieces_placed{};
};

class Game {
 public:
  explicit Game(GameConfig config = {});

  void reset(std::uint64_t seed = 1);
  void apply(Action action);
  void tick(int elapsed_ms);
  void set_config(GameConfig config);

  [[nodiscard]] const GameState& state() const { return state_; }
  [[nodiscard]] const GameConfig& config() const { return config_; }

  // True if the board/HUD may have changed since the last consume_dirty().
  [[nodiscard]] bool consume_dirty();

  // Test helpers / deterministic injection.
  void set_active_for_test(ActivePiece piece);
  void fill_row_for_test(int y, PieceType type = PieceType::I);

 private:
  void mark_dirty() { dirty_ = true; }

  [[nodiscard]] bool fits(const ActivePiece& piece) const;
  [[nodiscard]] bool fits_at(PieceType type, int x, int y, int rotation) const;
  [[nodiscard]] ActivePiece spawn_piece(PieceType type) const;
  [[nodiscard]] ActivePiece ghost_of(const ActivePiece& piece) const;
  [[nodiscard]] int gravity_ms_per_row() const;

  void refresh_ghost();
  void refresh_next_preview();
  bool try_move(int dx, int dy);
  bool try_rotate(int dir);  // +1 CW, -1 CCW
  void soft_drop_one();
  void sonic_drop();
  void hard_drop();
  void lock_active(bool from_hard_drop = false);
  void finish_hard_drop_flash();
  void begin_line_clear_anim();
  void finish_line_clear();
  void spawn_next();
  void hold();
  void add_line_score(int cleared);
  void begin_lock_if_grounded();
  void cancel_lock();
  [[nodiscard]] bool is_flashing() const {
    return state_.clear_flash_ms > 0 || state_.lock_flash_ms > 0;
  }

  GameConfig config_;
  GameState state_;
  Bag bag_;
  PieceType hold_piece_ = PieceType::I;
  bool has_hold_ = false;

  int gravity_accum_ms_ = 0;
  int lock_accum_ms_ = 0;
  bool locking_ = false;
  bool dirty_ = true;
  int pending_clear_count_ = 0;
};

}  // namespace tp
