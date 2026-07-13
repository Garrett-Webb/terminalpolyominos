#include "input/Input.hpp"
#include "terminal/Keys.hpp"
#include "test_assert.hpp"

namespace {

void test_tap_moves_exactly_once() {
  tp::InputConfig cfg;
  cfg.move_interval_ms = 50;
  cfg.release_ms = 120;
  tp::Input input(cfg);

  input.on_key(tp::KeyEvent{tp::Key::Left, 0});
  auto first = input.drain();
  TP_CHECK(first.size() == 1);
  TP_CHECK(first[0] == tp::Action::Left);

  int moves = 0;
  for (int t = 0; t < 500; t += 16) {
    input.tick(16);
    moves += static_cast<int>(input.drain().size());
  }
  TP_CHECK(moves == 0);
}

void test_hold_steps_after_key_repeat() {
  tp::InputConfig cfg;
  cfg.move_interval_ms = 50;
  cfg.release_ms = 120;
  tp::Input input(cfg);

  input.on_key(tp::KeyEvent{tp::Key::Right, 0});
  (void)input.drain();

  // Confirm hold via key-repeat, then advance past one interval.
  input.on_key(tp::KeyEvent{tp::Key::Right, 0});
  TP_CHECK(input.drain().empty());

  input.tick(50);
  input.on_key(tp::KeyEvent{tp::Key::Right, 0});
  auto stepped = input.drain();
  TP_CHECK(stepped.size() == 1);
  TP_CHECK(stepped[0] == tp::Action::Right);
}

void test_sonic_and_hard_drop_keys() {
  tp::Input input;
  input.on_key(tp::KeyEvent{tp::Key::Space, 0});
  auto sonic = input.drain();
  TP_CHECK(sonic.size() == 1);
  TP_CHECK(sonic[0] == tp::Action::SonicDrop);

  input.on_key(tp::KeyEvent{tp::Key::Up, 0});
  auto hard = input.drain();
  TP_CHECK(hard.size() == 1);
  TP_CHECK(hard[0] == tp::Action::HardDrop);
}

void test_custom_keybinds() {
  tp::InputConfig cfg;
  TP_CHECK(cfg.keys.set_list(cfg.keys.sonic_drop, "f"));
  TP_CHECK(cfg.keys.set_list(cfg.keys.hard_drop, "space"));
  tp::Input input(cfg);

  input.on_key(tp::KeyEvent{tp::Key::Char, 'f'});
  auto sonic = input.drain();
  TP_CHECK(sonic.size() == 1);
  TP_CHECK(sonic[0] == tp::Action::SonicDrop);

  input.on_key(tp::KeyEvent{tp::Key::Space, 0});
  auto hard = input.drain();
  TP_CHECK(hard.size() == 1);
  TP_CHECK(hard[0] == tp::Action::HardDrop);

  // Default Space binding removed.
  input.on_key(tp::KeyEvent{tp::Key::Up, 0});
  TP_CHECK(input.drain().empty());
}

}  // namespace

int main() {
  test_tap_moves_exactly_once();
  test_hold_steps_after_key_repeat();
  test_sonic_and_hard_drop_keys();
  test_custom_keybinds();
  return tp::test::summary("tp_input_tests");
}
