#include "input/Input.hpp"
#include "terminal/Keys.hpp"
#include "test_assert.hpp"

namespace {

void feed_str(tp::KeyDecoder& d, const char* s) {
  for (const char* p = s; *p; ++p) {
    d.feed(static_cast<unsigned char>(*p));
  }
}

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

void test_default_scores_key() {
  tp::Keybinds kb;
  TP_CHECK(kb.format_list(kb.scores) == "h");
  // `h` must not be claimed by left (old vim hjkl default).
  for (const auto& k : kb.left) {
    TP_CHECK(!(k.key == tp::Key::Char && k.ch == 'h'));
  }
  const auto act = kb.action_for(tp::KeyEvent{tp::Key::Char, 'h'});
  TP_CHECK(act.has_value());
  TP_CHECK(*act == tp::Action::Scores);
}

void test_kitty_csi_u_decode() {
  tp::KeyDecoder d;
  // Press 'a' (97), release 'a'
  feed_str(d, "\x1b[97;1:1u");
  tp::KeyEvent ev;
  TP_CHECK(d.poll(ev));
  TP_CHECK(ev.key == tp::Key::Char && ev.ch == 'a');
  TP_CHECK(ev.type == tp::KeyEventType::Press);

  feed_str(d, "\x1b[97;1:3u");
  TP_CHECK(d.poll(ev));
  TP_CHECK(ev.key == tp::Key::Char && ev.ch == 'a');
  TP_CHECK(ev.type == tp::KeyEventType::Release);

  // Left arrow via PUA + release
  feed_str(d, "\x1b[57350;1:1u");
  TP_CHECK(d.poll(ev));
  TP_CHECK(ev.key == tp::Key::Left);
  TP_CHECK(ev.type == tp::KeyEventType::Press);

  feed_str(d, "\x1b[57350;1:3u");
  TP_CHECK(d.poll(ev));
  TP_CHECK(ev.key == tp::Key::Left);
  TP_CHECK(ev.type == tp::KeyEventType::Release);

  // Ctrl+C via CSI u
  feed_str(d, "\x1b[99;5u");
  TP_CHECK(d.poll(ev));
  TP_CHECK(ev.key == tp::Key::CtrlC);

  // Legacy arrow still works
  feed_str(d, "\x1b[C");
  TP_CHECK(d.poll(ev));
  TP_CHECK(ev.key == tp::Key::Right);
  TP_CHECK(ev.type == tp::KeyEventType::Press);
}

void test_key_up_aware_hold_survives_rotate() {
  tp::InputConfig cfg;
  cfg.move_interval_ms = 50;
  cfg.release_ms = 100;  // DAS before ARR
  tp::Input input(cfg);
  input.set_key_up_aware(true);

  // Hold left
  input.on_key(tp::KeyEvent{tp::Key::Left, 0, tp::KeyEventType::Press});
  TP_CHECK(input.drain().size() == 1);

  // Rotate while held - must not clear left
  input.on_key(tp::KeyEvent{tp::Key::Char, 'x', tp::KeyEventType::Press});
  auto acts = input.drain();
  TP_CHECK(acts.size() == 1);
  TP_CHECK(acts[0] == tp::Action::RotateCW);

  // During DAS window: no ARR steps yet
  input.tick(50);
  TP_CHECK(input.drain().empty());

  // Past DAS: ARR at move_interval
  input.tick(50);  // held_ms = 100 -> arm ARR
  TP_CHECK(input.drain().empty());
  input.tick(50);
  acts = input.drain();
  TP_CHECK(acts.size() == 1);
  TP_CHECK(acts[0] == tp::Action::Left);

  // Protocol repeats must not inject extra steps
  input.on_key(tp::KeyEvent{tp::Key::Left, 0, tp::KeyEventType::Repeat});
  input.on_key(tp::KeyEvent{tp::Key::Left, 0, tp::KeyEventType::Repeat});
  TP_CHECK(input.drain().empty());

  // Release stops further steps
  input.on_key(tp::KeyEvent{tp::Key::Left, 0, tp::KeyEventType::Release});
  input.tick(200);
  TP_CHECK(input.drain().empty());
}

void test_key_up_aware_intervals() {
  tp::InputConfig cfg;
  cfg.move_interval_ms = 100;
  cfg.soft_drop_interval_ms = 80;
  cfg.release_ms = 50;  // short DAS
  tp::Input input(cfg);
  input.set_key_up_aware(true);

  input.on_key(tp::KeyEvent{tp::Key::Right, 0, tp::KeyEventType::Press});
  TP_CHECK(input.drain().size() == 1);

  // 50ms DAS arm, then 250ms ARR -> expect 2 steps (100 + 100), not 250/OS-rate
  input.tick(50);
  TP_CHECK(input.drain().empty());
  input.tick(250);
  auto acts = input.drain();
  TP_CHECK(acts.size() == 2);
  TP_CHECK(acts[0] == tp::Action::Right);
  TP_CHECK(acts[1] == tp::Action::Right);

  input.on_key(tp::KeyEvent{tp::Key::Right, 0, tp::KeyEventType::Release});

  input.on_key(tp::KeyEvent{tp::Key::Down, 0, tp::KeyEventType::Press});
  TP_CHECK(input.drain().size() == 1);
  input.tick(50);   // DAS
  input.tick(160);  // ARR -> 2 soft drops at 80ms
  acts = input.drain();
  TP_CHECK(acts.size() == 2);
  TP_CHECK(acts[0] == tp::Action::SoftDrop);
  TP_CHECK(acts[1] == tp::Action::SoftDrop);
}

void test_keyboard_protocol_parse() {
  TP_CHECK(tp::parse_keyboard_protocol("auto") == tp::KeyboardProtocol::Auto);
  TP_CHECK(tp::parse_keyboard_protocol("kitty") == tp::KeyboardProtocol::Kitty);
  TP_CHECK(tp::parse_keyboard_protocol("legacy") == tp::KeyboardProtocol::Legacy);
  TP_CHECK(tp::parse_keyboard_protocol("on") == tp::KeyboardProtocol::Kitty);
  TP_CHECK(tp::parse_keyboard_protocol("off") == tp::KeyboardProtocol::Legacy);
  TP_CHECK(std::string(tp::keyboard_protocol_token(tp::KeyboardProtocol::Auto)) == "auto");
  TP_CHECK(std::string(tp::keyboard_protocol_token(tp::KeyboardProtocol::Kitty)) == "kitty");
  TP_CHECK(std::string(tp::keyboard_protocol_token(tp::KeyboardProtocol::Legacy)) == "legacy");
}

}  // namespace

int main() {
  test_tap_moves_exactly_once();
  test_hold_steps_after_key_repeat();
  test_sonic_and_hard_drop_keys();
  test_custom_keybinds();
  test_default_scores_key();
  test_kitty_csi_u_decode();
  test_key_up_aware_hold_survives_rotate();
  test_key_up_aware_intervals();
  test_keyboard_protocol_parse();
  return tp::test::summary("tp_input_tests");
}
