#include "app/SettingsMenu.hpp"
#include "test_assert.hpp"

namespace {

void test_adjust_timing() {
  tp::SettingsMenu menu;
  tp::Settings base;
  base.input.move_interval_ms = 35;
  menu.open(base);

  // Right arrow increases move interval (+5ms step).
  menu.on_key(tp::KeyEvent{tp::Key::Right, 0});
  TP_CHECK(menu.draft().input.move_interval_ms == 40);
  TP_CHECK(menu.dirty());
}

void test_hold_nav_repeat() {
  tp::SettingsMenu menu;
  menu.open(tp::Settings{});
  TP_CHECK(menu.view().selected == 0);
  menu.on_key(tp::KeyEvent{tp::Key::Down, 0, tp::KeyEventType::Repeat});
  TP_CHECK(menu.view().selected == 1);
  menu.on_key(tp::KeyEvent{tp::Key::Up, 0, tp::KeyEventType::Repeat});
  TP_CHECK(menu.view().selected == 0);
  // Release must not move; Enter Repeat must not activate.
  menu.on_key(tp::KeyEvent{tp::Key::Down, 0, tp::KeyEventType::Release});
  TP_CHECK(menu.view().selected == 0);
  for (int i = 0; i < static_cast<int>(tp::SettingsItem::Save); ++i) {
    menu.on_key(tp::KeyEvent{tp::Key::Down, 0});
  }
  menu.on_key(tp::KeyEvent{tp::Key::Enter, 0, tp::KeyEventType::Repeat});
  TP_CHECK(menu.take_result() == tp::SettingsMenu::Result::None);
}

void test_scroll_reaches_bottom() {
  // Short viewport must still be able to bring Back into view.
  constexpr int kViewport = 8;
  TP_CHECK(tp::settings_list_rows() > kViewport);

  tp::SettingsMenu menu;
  menu.open(tp::Settings{});
  for (int i = 0; i < static_cast<int>(tp::SettingsItem::Back); ++i) {
    menu.on_key(tp::KeyEvent{tp::Key::Down, 0});
  }
  TP_CHECK(menu.view().selected == static_cast<int>(tp::SettingsItem::Back));
  menu.ensure_visible(kViewport);

  const int sel_v = tp::settings_item_visual_row(menu.view().selected);
  const int scroll = menu.view().scroll;
  TP_CHECK(sel_v >= scroll);
  TP_CHECK(sel_v < scroll + kViewport);
  TP_CHECK(scroll == tp::settings_list_rows() - kViewport);
}

void test_save_and_back() {
  tp::SettingsMenu menu;
  menu.open(tp::Settings{});
  // Jump to Save (index of Save).
  for (int i = 0; i < static_cast<int>(tp::SettingsItem::Save); ++i) {
    menu.on_key(tp::KeyEvent{tp::Key::Down, 0});
  }
  menu.on_key(tp::KeyEvent{tp::Key::Enter, 0});
  TP_CHECK(menu.take_result() == tp::SettingsMenu::Result::Saved);

  menu.on_key(tp::KeyEvent{tp::Key::Esc, 0});
  TP_CHECK(menu.take_result() == tp::SettingsMenu::Result::Back);
}

void test_rebind() {
  tp::SettingsMenu menu;
  menu.open(tp::Settings{});
  // Move to Key sonic drop.
  for (int i = 0; i < static_cast<int>(tp::SettingsItem::KeySonicDrop); ++i) {
    menu.on_key(tp::KeyEvent{tp::Key::Down, 0});
  }
  menu.on_key(tp::KeyEvent{tp::Key::Enter, 0});
  TP_CHECK(menu.view().capturing);
  menu.on_key(tp::KeyEvent{tp::Key::Char, 'f'});
  TP_CHECK(!menu.view().capturing);
  TP_CHECK(menu.draft().input.keys.action_for(tp::KeyEvent{tp::Key::Char, 'f'}) ==
           tp::Action::SonicDrop);
}

void test_toggle_freak_colors() {
  tp::SettingsMenu menu;
  tp::Settings base;
  TP_CHECK(base.game.freak_colors);
  menu.open(base);
  for (int i = 0; i < static_cast<int>(tp::SettingsItem::FreakColors); ++i) {
    menu.on_key(tp::KeyEvent{tp::Key::Down, 0});
  }
  menu.on_key(tp::KeyEvent{tp::Key::Right, 0});
  TP_CHECK(!menu.draft().game.freak_colors);
  TP_CHECK(menu.dirty());
  menu.on_key(tp::KeyEvent{tp::Key::Enter, 0});
  TP_CHECK(menu.draft().game.freak_colors);
}

void test_clear_scores_confirm() {
  tp::SettingsMenu menu;
  menu.open(tp::Settings{});
  for (int i = 0; i < static_cast<int>(tp::SettingsItem::ClearScores); ++i) {
    menu.on_key(tp::KeyEvent{tp::Key::Down, 0});
  }
  menu.on_key(tp::KeyEvent{tp::Key::Enter, 0});
  TP_CHECK(menu.view().confirming_clear);
  menu.on_key(tp::KeyEvent{tp::Key::Char, 'y'});
  menu.on_key(tp::KeyEvent{tp::Key::Char, 'e'});
  menu.on_key(tp::KeyEvent{tp::Key::Char, 's'});
  menu.on_key(tp::KeyEvent{tp::Key::Enter, 0});
  TP_CHECK(menu.take_result() == tp::SettingsMenu::Result::ClearedScores);
  TP_CHECK(!menu.view().confirming_clear);

  menu.open(tp::Settings{});
  for (int i = 0; i < static_cast<int>(tp::SettingsItem::ClearScores); ++i) {
    menu.on_key(tp::KeyEvent{tp::Key::Down, 0});
  }
  menu.on_key(tp::KeyEvent{tp::Key::Enter, 0});
  menu.on_key(tp::KeyEvent{tp::Key::Char, 'n'});
  menu.on_key(tp::KeyEvent{tp::Key::Enter, 0});
  TP_CHECK(menu.take_result() == tp::SettingsMenu::Result::None);
}

}  // namespace

int main() {
  test_adjust_timing();
  test_hold_nav_repeat();
  test_scroll_reaches_bottom();
  test_save_and_back();
  test_rebind();
  test_toggle_freak_colors();
  test_clear_scores_confirm();
  return tp::test::summary("tp_settings_menu_tests");
}
