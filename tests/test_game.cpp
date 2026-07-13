#include "game/Bag.hpp"
#include "game/Board.hpp"
#include "game/Game.hpp"
#include "game/Piece.hpp"
#include "game/ShapeGen.hpp"
#include "game/SrsKicks.hpp"
#include "test_assert.hpp"

#include <algorithm>
#include <array>
#include <random>
#include <set>
#include <utility>

namespace {

bool is_connected(const tp::PieceSpec& spec) {
  if (spec.n <= 0) {
    return false;
  }
  bool seen[5]{};
  int queue[5];
  int qh = 0;
  int qt = 0;
  queue[qt++] = 0;
  seen[0] = true;
  int found = 1;
  constexpr int kDx[4] = {1, -1, 0, 0};
  constexpr int kDy[4] = {0, 0, 1, -1};
  while (qh < qt) {
    const int i = queue[qh++];
    const auto& a = spec.cells[static_cast<std::size_t>(i)];
    for (int j = 0; j < spec.n; ++j) {
      if (seen[j]) {
        continue;
      }
      const auto& b = spec.cells[static_cast<std::size_t>(j)];
      for (int d = 0; d < 4; ++d) {
        if (a.x + kDx[d] == b.x && a.y + kDy[d] == b.y) {
          seen[j] = true;
          queue[qt++] = j;
          ++found;
          break;
        }
      }
    }
  }
  return found == spec.n;
}

bool bbox_ok(const tp::PieceSpec& spec) {
  int min_x = spec.cells[0].x;
  int max_x = spec.cells[0].x;
  int min_y = spec.cells[0].y;
  int max_y = spec.cells[0].y;
  for (int i = 1; i < spec.n; ++i) {
    min_x = std::min(min_x, spec.cells[static_cast<std::size_t>(i)].x);
    max_x = std::max(max_x, spec.cells[static_cast<std::size_t>(i)].x);
    min_y = std::min(min_y, spec.cells[static_cast<std::size_t>(i)].y);
    max_y = std::max(max_y, spec.cells[static_cast<std::size_t>(i)].y);
  }
  return min_x == 0 && min_y == 0 && (max_x - min_x + 1) <= 4 && (max_y - min_y + 1) <= 4;
}

void test_bag_is_permutation_of_seven() {
  tp::Bag bag(42);
  std::array<int, 7> counts{};
  for (int i = 0; i < 7; ++i) {
    const auto t = bag.next();
    const int ti = static_cast<int>(t.kind);
    TP_CHECK(ti < 7);
    ++counts[static_cast<std::size_t>(ti)];
  }
  for (int c : counts) {
    TP_CHECK(c == 1);
  }
}

void test_bag_seven_plus_one() {
  tp::Bag bag(42, tp::Randomizer::SevenPlusOne);
  std::array<int, 7> counts{};
  int total = 0;
  for (int i = 0; i < 8; ++i) {
    const auto t = bag.next();
    const int ti = static_cast<int>(t.kind);
    TP_CHECK(ti >= 0 && ti < 7);
    ++counts[static_cast<std::size_t>(ti)];
    ++total;
  }
  TP_CHECK(total == 8);
  int ones = 0;
  int twos = 0;
  for (int c : counts) {
    TP_CHECK(c == 1 || c == 2);
    if (c == 1) {
      ++ones;
    }
    if (c == 2) {
      ++twos;
    }
  }
  TP_CHECK(ones == 6);
  TP_CHECK(twos == 1);

  tp::Bag a(7, tp::Randomizer::SevenPlusOne);
  tp::Bag b(7, tp::Randomizer::SevenPlusOne);
  for (int i = 0; i < 24; ++i) {
    TP_CHECK(a.next() == b.next());
  }
}

void test_bag_full_random_deterministic() {
  tp::Bag a(99, tp::Randomizer::FullRandom);
  tp::Bag b(99, tp::Randomizer::FullRandom);
  std::array<int, 7> counts{};
  for (int i = 0; i < 70; ++i) {
    const auto ta = a.next();
    const auto tb = b.next();
    TP_CHECK(ta == tb);
    ++counts[static_cast<std::size_t>(ta.kind)];
  }
  for (int c : counts) {
    TP_CHECK(c > 0);
  }
}

void test_bag_torture() {
  tp::Bag bag(123, tp::Randomizer::Torture);
  bag.refill_for_test();
  TP_CHECK(bag.bag_size() == 25);
  int s = 0;
  int z = 0;
  int other = 0;
  for (int i = 0; i < 25; ++i) {
    const auto p = bag.bag_at(i);
    TP_CHECK(!p.is_custom());
    if (p.kind == tp::PieceType::S) {
      ++s;
    } else if (p.kind == tp::PieceType::Z) {
      ++z;
    } else {
      TP_CHECK(p.kind == tp::PieceType::I || p.kind == tp::PieceType::O ||
               p.kind == tp::PieceType::T || p.kind == tp::PieceType::J ||
               p.kind == tp::PieceType::L);
      ++other;
    }
  }
  TP_CHECK(s == 10);
  TP_CHECK(z == 10);
  TP_CHECK(other == 5);

  tp::Bag a(55, tp::Randomizer::Torture);
  tp::Bag b(55, tp::Randomizer::Torture);
  for (int i = 0; i < 50; ++i) {
    TP_CHECK(a.next() == b.next());
  }
}

void test_bag_funk() {
  tp::Bag bag(9, tp::Randomizer::Funk);
  bag.refill_for_test();
  TP_CHECK(bag.bag_size() == 8);
  std::array<int, 7> classic{};
  int custom = 0;
  for (int i = 0; i < 8; ++i) {
    const auto p = bag.bag_at(i);
    if (p.is_custom()) {
      ++custom;
      TP_CHECK(p.n >= 3 && p.n <= 5);
      TP_CHECK(is_connected(p));
      TP_CHECK(bbox_ok(p));
    } else {
      const int ti = static_cast<int>(p.kind);
      TP_CHECK(ti >= 0 && ti < 7);
      ++classic[static_cast<std::size_t>(ti)];
    }
  }
  TP_CHECK(custom == 1);
  for (int c : classic) {
    TP_CHECK(c == 1);
  }
}

void test_bag_freak() {
  tp::Bag bag(42, tp::Randomizer::Freak);
  for (int i = 0; i < 20; ++i) {
    const auto p = bag.next();
    TP_CHECK(p.is_custom());
    TP_CHECK(p.n >= 3 && p.n <= 5);
    TP_CHECK(is_connected(p));
    TP_CHECK(bbox_ok(p));
  }
  tp::Bag a(77, tp::Randomizer::Freak);
  tp::Bag b(77, tp::Randomizer::Freak);
  for (int i = 0; i < 30; ++i) {
    TP_CHECK(a.next() == b.next());
  }
}

void test_shape_gen_distribution() {
  std::mt19937_64 rng(2026);
  int counts[6]{};
  constexpr int kDraws = 500;
  for (int i = 0; i < kDraws; ++i) {
    const auto p = tp::generate_shape(rng);
    TP_CHECK(p.is_custom());
    TP_CHECK(p.n >= 3 && p.n <= 5);
    TP_CHECK(is_connected(p));
    TP_CHECK(bbox_ok(p));
    ++counts[p.n];
  }
  // ~40% fours, ~30% threes/fives — allow a wide band for RNG variance.
  TP_CHECK(counts[4] > kDraws / 5);
  TP_CHECK(counts[4] < (kDraws * 3) / 5);
  TP_CHECK(counts[3] > kDraws / 10);
  TP_CHECK(counts[5] > kDraws / 10);
}

void test_custom_rotation_stays_connected() {
  // L / J / T lookalikes that float-pivot rounding used to split.
  const tp::Offset l_cells[] = {{0, 0}, {0, 1}, {0, 2}, {1, 2}};
  const tp::Offset t_cells[] = {{0, 1}, {1, 0}, {1, 1}, {2, 1}};
  const tp::Offset j_cells[] = {{1, 0}, {1, 1}, {1, 2}, {0, 2}};

  auto make = [](const tp::Offset* cells, int n) {
    tp::PieceSpec s;
    s.kind = tp::PieceType::Custom;
    s.n = n;
    for (int i = 0; i < n; ++i) {
      s.cells[static_cast<std::size_t>(i)] = cells[i];
    }
    return s;
  };

  const tp::PieceSpec shapes[] = {make(l_cells, 4), make(t_cells, 4), make(j_cells, 4)};
  for (const auto& base : shapes) {
    for (int r = 0; r < 4; ++r) {
      tp::Offset out[tp::kMaxPieceCells];
      int n = 0;
      tp::piece_cells(base, r, out, n);
      TP_CHECK(n == base.n);
      tp::PieceSpec rotated = base;
      for (int i = 0; i < n; ++i) {
        rotated.cells[static_cast<std::size_t>(i)] = out[i];
      }
      // Normalize for the connectivity helper (expects any connected set).
      TP_CHECK(is_connected(rotated));
    }
  }

  std::mt19937_64 rng(99);
  for (int i = 0; i < 80; ++i) {
    const auto base = tp::generate_shape(rng);
    for (int r = 0; r < 4; ++r) {
      tp::Offset out[tp::kMaxPieceCells];
      int n = 0;
      tp::piece_cells(base, r, out, n);
      tp::PieceSpec rotated = base;
      rotated.n = n;
      for (int c = 0; c < n; ++c) {
        rotated.cells[static_cast<std::size_t>(c)] = out[c];
      }
      TP_CHECK(is_connected(rotated));
    }
  }
}

void test_custom_spawn_rotate_hold() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  cfg.randomizer = tp::Randomizer::Freak;
  tp::Game game(cfg);
  game.reset(3);
  TP_CHECK(game.state().active.alive);
  TP_CHECK(game.state().active.spec.is_custom());
  game.apply(tp::Action::RotateCW);
  TP_CHECK(game.state().active.alive);
  game.apply(tp::Action::Hold);
  TP_CHECK(game.state().hold.has_value());
  TP_CHECK(game.state().hold->is_custom());
  TP_CHECK(game.state().active.alive);
  game.apply(tp::Action::HardDrop);
  TP_CHECK(game.state().pieces_placed[static_cast<std::size_t>(tp::PieceType::Custom)] >= 1 ||
           game.state().active.alive || game.state().phase == tp::Phase::GameOver);
}

