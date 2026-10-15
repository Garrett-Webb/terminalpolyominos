#pragma once

#include "game/Bag.hpp"
#include "game/BattleAttack.hpp"
#include "game/Board.hpp"
#include "game/SrsKicks.hpp"
#include "game/Types.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace tp {

struct GameState {
  Board board;
  ActivePiece active;
  ActivePiece ghost;
  std::optional<PieceSpec> hold;
  std::array<PieceSpec, kNextQueueMax> next{};
  int next_count = kNextQueueDefault;
  int score = 0;
  int level = 1;
  int lines = 0;
  int play_ms = 0;
  int combo = 0;
  bool b2b_ready = false;
  Phase phase = Phase::Ready;
  bool hold_used = false;
  int clear_flash_ms = 0;
  std::array<bool, kBoardHeight> clear_rows{};
  int lock_flash_ms = 0;
  ActivePiece lock_flash{};
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
  [[nodiscard]] bool battle() const { return config_.battle; }

  [[nodiscard]] bool consume_dirty();

  // Battle: enqueue garbage packets (applied at next spawn_next). hole_col clamped 0..9.
  void queue_garbage(int rows, int hole_col);
  [[nodiscard]] int pending_incoming() const;
  [[nodiscard]] std::optional<BattleOutgoing> consume_outgoing();

  // Test helpers
  void set_active_for_test(ActivePiece piece);
  void fill_row_for_test(int y, PieceType type = PieceType::I);
  void set_cell_for_test(int x, int y, PieceType type = PieceType::I);
  void set_lines_for_test(int lines);

 private:
  struct GarbagePacket {
    int rows = 0;
    int hole_col = 0;
  };

  void apply_battle_config_constraints();
  void inject_pending_garbage();
  void reduce_pending_incoming(int rows);
  void finish_battle_attack(bool perfect_clear);
  [[nodiscard]] int garbage_color() const;
  void mark_dirty() { dirty_ = true; }

  [[nodiscard]] bool fits(const ActivePiece& piece) const;
  [[nodiscard]] bool fits_at(const PieceSpec& spec, int x, int y, int rotation) const;
  [[nodiscard]] ActivePiece spawn_piece(const PieceSpec& spec) const;
  [[nodiscard]] ActivePiece ghost_of(const ActivePiece& piece) const;
  [[nodiscard]] int gravity_ms_per_row() const;

  void refresh_ghost();
  void refresh_next_preview();
  bool try_move(int dx, int dy);
  bool try_rotate(int dir);  // +1 CW, -1 CCW
  void soft_drop_one();
  void sonic_drop();
  void hard_drop();
  void lock_active();
  void finish_lock_flash();
  void begin_line_clear_anim();
  void finish_line_clear();
  void spawn_next();
  void hold();
  void add_lock_score(int cleared, SpinType spin);
  [[nodiscard]] SpinType detect_tspin_on_lock() const;
  void begin_lock_if_grounded();
  void cancel_lock();
  void clear_rotate_flag();
  void note_rotate(int kick_dx, int kick_dy);
  [[nodiscard]] bool is_flashing() const {
    return state_.clear_flash_ms > 0 || state_.lock_flash_ms > 0;
  }

  GameConfig config_;
  GameState state_;
  Bag bag_;
  PieceSpec hold_piece_{};
  bool has_hold_ = false;

  int gravity_accum_ms_ = 0;
  int lock_accum_ms_ = 0;
  bool locking_ = false;
  bool dirty_ = true;
  int pending_clear_count_ = 0;
  bool last_rotate_ = false;
  int last_kick_dx_ = 0;
  int last_kick_dy_ = 0;
  SpinType pending_spin_ = SpinType::None;

  // Battle attack stash (set at lock; resolved after collapse).
  bool pending_attack_ = false;
  int pending_attack_lines_ = 0;
  SpinType pending_attack_spin_ = SpinType::None;
  int pending_attack_combo_ = 0;
  bool pending_attack_b2b_ = false;

  std::array<GarbagePacket, kMaxGarbagePackets> garbage_queue_{};
  int garbage_queue_n_ = 0;
  std::optional<BattleOutgoing> outgoing_{};
};

}  // namespace tp
