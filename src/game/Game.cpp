#include "game/Game.hpp"

#include "game/Piece.hpp"
#include "game/SrsKicks.hpp"

#include <algorithm>

namespace tp {
namespace {

int ordinary_clear_points(int cleared, int level) {
  int base = 0;
  switch (cleared) {
    case 1:
      base = 100;
      break;
    case 2:
      base = 300;
      break;
    case 3:
      base = 500;
      break;
    case 4:
      base = 800;
      break;
    case 5:
      base = 1200;
      break;
    default:
      return 0;
  }
  return base * level;
}

int lock_points(int cleared, SpinType spin, int level) {
  if (spin == SpinType::None) {
    return ordinary_clear_points(cleared, level);
  }
  int base = 0;
  if (spin == SpinType::Mini) {
    switch (cleared) {
      case 0:
        base = 100;
        break;
      case 1:
        base = 200;
        break;
      case 2:
        base = 400;
        break;
      default:
        return ordinary_clear_points(cleared, level);
    }
  } else {
    switch (cleared) {
      case 0:
        base = 400;
        break;
      case 1:
        base = 800;
        break;
      case 2:
        base = 1200;
        break;
      case 3:
        base = 1600;
        break;
      default:
        return ordinary_clear_points(cleared, level);
    }
  }
  return base * level;
}

int clamp_next_count(int n) {
  if (n < 1) {
    return 1;
  }
  if (n > kNextQueueMax) {
    return kNextQueueMax;
  }
  return n;
}

}  // namespace

Game::Game(GameConfig config) : config_(config) {
  config_.next_count = clamp_next_count(config_.next_count);
  reset(1);
}

void Game::set_config(GameConfig config) {
  config.next_count = clamp_next_count(config.next_count);
  const int old_n = state_.next_count;
  config_ = config;
  bag_.set_randomizer(config_.randomizer);
  state_.next_count = config_.next_count;
  if (state_.next_count > old_n) {
    for (int i = old_n; i < state_.next_count; ++i) {
      state_.next[static_cast<std::size_t>(i)] = bag_.next();
    }
  }
  mark_dirty();
}

bool Game::consume_dirty() {
  const bool d = dirty_;
  dirty_ = false;
  return d;
}

void Game::reset(std::uint64_t seed) {
  state_ = GameState{};
  state_.phase = Phase::Playing;
  state_.next_count = clamp_next_count(config_.next_count);
  config_.next_count = state_.next_count;
  bag_.set_randomizer(config_.randomizer);
  bag_.reseed(seed);
  has_hold_ = false;
  hold_piece_ = {};
  gravity_accum_ms_ = 0;
  lock_accum_ms_ = 0;
  locking_ = false;
  pending_clear_count_ = 0;
  pending_spin_ = SpinType::None;
  clear_rotate_flag();
  refresh_next_preview();
  spawn_next();
  mark_dirty();
}

void Game::apply(Action action) {
  if (action == Action::Quit) {
    return;
  }
  if (action == Action::Restart) {
    reset(1);
    return;
  }
  if (action == Action::Pause) {
    if (state_.phase == Phase::Playing) {
      state_.phase = Phase::Paused;
      mark_dirty();
    } else if (state_.phase == Phase::Paused) {
      state_.phase = Phase::Playing;
      mark_dirty();
    }
    return;
  }

  if (state_.phase != Phase::Playing) {
    return;
  }
  if (is_flashing()) {
    return;
  }
  if (!state_.active.alive) {
    return;
  }

  switch (action) {
    case Action::Left:
      if (try_move(-1, 0)) {
        begin_lock_if_grounded();
        mark_dirty();
      }
      break;
    case Action::Right:
      if (try_move(1, 0)) {
        begin_lock_if_grounded();
        mark_dirty();
      }
      break;
    case Action::SoftDrop:
      soft_drop_one();
      break;
    case Action::SonicDrop:
      sonic_drop();
      break;
    case Action::HardDrop:
      hard_drop();
      break;
    case Action::RotateCW:
      if (try_rotate(1)) {
        begin_lock_if_grounded();
        mark_dirty();
      }
      break;
    case Action::RotateCCW:
      if (try_rotate(-1)) {
        begin_lock_if_grounded();
        mark_dirty();
      }
      break;
    case Action::Hold:
      hold();
      break;
    case Action::None:
    case Action::Pause:
    case Action::Quit:
    case Action::Restart:
    case Action::Settings:
    case Action::Scores:
      break;
  }
}

void Game::tick(int elapsed_ms) {
  if (elapsed_ms < 0) {
    elapsed_ms = 0;
  }
  if (state_.phase != Phase::Playing) {
    return;
  }

  state_.play_ms += elapsed_ms;

  if (state_.lock_flash_ms > 0) {
    state_.lock_flash_ms -= elapsed_ms;
    mark_dirty();
    if (state_.lock_flash_ms <= 0) {
      state_.lock_flash_ms = 0;
      finish_hard_drop_flash();
    }
    return;
  }

  if (state_.clear_flash_ms > 0) {
    state_.clear_flash_ms -= elapsed_ms;
    mark_dirty();
    if (state_.clear_flash_ms <= 0) {
      state_.clear_flash_ms = 0;
      finish_line_clear();
    }
    return;
  }

  if (!state_.active.alive) {
    return;
  }

  if (locking_) {
    lock_accum_ms_ += elapsed_ms;
    if (lock_accum_ms_ >= config_.lock_delay_ms) {
      lock_active();
    }
    return;
  }

  gravity_accum_ms_ += elapsed_ms;
  const int step = gravity_ms_per_row();
  bool moved = false;
  while (gravity_accum_ms_ >= step) {
    gravity_accum_ms_ -= step;
    if (!try_move(0, 1)) {
      begin_lock_if_grounded();
      break;
    }
    moved = true;
  }
  if (moved) {
    mark_dirty();
  }
}

void Game::set_active_for_test(ActivePiece piece) {
  state_.active = piece;
  state_.active.alive = true;
  refresh_ghost();
  gravity_accum_ms_ = 0;
  cancel_lock();
  clear_rotate_flag();
}

void Game::fill_row_for_test(int y, PieceType type) {
  const int color = piece_color(PieceSpec::classic(type), config_.colors_256);
  for (int x = 0; x < kBoardWidth; ++x) {
    state_.board.set(x, y, type, color);
  }
}

void Game::set_cell_for_test(int x, int y, PieceType type) {
  state_.board.set(x, y, type, piece_color(PieceSpec::classic(type), config_.colors_256));
}

void Game::set_lines_for_test(int lines) {
  state_.lines = lines;
  state_.level = 1 + lines / config_.lines_per_level;
  mark_dirty();
}

bool Game::fits(const ActivePiece& piece) const {
  return fits_at(piece.spec, piece.x, piece.y, piece.rotation);
}

bool Game::fits_at(const PieceSpec& spec, int x, int y, int rotation) const {
  Offset cells[kMaxPieceCells];
  int n = 0;
  piece_cells(spec, rotation, cells, n);
  for (int i = 0; i < n; ++i) {
    if (state_.board.occupied(x + cells[i].x, y + cells[i].y)) {
      return false;
    }
  }
  return true;
}

ActivePiece Game::spawn_piece(const PieceSpec& spec) const {
  ActivePiece p;
  p.spec = spec;
  p.rotation = 0;
  p.alive = true;
  p.y = 0;

  Offset cells[kMaxPieceCells];
  int n = 0;
  piece_cells(spec, 0, cells, n);
  int min_x = 0;
  int max_x = 0;
  if (n > 0) {
    min_x = cells[0].x;
    max_x = cells[0].x;
    for (int i = 1; i < n; ++i) {
      min_x = std::min(min_x, cells[i].x);
      max_x = std::max(max_x, cells[i].x);
    }
  }
  const int width = max_x - min_x + 1;
  // Center the piece's bbox in the well, adjusting for local min_x.
  p.x = (kBoardWidth - width) / 2 - min_x;
  return p;
}

ActivePiece Game::ghost_of(const ActivePiece& piece) const {
  ActivePiece g = piece;
  if (!g.alive) {
    return g;
  }
  while (fits_at(g.spec, g.x, g.y + 1, g.rotation)) {
    ++g.y;
  }
  return g;
}

int Game::gravity_ms_per_row() const {
  const int level = state_.level < 1 ? 1 : state_.level;
  const int ms = 800 - (level - 1) * 50;
  return ms < 50 ? 50 : ms;
}

void Game::refresh_ghost() {
  state_.ghost = ghost_of(state_.active);
}

void Game::refresh_next_preview() {
  for (int i = 0; i < state_.next_count; ++i) {
    state_.next[static_cast<std::size_t>(i)] = bag_.next();
  }
}

void Game::clear_rotate_flag() {
  last_rotate_ = false;
  last_kick_dx_ = 0;
  last_kick_dy_ = 0;
}

void Game::note_rotate(int kick_dx, int kick_dy) {
  last_rotate_ = true;
  last_kick_dx_ = kick_dx;
  last_kick_dy_ = kick_dy;
}

bool Game::try_move(int dx, int dy) {
  ActivePiece next = state_.active;
  next.x += dx;
  next.y += dy;
  if (!fits(next)) {
    return false;
  }
  state_.active = next;
  refresh_ghost();
  clear_rotate_flag();
  if (dy != 0) {
    cancel_lock();
  } else if (locking_) {
    lock_accum_ms_ = 0;
  }
  return true;
}

bool Game::try_rotate(int dir) {
  const int from = state_.active.rotation;
  const int to = (from + dir + 4) % 4;
  Kick kicks[kMaxKickTests];
  const int n = srs_kick_tests(state_.active.spec.kind, from, to, kicks);
  for (int i = 0; i < n; ++i) {
    const int nx = state_.active.x + kicks[i].dx;
    const int ny = state_.active.y + kicks[i].dy;
    if (fits_at(state_.active.spec, nx, ny, to)) {
      state_.active.rotation = to;
      state_.active.x = nx;
      state_.active.y = ny;
      note_rotate(kicks[i].dx, kicks[i].dy);
      refresh_ghost();
      if (locking_) {
        lock_accum_ms_ = 0;
      }
      return true;
    }
  }
  return false;
}

void Game::soft_drop_one() {
  if (try_move(0, 1)) {
    state_.score += 1;
    mark_dirty();
  } else {
    begin_lock_if_grounded();
  }
}

void Game::sonic_drop() {
  if (!state_.active.alive) {
    return;
  }
  const int start_y = state_.active.y;
  state_.active = ghost_of(state_.active);
  const int dropped = state_.active.y - start_y;
  if (dropped > 0) {
    state_.score += dropped * 2;
  }
  // Drop is a translate; clears rotate even when distance is 0.
  clear_rotate_flag();
  refresh_ghost();
  begin_lock_if_grounded();
  mark_dirty();
}

void Game::hard_drop() {
  const int start_y = state_.active.y;
  state_.active = ghost_of(state_.active);
  const int dropped = state_.active.y - start_y;
  if (dropped > 0) {
    state_.score += dropped * 2;
  }
  clear_rotate_flag();
  refresh_ghost();
  lock_active(/*from_hard_drop=*/true);
}

SpinType Game::detect_tspin_on_lock() const {
  if (!last_rotate_ || state_.active.kind() != PieceType::T) {
    return SpinType::None;
  }
  return classify_tspin(state_.board, state_.active.x, state_.active.y, state_.active.rotation,
                        last_kick_dx_, last_kick_dy_);
}

void Game::lock_active(bool from_hard_drop) {
  if (!state_.active.alive) {
    return;
  }
  const SpinType spin = detect_tspin_on_lock();
  const PieceSpec locked = state_.active.spec;
  Offset cells[kMaxPieceCells];
  int n = 0;
  piece_cells(locked, state_.active.rotation, cells, n);
  for (int i = 0; i < n; ++i) {
    const int x = state_.active.x + cells[i].x;
    const int y = state_.active.y + cells[i].y;
    if (state_.board.inside(x, y)) {
      state_.board.set(x, y, locked.kind,
                       piece_color(locked, config_.freak_colors, config_.colors_256));
    }
  }
  const auto ti = static_cast<std::size_t>(locked.kind);
  if (ti < state_.pieces_placed.size()) {
    ++state_.pieces_placed[ti];
  }

  ActivePiece flashed = state_.active;
  state_.active.alive = false;
  state_.ghost = {};
  cancel_lock();
  clear_rotate_flag();

  state_.clear_rows.fill(false);
  pending_clear_count_ = 0;
  for (int y = 0; y < kBoardHeight; ++y) {
    if (state_.board.line_full(y)) {
      state_.clear_rows[static_cast<std::size_t>(y)] = true;
      ++pending_clear_count_;
    }
  }
  pending_spin_ = spin;

  // Always run scoring path so 0-line locks reset combo (B2B chain stays).
  const bool has_clear = pending_clear_count_ > 0;
  const bool has_spin = spin != SpinType::None;

  if (from_hard_drop && config_.hard_drop_flash_ms > 0) {
    state_.lock_flash = flashed;
    state_.lock_flash.alive = true;
    state_.lock_flash_ms = config_.hard_drop_flash_ms;
    add_lock_score(pending_clear_count_, pending_spin_);
    pending_spin_ = SpinType::None;
    mark_dirty();
    return;
  }

  add_lock_score(pending_clear_count_, pending_spin_);
  pending_spin_ = SpinType::None;
  if (has_clear) {
    begin_line_clear_anim();
    return;
  }
  (void)has_spin;

  state_.hold_used = false;
  spawn_next();
  mark_dirty();
}

void Game::finish_hard_drop_flash() {
  state_.lock_flash = {};
  state_.lock_flash_ms = 0;
  if (pending_clear_count_ > 0) {
    begin_line_clear_anim();
    return;
  }
  state_.hold_used = false;
  spawn_next();
  mark_dirty();
}

void Game::begin_line_clear_anim() {
  const int flash = config_.clear_flash_ms;
  if (flash <= 0) {
    finish_line_clear();
  } else {
    state_.clear_flash_ms = flash;
    mark_dirty();
  }
}

void Game::finish_line_clear() {
  const int cleared = state_.board.clear_full_lines();
  (void)cleared;
  state_.clear_rows.fill(false);
  pending_clear_count_ = 0;
  state_.clear_flash_ms = 0;
  state_.hold_used = false;
  const int goal = play_mode_line_goal(config_.play_mode);
  if (goal > 0 && state_.lines >= goal) {
    state_.active = {};
    state_.ghost = {};
    state_.phase = Phase::Finished;
    mark_dirty();
    return;
  }
  spawn_next();
  mark_dirty();
}

void Game::spawn_next() {
  const PieceSpec type = state_.next[0];
  const int n = state_.next_count;
  for (int i = 0; i < n - 1; ++i) {
    state_.next[static_cast<std::size_t>(i)] = state_.next[static_cast<std::size_t>(i + 1)];
  }
  if (n > 0) {
    state_.next[static_cast<std::size_t>(n - 1)] = bag_.next();
  }

  ActivePiece spawned = spawn_piece(type);
  if (!fits(spawned)) {
    state_.active = spawned;
    state_.active.alive = false;
    state_.phase = Phase::GameOver;
    state_.ghost = {};
    mark_dirty();
    return;
  }
  state_.active = spawned;
  gravity_accum_ms_ = 0;
  cancel_lock();
  clear_rotate_flag();
  refresh_ghost();
  mark_dirty();
}

void Game::hold() {
  if (state_.hold_used || !state_.active.alive) {
    return;
  }
  const PieceSpec current = state_.active.spec;
  if (!has_hold_) {
    has_hold_ = true;
    hold_piece_ = current;
    state_.hold = hold_piece_;
    state_.hold_used = true;
    state_.active.alive = false;
    cancel_lock();
    clear_rotate_flag();
    spawn_next();
    state_.hold_used = true;
    mark_dirty();
    return;
  }

  const PieceSpec swap = hold_piece_;
  hold_piece_ = current;
  state_.hold = hold_piece_;
  ActivePiece swapped = spawn_piece(swap);
  if (!fits(swapped)) {
    hold_piece_ = swap;
    state_.hold = hold_piece_;
    return;
  }
  state_.active = swapped;
  state_.hold_used = true;
  gravity_accum_ms_ = 0;
  cancel_lock();
  clear_rotate_flag();
  refresh_ghost();
  mark_dirty();
}

void Game::add_lock_score(int cleared, SpinType spin) {
  const bool difficult = cleared > 0 && (spin != SpinType::None || cleared >= 4);

  int points = lock_points(cleared, spin, state_.level);
  if (difficult && state_.b2b_ready) {
    // Difficult chain: action score × 1.5 (drop points excluded; those are added elsewhere).
    points += points / 2;
  }

  if (cleared > 0) {
    if (state_.combo > 0) {
      points += 50 * state_.combo * state_.level;
    }
    ++state_.combo;
    state_.b2b_ready = difficult;
    state_.lines += cleared;
    state_.level = 1 + state_.lines / config_.lines_per_level;
  } else {
    // No lines: break combo; B2B chain preserved (incl. 0-line T-piece spins).
    state_.combo = 0;
  }

  state_.score += points;
}

void Game::begin_lock_if_grounded() {
  if (!state_.active.alive) {
    return;
  }
  if (fits_at(state_.active.spec, state_.active.x, state_.active.y + 1, state_.active.rotation)) {
    cancel_lock();
    return;
  }
  if (!locking_) {
    locking_ = true;
    lock_accum_ms_ = 0;
  }
}

void Game::cancel_lock() {
  locking_ = false;
  lock_accum_ms_ = 0;
}

}  // namespace tp
