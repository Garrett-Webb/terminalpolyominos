#include "game/Game.hpp"

#include "game/Piece.hpp"

#include <algorithm>

namespace tp {
namespace {

int line_clear_points(int cleared, int level) {
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
}

void Game::fill_row_for_test(int y, PieceType type) {
  for (int x = 0; x < kBoardWidth; ++x) {
    state_.board.set(x, y, type);
  }
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

bool Game::try_move(int dx, int dy) {
  ActivePiece next = state_.active;
  next.x += dx;
  next.y += dy;
  if (!fits(next)) {
    return false;
  }
  state_.active = next;
  refresh_ghost();
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
  const int kicks[] = {0, -1, 1, -2, 2};
  for (int kick : kicks) {
    if (fits_at(state_.active.spec, state_.active.x + kick, state_.active.y, to)) {
      state_.active.rotation = to;
      state_.active.x += kick;
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
  refresh_ghost();
  lock_active(/*from_hard_drop=*/true);
}

void Game::lock_active(bool from_hard_drop) {
  if (!state_.active.alive) {
    return;
  }
  const PieceSpec locked = state_.active.spec;
  Offset cells[kMaxPieceCells];
  int n = 0;
  piece_cells(locked, state_.active.rotation, cells, n);
  for (int i = 0; i < n; ++i) {
    const int x = state_.active.x + cells[i].x;
    const int y = state_.active.y + cells[i].y;
    if (state_.board.inside(x, y)) {
      state_.board.set(x, y, locked.kind);
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

  state_.clear_rows.fill(false);
  pending_clear_count_ = 0;
  for (int y = 0; y < kBoardHeight; ++y) {
    if (state_.board.line_full(y)) {
      state_.clear_rows[static_cast<std::size_t>(y)] = true;
      ++pending_clear_count_;
    }
  }

  if (from_hard_drop && config_.hard_drop_flash_ms > 0) {
    state_.lock_flash = flashed;
    state_.lock_flash.alive = true;
    state_.lock_flash_ms = config_.hard_drop_flash_ms;
    if (pending_clear_count_ > 0) {
      add_line_score(pending_clear_count_);
    }
    mark_dirty();
    return;
  }

  if (pending_clear_count_ > 0) {
    add_line_score(pending_clear_count_);
    begin_line_clear_anim();
    return;
  }

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
  refresh_ghost();
  mark_dirty();
}

void Game::add_line_score(int cleared) {
  state_.score += line_clear_points(cleared, state_.level);
  state_.lines += cleared;
  state_.level = 1 + state_.lines / config_.lines_per_level;
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