void test_next_queue_length() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  cfg.next_count = 5;
  tp::Game game(cfg);
  game.reset(1);
  TP_CHECK(game.state().next_count == 5);
  game.apply(tp::Action::HardDrop);
  TP_CHECK(game.state().next_count == 5);
  TP_CHECK(game.state().active.alive);

  cfg.next_count = 1;
  game.set_config(cfg);
  TP_CHECK(game.state().next_count == 1);
}

void test_bag_deterministic() {
  tp::Bag a(99);
  tp::Bag b(99);
  for (int i = 0; i < 21; ++i) {
    TP_CHECK(a.next() == b.next());
  }
}

void test_board_line_clear() {
  tp::Board board;
  for (int x = 0; x < tp::kBoardWidth; ++x) {
    board.set(x, tp::kBoardHeight - 1, tp::PieceType::I);
  }
  board.set(0, tp::kBoardHeight - 2, tp::PieceType::T);
  TP_CHECK(board.clear_full_lines() == 1);
  TP_CHECK(board.at(0, tp::kBoardHeight - 1).filled);
  TP_CHECK(board.at(0, tp::kBoardHeight - 1).type == tp::PieceType::T);
  TP_CHECK(!board.at(1, tp::kBoardHeight - 1).filled);
}

void test_piece_cells_count() {
  for (int t = 0; t < 7; ++t) {
    for (int r = 0; r < 4; ++r) {
      tp::Offset cells[4];
      tp::piece_cells(static_cast<tp::PieceType>(t), r, cells);
      std::set<std::pair<int, int>> uniq;
      for (const auto& c : cells) {
        uniq.insert({c.x, c.y});
      }
      TP_CHECK(uniq.size() == 4);
    }
  }
}

