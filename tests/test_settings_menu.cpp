#include "app/SettingsMenu.hpp"
#include "test_assert.hpp"

namespace {

void test_adjust_timing() {
  tp::SettingsMenu menu;
  tp::Settings base;
  base.input.move_interval_ms = 50;
  menu.open(base);

  // Right arrow increases move interval.
  menu.on_key(tp::KeyEvent{tp::Key::Right, 0});
  TP_CHECK(menu.draft().input.move_interval_ms == 55);
  TP_CHECK(menu.dirty());
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

}  // namespace

int main() {
  test_adjust_timing();
  test_save_and_back();
  test_rebind();
  test_toggle_freak_colors();
  return tp::test::summary("tp_settings_menu_tests");
}
