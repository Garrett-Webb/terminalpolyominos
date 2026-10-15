#include "game/BattleAttack.hpp"
#include "game/Board.hpp"
#include "game/Game.hpp"
#include "game/Piece.hpp"
#include "game/SrsKicks.hpp"
#include "test_assert.hpp"

namespace {

tp::GameConfig battle_cfg() {
  tp::GameConfig cfg;
  cfg.battle = true;
  cfg.clear_flash_ms = 0;
  cfg.piece_lock_flash_ms = 0;
  cfg.colors_256 = true;
  return cfg;
}

void test_attack_table_ordinary() {
  using tp::SpinType;
  auto send = [](int lines, SpinType spin, bool b2b = false, int combo = 1, bool pc = false) {
    tp::AttackInput in;
    in.lines_cleared = lines;
    in.spin = spin;
    in.b2b_bonus = b2b;
    in.combo_after_clear = combo;
    in.perfect_clear = pc;
    return tp::compute_send(in).total_send;
  };

  TP_CHECK(send(1, SpinType::None) == 0);
  TP_CHECK(send(2, SpinType::None) == 1);
  TP_CHECK(send(3, SpinType::None) == 2);
  TP_CHECK(send(4, SpinType::None) == 4);
  TP_CHECK(send(5, SpinType::None) == 4);

  TP_CHECK(send(1, SpinType::Mini) == 1);
  TP_CHECK(send(2, SpinType::Mini) == 2);
  TP_CHECK(send(1, SpinType::Full) == 2);
  TP_CHECK(send(2, SpinType::Full) == 4);
  TP_CHECK(send(3, SpinType::Full) == 6);
}

void test_attack_b2b_combo_pc() {
  using tp::SpinType;
  tp::AttackInput in;
  in.lines_cleared = 4;
  in.spin = SpinType::None;
  in.b2b_bonus = true;
  in.combo_after_clear = 1;
  in.perfect_clear = false;
  auto r = tp::compute_send(in);
  TP_CHECK(r.base == 4);
  TP_CHECK(r.b2b_bonus == 1);
  TP_CHECK(r.combo_bonus == 0);
  TP_CHECK(r.total_send == 5);

  in.b2b_bonus = false;
  in.combo_after_clear = 2;
  r = tp::compute_send(in);
  TP_CHECK(r.combo_bonus == 1);
  TP_CHECK(r.total_send == 5);

  in.combo_after_clear = 8;
  r = tp::compute_send(in);
  TP_CHECK(r.combo_bonus == 4);
  TP_CHECK(r.total_send == 8);

  in.combo_after_clear = 1;
  in.perfect_clear = true;
  in.lines_cleared = 1;
  r = tp::compute_send(in);
  TP_CHECK(r.base == 0);
  TP_CHECK(r.pc_bonus == 6);
  TP_CHECK(r.total_send == 6);

  // T-spin Full Double + B2B + combo 4
  in.lines_cleared = 2;
  in.spin = SpinType::Full;
  in.b2b_bonus = true;
  in.combo_after_clear = 4;
  in.perfect_clear = false;
  r = tp::compute_send(in);
  TP_CHECK(r.base == 4);
  TP_CHECK(r.b2b_bonus == 1);
  TP_CHECK(r.combo_bonus == 2);
  TP_CHECK(r.total_send == 7);
}

void test_combo_bonus_schedule() {
  TP_CHECK(tp::combo_attack_bonus(0) == 0);
  TP_CHECK(tp::combo_attack_bonus(1) == 0);
  TP_CHECK(tp::combo_attack_bonus(2) == 1);
  TP_CHECK(tp::combo_attack_bonus(3) == 1);
  TP_CHECK(tp::combo_attack_bonus(4) == 2);
  TP_CHECK(tp::combo_attack_bonus(5) == 2);
  TP_CHECK(tp::combo_attack_bonus(6) == 3);
  TP_CHECK(tp::combo_attack_bonus(7) == 3);
  TP_CHECK(tp::combo_attack_bonus(8) == 4);
  TP_CHECK(tp::combo_attack_bonus(99) == 4);
}

void test_apply_cancel() {
  int cancelled = -1;
  TP_CHECK(tp::apply_cancel(3, 5, &cancelled) == 2);
  TP_CHECK(cancelled == 3);
  TP_CHECK(tp::apply_cancel(5, 3, &cancelled) == 0);
  TP_CHECK(cancelled == 3);
  TP_CHECK(tp::apply_cancel(0, 4, &cancelled) == 4);
  TP_CHECK(cancelled == 0);
}

void test_board_inject_shared_hole() {
  tp::Board b;
  b.inject_garbage_row(3, tp::kGarbageColor256);
  b.inject_garbage_row(3, tp::kGarbageColor256);
  for (int row = 0; row < 2; ++row) {
    const int y = tp::kBoardHeight - 1 - row;
    for (int x = 0; x < tp::kBoardWidth; ++x) {
      if (x == 3) {
        TP_CHECK(!b.at(x, y).filled);
      } else {
        TP_CHECK(b.at(x, y).filled);
        TP_CHECK(b.at(x, y).type == tp::PieceType::Garbage);
      }
    }
  }
}

void test_board_shift_up_discards_top() {
  tp::Board b;
  b.set(0, 0, tp::PieceType::I, 51);
  b.set(1, tp::kBoardHeight - 1, tp::PieceType::O, 226);
  b.shift_up(1);
  TP_CHECK(!b.at(0, 0).filled);
  TP_CHECK(b.at(1, tp::kBoardHeight - 2).filled);
  TP_CHECK(!b.at(1, tp::kBoardHeight - 1).filled);
}

void test_game_queue_inject_on_spawn() {
  tp::Game g(battle_cfg());
  g.reset(1);
  TP_CHECK(g.state().phase == tp::Phase::Playing);
  TP_CHECK(g.pending_incoming() == 0);

  g.queue_garbage(2, 4);
  TP_CHECK(g.pending_incoming() == 2);

  // Force a 0-line lock → spawn_next → inject.
  // Put a piece that locks without clearing: fill nothing, hard-drop current.
  g.apply(tp::Action::HardDrop);
  // After lock+spawn, garbage should be injected.
  TP_CHECK(g.pending_incoming() == 0);
  const auto& board = g.state().board;
  for (int row = 0; row < 2; ++row) {
    const int y = tp::kBoardHeight - 1 - row;
    for (int x = 0; x < tp::kBoardWidth; ++x) {
      if (x == 4) {
        TP_CHECK(!board.at(x, y).filled);
      } else {
        TP_CHECK(board.at(x, y).filled);
        TP_CHECK(board.at(x, y).type == tp::PieceType::Garbage);
      }
    }
  }
}

void test_game_outgoing_double() {
  tp::Game g(battle_cfg());
  g.reset(1);

  // Build a double: two nearly-full bottom rows, hole in col 0; drop J/L/I to fill.
  // Fill rows height-1 and height-2 except x=0.
  for (int y = tp::kBoardHeight - 2; y < tp::kBoardHeight; ++y) {
    for (int x = 1; x < tp::kBoardWidth; ++x) {
      g.set_cell_for_test(x, y, tp::PieceType::I);
    }
  }
  // Vertical I at x=0 covering both holes — I spawn rotation is horizontal.
  // Use O? O is 2x2. Place active as two-cell stack via set_active:
  // Simpler: fill both rows completely then they're already full — lock anything
  // that doesn't break them. fill_row_for_test makes full rows; lock a piece in
  // the buffer that doesn't clear extra... If rows are already full, lock_active
  // detects them as clear rows.
  tp::Game g2(battle_cfg());
  g2.reset(1);
  g2.fill_row_for_test(tp::kBoardHeight - 1);
  g2.fill_row_for_test(tp::kBoardHeight - 2);
  // Active piece already exists from reset; hard drop locks it (may add cells).
  // Clear active by locking a piece in empty space above: move to top and hard drop
  // onto the filled rows — the piece will sit on top and lock, then the two full
  // rows clear.
  g2.apply(tp::Action::HardDrop);
  auto out = g2.consume_outgoing();
  TP_CHECK(out.has_value());
  // Double base 1 + combo 1 (first clear) → combo_after=1 → bonus 0 → send 1
  TP_CHECK(out->lines_cleared == 2);
  TP_CHECK(out->raw_send == 1);
  TP_CHECK(out->lines_sent == 1);
  TP_CHECK(out->cancelled == 0);
  TP_CHECK(g2.state().lines == 2);
}

void test_game_cancel_against_pending() {
  tp::Game g(battle_cfg());
  g.reset(1);
  g.queue_garbage(3, 1);
  TP_CHECK(g.pending_incoming() == 3);

  g.fill_row_for_test(tp::kBoardHeight - 1);
  g.fill_row_for_test(tp::kBoardHeight - 2);
  g.apply(tp::Action::HardDrop);

  auto out = g.consume_outgoing();
  TP_CHECK(out.has_value());
  // send 1, pending 3 → cancel 1, outgoing 0, pending left 2
  TP_CHECK(out->raw_send == 1);
  TP_CHECK(out->cancelled == 1);
  TP_CHECK(out->lines_sent == 0);
  // Remaining pending injected on spawn after clear.
  TP_CHECK(g.pending_incoming() == 0);
  // 2 garbage rows should be on board (3-1 cancelled).
  int garbage_rows = 0;
  for (int y = 0; y < tp::kBoardHeight; ++y) {
    bool any_g = false;
    for (int x = 0; x < tp::kBoardWidth; ++x) {
      if (g.state().board.at(x, y).filled && g.state().board.at(x, y).type == tp::PieceType::Garbage) {
        any_g = true;
        break;
      }
    }
    if (any_g) {
      ++garbage_rows;
    }
  }
  TP_CHECK(garbage_rows == 2);
}

void test_inject_top_out() {
  tp::Game g(battle_cfg());
  g.reset(1);
  // Nearly full stack with a hole per row so nothing line-clears on lock.
  for (int y = 0; y < tp::kBoardHeight; ++y) {
    for (int x = 0; x < tp::kBoardWidth; ++x) {
      if (x != (y % tp::kBoardWidth)) {
        g.set_cell_for_test(x, y, tp::PieceType::I);
      }
    }
  }
  g.queue_garbage(3, 0);
  g.apply(tp::Action::HardDrop);
  TP_CHECK(g.state().phase == tp::Phase::GameOver);
}

void test_solo_unaffected() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.piece_lock_flash_ms = 0;
  cfg.play_mode = tp::PlayMode::Sprint;
  tp::Game g(cfg);
  g.reset(1);
  TP_CHECK(!g.battle());
  g.queue_garbage(5, 0);
  TP_CHECK(g.pending_incoming() == 0);  // ignored when not battle