void test_soft_and_hard_drop_scoring() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  tp::Game game(cfg);
  game.reset(7);
  const int before = game.state().score;
  for (int i = 0; i < 30; ++i) {
    game.apply(tp::Action::SoftDrop);
  }
  TP_CHECK(game.state().score >= before);
  const int mid = game.state().score;
  game.apply(tp::Action::HardDrop);
  TP_CHECK(game.state().score >= mid);
  TP_CHECK(game.state().active.alive);
}

void test_sonic_drop_lands_without_lock() {
  tp::Game game;
  game.reset(11);
  const auto type = game.state().active.spec;
  game.apply(tp::Action::SonicDrop);
  TP_CHECK(game.state().active.alive);
  TP_CHECK(game.state().active.spec == type);
  const int y = game.state().active.y;
  game.apply(tp::Action::Left);
  TP_CHECK(game.state().active.alive);
  TP_CHECK(game.state().active.y == y);
}

void test_line_clear_scoring_and_level() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  tp::Game game(cfg);
  game.reset(1);
  for (int y = tp::kBoardHeight - 4; y < tp::kBoardHeight; ++y) {
    game.fill_row_for_test(y);
  }
  tp::ActivePiece p;
  p.spec = tp::PieceSpec::classic(tp::PieceType::O);
  p.x = 0;
  p.y = 0;
  p.rotation = 0;
  p.alive = true;
  game.set_active_for_test(p);
  game.apply(tp::Action::HardDrop);
  TP_CHECK(game.state().lines == 4);
  TP_CHECK(game.state().score >= 800);
  TP_CHECK(game.state().level == 1);
}

