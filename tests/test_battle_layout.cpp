#include "game/Game.hpp"
#include "render/Renderer.hpp"
#include "test_assert.hpp"

namespace {

void test_fits_battle_min() {
  TP_CHECK(!tp::Renderer::fits_battle_min({24, 79}));
  TP_CHECK(tp::Renderer::fits_battle_min({24, 80}));
  TP_CHECK(tp::Renderer::fits_battle_min({30, 100}));
  TP_CHECK(!tp::Renderer::fits_min({24, 67}));
  TP_CHECK(tp::Renderer::fits_min({24, 68}));
}

void test_battle_layout_geometry() {
  const tp::BattleLayout bl = tp::Renderer::compute_battle_layout({24, 80});
  TP_CHECK(bl.self.scale >= 1);
  TP_CHECK(bl.self.cell_w == 3);
  TP_CHECK(bl.opp_cell_w == 1);
  TP_CHECK(bl.opp_cell_h == 1);
  TP_CHECK(!bl.opp_scale1);
  TP_CHECK(bl.opp_col > bl.self.next_col);
  TP_CHECK(bl.meter_col > bl.opp_col);
  TP_CHECK(bl.hint_row > bl.self.field_row);
}

void test_battle_layout_wide_opp_scale1() {
  const tp::BattleLayout bl = tp::Renderer::compute_battle_layout({30, 120});
  TP_CHECK(bl.opp_scale1);
  TP_CHECK(bl.opp_cell_w == 3);
  TP_CHECK(bl.opp_cell_h == 1);
}

void test_make_opp_view_silhouette() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.piece_lock_flash_ms = 0;
  tp::Game g(cfg);
  g.reset(1);
  TP_CHECK(g.state().active.alive);
  // Drop into the visible field (spawn often sits in buffer rows).
  for (int i = 0; i < 8; ++i) {
    g.apply(tp::Action::SoftDrop);
  }
  const tp::OppBoardView v = tp::make_opp_view(g.state());
  int sil = 0;
  for (int vr = 0; vr < tp::kVisibleRows; ++vr) {
    for (int x = 0; x < tp::kBoardWidth; ++x) {
      if (v.silhouette[static_cast<std::size_t>(vr)][static_cast<std::size_t>(x)]) {
        ++sil;
      }
    }
  }
  TP_CHECK(sil > 0);
  TP_CHECK(v.combo == 0);
}

void test_make_opp_view_stack() {
  tp::GameConfig cfg;
  cfg.clear_flash_ms = 0;
  cfg.piece_lock_flash_ms = 0;
  tp::Game g(cfg);
  g.reset(1);
  g.fill_row_for_test(tp::kBoardHeight - 1);
  g.fill_row_for_test(tp::kBoardHeight - 2);
  // Kill active influence on occupancy by hard-dropping then... just read board rows.
  const tp::OppBoardView v = tp::make_opp_view(g.state());
  int filled = 0;
  for (int x = 0; x < tp::kBoardWidth; ++x) {
    if (v.occupied[tp::kVisibleRows - 1][static_cast<std::size_t>(x)]) {
      ++filled;
    }
  }
  TP_CHECK(filled == tp::kBoardWidth);
  TP_CHECK(v.height >= 2);
}

}  // namespace

int main() {
  test_fits_battle_min();
  test_battle_layout_geometry();
  test_battle_layout_wide_opp_scale1();
  test_make_opp_view_silhouette();
  test_make_opp_view_stack();
  return tp::test::summary("tp_battle_layout_tests");
}
