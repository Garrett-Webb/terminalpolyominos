#include "game/Bag.hpp"
#include "game/Board.hpp"
#include "game/Game.hpp"
#include "game/Piece.hpp"
#include "test_assert.hpp"

#include <array>
#include <set>
#include <utility>

namespace {

void test_bag_is_permutation_of_seven() {
  tp::Bag bag(42);
  std::array<int, 7> counts{};
  for (int i = 0; i < 7; ++i) {
    const auto t = bag.next();
    const int ti = static_cast<int>(t);
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
    const int ti = static_cast<int>(t);
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
  // All seven appear at least once; exactly one type is duplicated.
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
    ++counts[static_cast<std::size_t>(ta)];
  }
  // With 70 draws, every type should appear at least once for this seed.
  for (int c : counts) {
    TP_CHECK(c > 0);
  }
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
  const auto type = game.state().active.type;
  game.apply(tp::Action::SonicDrop);
  TP_CHECK(game.state().active.alive);
  TP_CHECK(game.state().active.type == type);
  // Still on the board: can move after landing.
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
  p.type = tp::PieceType::O;
  p.x = 0;
  p.y = 0;
  p.rotation = 0;
  p.alive = true;
  game.set_active_for_test(p);
  game.apply(tp::Action::HardDrop);
  TP_CHECK(game.state().lines == 4);
  TP_CHECK(game.state().score >= 800);  // 4-line clear at level 1
  TP_CHECK(game.state().level == 1);    // 4 lines < 10
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
    p.type = tp::PieceType::O;
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
  const auto first = game.state().active.type;
  game.apply(tp::Action::Hold);
  TP_CHECK(game.state().hold.has_value());
  TP_CHECK(*game.state().hold == first);
  const auto second = game.state().active.type;
  TP_CHECK(game.state().active.alive);
  game.apply(tp::Action::Hold);  // should be ignored
  TP_CHECK(*game.state().hold == first);
  TP_CHECK(game.state().active.type == second);
}

void test_lock_delay() {
  tp::GameConfig cfg;
  cfg.lock_delay_ms = 500;
  tp::Game game(cfg);
  game.reset(1);
  // Move piece to floor.
  for (int i = 0; i < 40; ++i) {
    game.apply(tp::Action::SoftDrop);
  }
  const auto y = game.state().active.y;
  const auto type = game.state().active.type;
  // Not locked yet after soft-drop collision — lock delay starts.
  TP_CHECK(game.state().active.alive);
  game.tick(499);
  TP_CHECK(game.state().active.alive);
  TP_CHECK(game.state().active.y == y);
  TP_CHECK(game.state().active.type == type);
  game.tick(1);
  // After full delay, piece should have locked and a new one spawned (or game over).
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
  p.type = tp::PieceType::T;
  p.x = 0;  // against left wall
  p.y = 5;
  p.rotation = 0;
  p.alive = true;
  game.set_active_for_test(p);
  game.apply(tp::Action::RotateCW);
  TP_CHECK(game.state().active.alive);
  TP_CHECK(game.state().active.rotation == 1);
}

void test_clear_flash_then_collapse() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 100;
  cfg.hard_drop_flash_ms = 0;
  tp::Game game(cfg);
  game.reset(1);
  game.fill_row_for_test(tp::kBoardHeight - 1);
  tp::ActivePiece p;
  p.type = tp::PieceType::O;
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
  const auto type = game.state().active.type;
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
  const auto type = game.state().active.type;
  game.apply(tp::Action::HardDrop);
  TP_CHECK(game.state().lock_flash_ms > 0);
  TP_CHECK(game.state().lock_flash.alive);
  TP_CHECK(game.state().lock_flash.type == type);
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
  test_clear_flash_then_collapse();
  test_pieces_placed_counted_on_lock();
  test_hard_drop_flash();
  return tp::test::summary("tp_tests");
}