void test_level_up() {
  tp::GameConfig cfg;
  cfg.lines_per_level = 10;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  tp::Game game(cfg);
  game.reset(1);
  for (int i = 0; i < 10; ++i) {
    game.fill_row_for_test(tp::kBoardHeight - 1);
    tp::ActivePiece p;
    p.spec = tp::PieceSpec::classic(tp::PieceType::O);
    p.x = 4;
    p.y = 0;
    p.rotation = 0;
    p.alive = true;
    game.set_active_for_test(p);
    game.apply(tp::Action::HardDrop);
  }
  TP_CHECK(game.state().lines == 10);
  TP_CHECK(game.state().level == 2);
}

void test_hold_once_per_piece() {
  tp::Game game;
  game.reset(3);
  const auto first = game.state().active.spec;
  game.apply(tp::Action::Hold);
  TP_CHECK(game.state().hold.has_value());
  TP_CHECK(*game.state().hold == first);
  const auto second = game.state().active.spec;
  TP_CHECK(game.state().active.alive);
  game.apply(tp::Action::Hold);
  TP_CHECK(*game.state().hold == first);
  TP_CHECK(game.state().active.spec == second);
}

void test_lock_delay() {
  tp::GameConfig cfg;
  cfg.lock_delay_ms = 500;
  tp::Game game(cfg);
  game.reset(1);
  for (int i = 0; i < 40; ++i) {
    game.apply(tp::Action::SoftDrop);
  }
  const auto y = game.state().active.y;
  const auto type = game.state().active.spec;
  TP_CHECK(game.state().active.alive);
  game.tick(499);
  TP_CHECK(game.state().active.alive);
  TP_CHECK(game.state().active.y == y);
  TP_CHECK(game.state().active.spec == type);
  game.tick(1);
  TP_CHECK(game.state().phase == tp::Phase::Playing ||
           game.state().phase == tp::Phase::GameOver);
}

void test_pause() {
  tp::Game game;
  game.reset(1);
  game.apply(tp::Action::Pause);
  TP_CHECK(game.state().phase == tp::Phase::Paused);
  const int score = game.state().score;
  game.apply(tp::Action::SoftDrop);
  game.tick(1000);
  TP_CHECK(game.state().score == score);
  game.apply(tp::Action::Pause);
  TP_CHECK(game.state().phase == tp::Phase::Playing);
}

void test_rotate_basic_kick() {
  tp::Game game;
  game.reset(1);
  tp::ActivePiece p;
  p.spec = tp::PieceSpec::classic(tp::PieceType::T);
  p.x = 0;
  p.y = 5;
  p.rotation = 0;
  p.alive = true;
  game.set_active_for_test(p);
  game.apply(tp::Action::RotateCW);
  TP_CHECK(game.state().active.alive);
  TP_CHECK(game.state().active.rotation == 1);
}

void test_srs_kick_table_code_coords() {
  tp::Kick out[tp::kMaxKickTests];
  const int n = tp::srs_kick_tests(tp::PieceType::T, 0, 1, out);
  TP_CHECK(n == 5);
  TP_CHECK((out[0] == tp::Kick{0, 0}));
  TP_CHECK((out[1] == tp::Kick{-1, 0}));
  TP_CHECK((out[2] == tp::Kick{-1, -1}));
  TP_CHECK((out[3] == tp::Kick{0, 2}));
  TP_CHECK((out[4] == tp::Kick{-1, 2}));

  const int ni = tp::srs_kick_tests(tp::PieceType::I, 0, 1, out);
  TP_CHECK(ni == 5);
  TP_CHECK((out[0] == tp::Kick{0, 0}));
  TP_CHECK((out[1] == tp::Kick{-2, 0}));
  TP_CHECK((out[2] == tp::Kick{1, 0}));
  TP_CHECK((out[3] == tp::Kick{-2, 1}));
  TP_CHECK((out[4] == tp::Kick{1, -2}));

  TP_CHECK(tp::srs_kick_tests(tp::PieceType::O, 0, 1, out) == 1);
  TP_CHECK((out[0] == tp::Kick{0, 0}));
}