  g.set_lines_for_test(tp::kSprintLineGoal - 1);
  g.fill_row_for_test(tp::kBoardHeight - 1);
  g.apply(tp::Action::HardDrop);
  TP_CHECK(g.state().phase == tp::Phase::Finished);
  TP_CHECK(!g.consume_outgoing().has_value());
}

void test_battle_skips_sprint_finish() {
  tp::GameConfig cfg = battle_cfg();
  // battle forces Endless; even if caller asks Sprint, constraints win
  cfg.play_mode = tp::PlayMode::Sprint;
  tp::Game g(cfg);
  g.reset(1);
  TP_CHECK(g.config().play_mode == tp::PlayMode::Endless);
  g.set_lines_for_test(tp::kSprintLineGoal - 1);
  g.fill_row_for_test(tp::kBoardHeight - 1);
  g.apply(tp::Action::HardDrop);
  TP_CHECK(g.state().phase == tp::Phase::Playing);
}

void test_battle_forces_seven_bag() {
  tp::GameConfig cfg = battle_cfg();
  cfg.randomizer = tp::Randomizer::Freak;
  tp::Game g(cfg);
  TP_CHECK(g.config().randomizer == tp::Randomizer::SevenBag);
}

void test_perfect_clear_outgoing() {
  tp::Game g(battle_cfg());
  g.reset(1);
  // Four already-full bottom rows; lock a piece whose cells lie entirely inside
  // those rows so collapse leaves an empty board.
  for (int y = tp::kBoardHeight - 4; y < tp::kBoardHeight; ++y) {
    g.fill_row_for_test(y);
  }
  tp::ActivePiece p;
  p.spec = tp::PieceSpec::classic(tp::PieceType::O);
  p.x = 4;
  p.y = tp::kBoardHeight - 2;
  p.rotation = 0;
  p.alive = true;
  g.set_active_for_test(p);
  g.apply(tp::Action::HardDrop);
  auto out = g.consume_outgoing();
  TP_CHECK(out.has_value());
  TP_CHECK(out->perfect_clear);
  TP_CHECK(out->lines_cleared == 4);
  // Quad 4 + PC 6
  TP_CHECK(out->raw_send == 10);
  TP_CHECK(out->lines_sent == 10);
  TP_CHECK(g.state().board.empty());
}

}  // namespace

int main() {
  test_attack_table_ordinary();
  test_attack_b2b_combo_pc();
  test_combo_bonus_schedule();
  test_apply_cancel();
  test_board_inject_shared_hole();
  test_board_shift_up_discards_top();
  test_game_queue_inject_on_spawn();
  test_game_outgoing_double();
  test_game_cancel_against_pending();
  test_inject_top_out();
  test_solo_unaffected();
  test_battle_skips_sprint_finish();
  test_battle_forces_seven_bag();
  test_perfect_clear_outgoing();
  return tp::test::summary("tp_battle_tests");
}
