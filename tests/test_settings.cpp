#include "test_assert.hpp"
#include "util/Settings.hpp"

#include "input/Input.hpp"
#include "terminal/Keys.hpp"

#include <optional>

namespace {

void test_parse_overrides() {
  const char* rc = R"(
# comment
move_interval_ms = 40
soft_drop_interval_ms=20
release_ms=100
lock_delay_ms=600
lines_per_level=5
unknown_key=1
)";
  const tp::Settings s = tp::Settings::parse(rc);
  TP_CHECK(s.input.move_interval_ms == 40);
  TP_CHECK(s.input.soft_drop_interval_ms == 20);
  TP_CHECK(s.input.release_ms == 100);
  TP_CHECK(s.game.lock_delay_ms == 600);
  TP_CHECK(s.game.lines_per_level == 5);
}

void test_serialize_roundtrip() {
  tp::Settings s;
  s.input.move_interval_ms = 55;
  s.game.lock_delay_ms = 400;
  TP_CHECK(s.input.keys.set_list(s.input.keys.rotate_cw, "e,u"));
  const tp::Settings again = tp::Settings::parse(s.serialize());
  TP_CHECK(again.input.move_interval_ms == 55);
  TP_CHECK(again.game.lock_delay_ms == 400);
  TP_CHECK(again.input.soft_drop_interval_ms == s.input.soft_drop_interval_ms);
  TP_CHECK(again.input.keys.rotate_cw.size() == 2);
  TP_CHECK(again.input.keys.action_for(tp::KeyEvent{tp::Key::Char, 'e'}) ==
           tp::Action::RotateCW);
  TP_CHECK(again.input.keys.action_for(tp::KeyEvent{tp::Key::Char, 'x'}) ==
           std::nullopt);
}

void test_bad_values_ignored() {
  const tp::Settings s = tp::Settings::parse("move_interval_ms=nope\nlock_delay_ms=-5\n");
  TP_CHECK(s.input.move_interval_ms == tp::InputConfig{}.move_interval_ms);
  TP_CHECK(s.game.lock_delay_ms == tp::GameConfig{}.lock_delay_ms);
}

void test_keybind_parse() {
  const tp::Settings s = tp::Settings::parse(
      "key_sonic_drop=s\nkey_hard_drop=space\nkey_left=bogus,a\n");
  TP_CHECK(s.input.keys.action_for(tp::KeyEvent{tp::Key::Char, 's'}) ==
           tp::Action::SonicDrop);
  TP_CHECK(s.input.keys.action_for(tp::KeyEvent{tp::Key::Space, 0}) ==
           tp::Action::HardDrop);
  TP_CHECK(s.input.keys.action_for(tp::KeyEvent{tp::Key::Char, 'a'}) ==
           tp::Action::Left);
  // Invalid-only list keeps defaults.
  const tp::Settings keep =
      tp::Settings::parse("key_quit=notakey\n");
  TP_CHECK(keep.input.keys.action_for(tp::KeyEvent{tp::Key::Char, 'q'}) ==
           tp::Action::Quit);
}

void test_randomizer_parse() {
  const tp::Settings a = tp::Settings::parse("randomizer=7+1\n");
  TP_CHECK(a.game.randomizer == tp::Randomizer::SevenPlusOne);
  const tp::Settings b = tp::Settings::parse("randomizer=7bag\n");
  TP_CHECK(b.game.randomizer == tp::Randomizer::SevenBag);
  const tp::Settings c = tp::Settings::parse(a.serialize());
  TP_CHECK(c.game.randomizer == tp::Randomizer::SevenPlusOne);
  const tp::Settings d = tp::Settings::parse("randomizer=random\n");
  TP_CHECK(d.game.randomizer == tp::Randomizer::FullRandom);
  const tp::Settings e = tp::Settings::parse(d.serialize());
  TP_CHECK(e.game.randomizer == tp::Randomizer::FullRandom);
}

void test_next_count_parse() {
  const tp::Settings a = tp::Settings::parse("next_count=5\n");
  TP_CHECK(a.game.next_count == 5);
  const tp::Settings b = tp::Settings::parse("next_count=0\n");
  TP_CHECK(b.game.next_count == 1);
  const tp::Settings c = tp::Settings::parse("next_count=9\n");
  TP_CHECK(c.game.next_count == tp::kNextQueueMax);
}

void test_ctrl_c_always_quits() {
  tp::Input input;
  input.on_key(tp::KeyEvent{tp::Key::CtrlC, 0});
  auto acts = input.drain();
  TP_CHECK(acts.size() == 1);
  TP_CHECK(acts[0] == tp::Action::Quit);
}

}  // namespace

int main() {
  test_parse_overrides();
  test_serialize_roundtrip();
  test_bad_values_ignored();
  test_keybind_parse();
  test_randomizer_parse();
  test_next_count_parse();
  test_ctrl_c_always_quits();
  return tp::test::summary("tp_settings_tests");
}