void test_srs_i_wall_kick() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  tp::Game game(cfg);
  game.reset(1);

  // Block the basic 0→R placement so I must use the (-2,0) kick.
  for (int y = 0; y < 4; ++y) {
    game.set_cell_for_test(7, y);
  }

  tp::ActivePiece p;
  p.spec = tp::PieceSpec::classic(tp::PieceType::I);
  p.x = 5;
  p.y = 0;
  p.rotation = 0;
  p.alive = true;
  game.set_active_for_test(p);
  game.apply(tp::Action::RotateCW);
  TP_CHECK(game.state().active.rotation == 1);
  TP_CHECK(game.state().active.x == 3);
  TP_CHECK(game.state().active.y == 0);
}

void test_srs_jlstz_wall_kick() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  tp::Game game(cfg);
  game.reset(1);

  // T at left; block basic 0→R so kick (-1,0) is required.
  const int y = 8;
  game.set_cell_for_test(2, y + 1);

  tp::ActivePiece p;
  p.spec = tp::PieceSpec::classic(tp::PieceType::T);
  p.x = 0;
  p.y = y;
  p.rotation = 0;
  p.alive = true;
  game.set_active_for_test(p);
  game.apply(tp::Action::RotateCW);
  TP_CHECK(game.state().active.rotation == 1);
  TP_CHECK(game.state().active.x == -1);
  TP_CHECK(game.state().active.y == y);
}

void test_tspin_full_single() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  cfg.lock_delay_ms = 50;
  tp::Game game(cfg);
  game.reset(1);

  const int by = tp::kBoardHeight - 1;  // 21
  for (int x = 0; x < tp::kBoardWidth; ++x) {
    if (x != 5) {
      game.set_cell_for_test(x, by);
    }
  }
  // Back corners + ensure cavity around T flat on row 20.
  game.set_cell_for_test(4, by - 2);
  game.set_cell_for_test(6, by - 2);

  tp::ActivePiece p;
  p.spec = tp::PieceSpec::classic(tp::PieceType::T);
  p.x = 4;
  p.y = by - 2;  // 19
  p.rotation = 1;  // R; CW → 2 (stem down)
  p.alive = true;
  game.set_active_for_test(p);
  game.apply(tp::Action::RotateCW);
  TP_CHECK(game.state().active.rotation == 2);
  TP_CHECK(game.state().active.x == 4);
  TP_CHECK(game.state().active.y == by - 2);

  const int before = game.state().score;
  // Grounded after rotate → lock delay.
  game.tick(50);
  TP_CHECK(game.state().lines == 1);
  TP_CHECK(game.state().score == before + 800);
}

void test_tspin_mini_no_lines() {
  tp::Board board;
  // Center (5,10). Front for rot0 (stem up) = top pair. Fill one front + both backs.
  board.set(4, 9, tp::PieceType::I);   // front-left
  board.set(4, 11, tp::PieceType::I);  // back-left
  board.set(6, 11, tp::PieceType::I);  // back-right
  // Not front-right (6,9) → Mini candidate.
  TP_CHECK(tp::classify_tspin(board, /*px=*/4, /*py=*/9, /*rot=*/0, 0, 0) == tp::SpinType::Mini);

  // Both fronts → Full.
  board.set(6, 9, tp::PieceType::I);
  TP_CHECK(tp::classify_tspin(board, 4, 9, 0, 0, 0) == tp::SpinType::Full);

  // 1×2 kick forces Full even when Mini shape.
  board.clear_cell(6, 9);
  TP_CHECK(tp::classify_tspin(board, 4, 9, 0, -1, 2) == tp::SpinType::Full);
}

void test_tspin_cancelled_by_move() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  cfg.lock_delay_ms = 50;
  tp::Game game(cfg);
  game.reset(1);

  const int by = tp::kBoardHeight - 1;
  for (int x = 0; x < tp::kBoardWidth; ++x) {
    if (x != 5) {
      game.set_cell_for_test(x, by);
    }
  }
  game.set_cell_for_test(4, by - 2);
  game.set_cell_for_test(6, by - 2);

  tp::ActivePiece p;
  p.spec = tp::PieceSpec::classic(tp::PieceType::T);
  p.x = 4;
  p.y = by - 2;
  p.rotation = 1;
  p.alive = true;
  game.set_active_for_test(p);
  game.apply(tp::Action::RotateCW);
  TP_CHECK(game.state().active.rotation == 2);

  // Sonic drop is a translate even when distance is 0 — clears last-rotate.
  game.apply(tp::Action::SonicDrop);

  const int before = game.state().score;
  game.tick(50);
  TP_CHECK(game.state().lines == 1);
  TP_CHECK(game.state().score == before + 100);
}

void lock_grounded_now(tp::Game& game) {
  // 0-travel hard drop: locks immediately without soft/hard drop points.
  game.apply(tp::Action::HardDrop);
}

void lock_filled_tetris(tp::Game& game) {
  for (int y = tp::kBoardHeight - 4; y < tp::kBoardHeight; ++y) {
    game.fill_row_for_test(y);
  }
  tp::ActivePiece p;
  p.spec = tp::PieceSpec::classic(tp::PieceType::O);
  p.x = 4;
  p.y = tp::kBoardHeight - 6;
  p.rotation = 0;
  p.alive = true;
  game.set_active_for_test(p);
  lock_grounded_now(game);
}

void test_b2b_tetris() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  cfg.lock_delay_ms = 50;
  tp::Game game(cfg);
  game.reset(1);

  lock_filled_tetris(game);
  TP_CHECK(game.state().lines == 4);
  TP_CHECK(game.state().score == 800);
  TP_CHECK(game.state().b2b_ready);
  TP_CHECK(game.state().combo == 1);

  lock_filled_tetris(game);
  // B2B Tetris 1200 + combo 50×1×level
  TP_CHECK(game.state().lines == 8);
  TP_CHECK(game.state().score == 800 + 1200 + 50);
  TP_CHECK(game.state().b2b_ready);
  TP_CHECK(game.state().combo == 2);
}

void test_b2b_broken_by_single() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  cfg.lock_delay_ms = 50;
  tp::Game game(cfg);
  game.reset(1);

  lock_filled_tetris(game);
  TP_CHECK(game.state().b2b_ready);

  game.fill_row_for_test(tp::kBoardHeight - 1);
  tp::ActivePiece p;
  p.spec = tp::PieceSpec::classic(tp::PieceType::O);
  p.x = 4;
  p.y = tp::kBoardHeight - 3;
  p.rotation = 0;
  p.alive = true;
  game.set_active_for_test(p);
  const int before = game.state().score;
  lock_grounded_now(game);
  TP_CHECK(game.state().lines == 5);
  // Ordinary single breaks B2B; combo was 1 so +50.
  TP_CHECK(game.state().score == before + 100 + 50);
  TP_CHECK(!game.state().b2b_ready);
}

void test_b2b_survives_nolines_lock() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  cfg.lock_delay_ms = 50;
  tp::Game game(cfg);
  game.reset(1);

  lock_filled_tetris(game);
  TP_CHECK(game.state().b2b_ready);
  TP_CHECK(game.state().combo == 1);

  tp::ActivePiece p;
  p.spec = tp::PieceSpec::classic(tp::PieceType::O);
  p.x = 4;
  p.y = tp::kBoardHeight - 3;
  p.rotation = 0;
  p.alive = true;
  game.set_active_for_test(p);
  lock_grounded_now(game);
  TP_CHECK(game.state().b2b_ready);
  TP_CHECK(game.state().combo == 0);

  lock_filled_tetris(game);
  // Still B2B (1200) but combo reset so no combo points.
  TP_CHECK(game.state().score == 800 + 1200);
  TP_CHECK(game.state().combo == 1);
}

void test_combo_three_singles() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  cfg.lock_delay_ms = 50;
  tp::Game game(cfg);
  game.reset(1);

  auto single = [&] {
    game.fill_row_for_test(tp::kBoardHeight - 1);
    tp::ActivePiece p;
    p.spec = tp::PieceSpec::classic(tp::PieceType::O);
    p.x = 4;
    p.y = tp::kBoardHeight - 3;
    p.rotation = 0;
    p.alive = true;
    game.set_active_for_test(p);
    lock_grounded_now(game);
  };

  single();
  TP_CHECK(game.state().score == 100);
  TP_CHECK(game.state().combo == 1);
  single();
  TP_CHECK(game.state().score == 100 + 100 + 50);
  TP_CHECK(game.state().combo == 2);
  single();
  TP_CHECK(game.state().score == 250 + 100 + 100);  // +100 clear +50*2 combo
  TP_CHECK(game.state().combo == 3);
}

void test_clear_flash_then_collapse() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 100;
  cfg.hard_drop_flash_ms = 0;
  tp::Game game(cfg);
  game.reset(1);
  game.fill_row_for_test(tp::kBoardHeight - 1);
  tp::ActivePiece p;
  p.spec = tp::PieceSpec::classic(tp::PieceType::O);
  p.x = 4;
  p.y = 0;
  p.rotation = 0;
  p.alive = true;
  game.set_active_for_test(p);
  game.apply(tp::Action::HardDrop);

  TP_CHECK(game.state().lines == 1);
  TP_CHECK(game.state().clear_flash_ms > 0);
  TP_CHECK(game.state().clear_rows[static_cast<std::size_t>(tp::kBoardHeight - 1)]);
  TP_CHECK(!game.state().active.alive);

  game.tick(99);
  TP_CHECK(game.state().clear_flash_ms > 0);
  TP_CHECK(game.state().board.line_full(tp::kBoardHeight - 1));

  game.tick(1);
  TP_CHECK(game.state().clear_flash_ms == 0);
  TP_CHECK(!game.state().board.line_full(tp::kBoardHeight - 1));
  TP_CHECK(game.state().active.alive);
}

void test_pieces_placed_counted_on_lock() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 0;
  tp::Game game(cfg);
  game.reset(1);
  const auto type = game.state().active.kind();
  game.apply(tp::Action::HardDrop);
  TP_CHECK(game.state().pieces_placed[static_cast<std::size_t>(type)] == 1);
  int total = 0;
  for (int c : game.state().pieces_placed) {
    total += c;
  }
  TP_CHECK(total == 1);
}

void test_hard_drop_flash() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.hard_drop_flash_ms = 100;
  tp::Game game(cfg);
  game.reset(1);
  const auto type = game.state().active.spec;
  game.apply(tp::Action::HardDrop);
  TP_CHECK(game.state().lock_flash_ms > 0);
  TP_CHECK(game.state().lock_flash.alive);
  TP_CHECK(game.state().lock_flash.spec == type);
  TP_CHECK(!game.state().active.alive);

  game.tick(99);
  TP_CHECK(game.state().lock_flash_ms > 0);
  game.tick(1);
  TP_CHECK(game.state().lock_flash_ms == 0);
  TP_CHECK(!game.state().lock_flash.alive);
  TP_CHECK(game.state().active.alive);
}

}  // namespace

int main() {
  test_bag_is_permutation_of_seven();
  test_bag_seven_plus_one();
  test_bag_full_random_deterministic();
  test_bag_torture();
  test_bag_funk();
  test_bag_freak();
  test_shape_gen_distribution();
  test_custom_rotation_stays_connected();
  test_custom_spawn_rotate_hold();
  test_next_queue_length();
  test_bag_deterministic();
  test_board_line_clear();
  test_piece_cells_count();
  test_soft_and_hard_drop_scoring();
  test_sonic_drop_lands_without_lock();
  test_line_clear_scoring_and_level();
  test_level_up();
  test_hold_once_per_piece();
  test_lock_delay();
  test_pause();
  test_rotate_basic_kick();
  test_srs_kick_table_code_coords();
  test_srs_i_wall_kick();
  test_srs_jlstz_wall_kick();
  test_tspin_full_single();
  test_tspin_mini_no_lines();
  test_tspin_cancelled_by_move();
  test_b2b_tetris();
  test_b2b_broken_by_single();
  test_b2b_survives_nolines_lock();
  test_combo_three_singles();
  test_clear_flash_then_collapse();
  test_pieces_placed_counted_on_lock();
  test_hard_drop_flash();
  return tp::test::summary("tp_tests");
}
