#include "input/Input.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace tp {
namespace {

std::string lower_copy(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  for (unsigned char c : in) {
    out.push_back(static_cast<char>(std::tolower(c)));
  }
  return out;
}

bool any_match(const std::vector<KeySpec>& keys, const KeyEvent& ev) {
  for (const KeySpec& k : keys) {
    if (k.matches(ev)) {
      return true;
    }
  }
  return false;
}

KeySpec special(Key k) { return KeySpec{k, 0}; }

KeySpec letter(char c) {
  return KeySpec{Key::Char, static_cast<char>(std::tolower(static_cast<unsigned char>(c)))};
}

bool same_spec(const KeySpec& a, const KeySpec& b) {
  if (a.key != b.key) {
    return false;
  }
  if (a.key == Key::Char) {
    return std::tolower(static_cast<unsigned char>(a.ch)) ==
           std::tolower(static_cast<unsigned char>(b.ch));
  }
  return true;
}

void strip_specs(std::vector<KeySpec>& list, const std::vector<KeySpec>& claimed) {
  list.erase(std::remove_if(list.begin(), list.end(),
                            [&](const KeySpec& k) {
                              for (const KeySpec& c : claimed) {
                                if (same_spec(k, c)) {
                                  return true;
                                }
                              }
                              return false;
                            }),
             list.end());
}

}  // namespace

bool KeySpec::matches(const KeyEvent& ev) const {
  if (key == Key::Char) {
    if (ev.key != Key::Char) {
      return false;
    }
    return std::tolower(static_cast<unsigned char>(ev.ch)) ==
           std::tolower(static_cast<unsigned char>(ch));
  }
  return ev.key == key;
}

std::string KeySpec::token() const {
  switch (key) {
    case Key::Left:
      return "left";
    case Key::Right:
      return "right";
    case Key::Up:
      return "up";
    case Key::Down:
      return "down";
    case Key::Space:
      return "space";
    case Key::Enter:
      return "enter";
    case Key::Esc:
      return "esc";
    case Key::Backspace:
      return "backspace";
    case Key::Char:
      if (ch != 0) {
        return std::string(1, static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
      }
      return {};
    case Key::CtrlC:
      return "ctrl-c";
    case Key::None:
      return {};
  }
  return {};
}

std::optional<KeySpec> KeySpec::from_token(std::string_view t) {
  const std::string s = lower_copy(t);
  if (s.empty()) {
    return std::nullopt;
  }
  if (s == "left" || s == "arrowleft") {
    return special(Key::Left);
  }
  if (s == "right" || s == "arrowright") {
    return special(Key::Right);
  }
  if (s == "up" || s == "arrowup") {
    return special(Key::Up);
  }
  if (s == "down" || s == "arrowdown") {
    return special(Key::Down);
  }
  if (s == "space" || s == "spc") {
    return special(Key::Space);
  }
  if (s == "enter" || s == "return") {
    return special(Key::Enter);
  }
  if (s == "esc" || s == "escape") {
    return special(Key::Esc);
  }
  if (s == "backspace" || s == "bs") {
    return special(Key::Backspace);
  }
  if (s == "ctrl-c" || s == "ctrl+c" || s == "^c") {
    return special(Key::CtrlC);
  }
  if (s.size() == 1) {
    const unsigned char c = static_cast<unsigned char>(s[0]);
    if (std::isprint(c) && !std::isspace(c)) {
      return letter(static_cast<char>(c));
    }
  }
  return std::nullopt;
}

Keybinds::Keybinds() {
  // Arrows + wasd-ish letters. Do not claim `h` here — it is the default scores key.
  left = {special(Key::Left), letter('a')};
  right = {special(Key::Right), letter('d'), letter('l')};
  soft_drop = {special(Key::Down), letter('s'), letter('j')};
  hard_drop = {special(Key::Up)};
  sonic_drop = {special(Key::Space)};
  rotate_cw = {letter('x'), letter('w'), letter('k')};
  rotate_ccw = {letter('z')};
  hold = {letter('c')};
  pause = {special(Key::Esc), letter('p')};
  quit = {letter('q')};
  restart = {special(Key::Enter), letter('r')};
  settings = {letter('o')};
  scores = {letter('h')};
}

std::optional<Action> Keybinds::action_for(const KeyEvent& ev) const {
  // Always honor Ctrl+C as quit, even if remapped away in the rc.
  if (ev.key == Key::CtrlC) {
    return Action::Quit;
  }
  if (any_match(left, ev)) {
    return Action::Left;
  }
  if (any_match(right, ev)) {
    return Action::Right;
  }
  if (any_match(soft_drop, ev)) {
    return Action::SoftDrop;
  }
  if (any_match(hard_drop, ev)) {
    return Action::HardDrop;
  }
  if (any_match(sonic_drop, ev)) {
    return Action::SonicDrop;
  }
  if (any_match(rotate_cw, ev)) {
    return Action::RotateCW;
  }
  if (any_match(rotate_ccw, ev)) {
    return Action::RotateCCW;
  }
  if (any_match(hold, ev)) {
    return Action::Hold;
  }
  if (any_match(pause, ev)) {
    return Action::Pause;
  }
  if (any_match(quit, ev)) {
    return Action::Quit;
  }
  if (any_match(restart, ev)) {
    return Action::Restart;
  }
  if (any_match(settings, ev)) {
    return Action::Settings;
  }
  if (any_match(scores, ev)) {
    return Action::Scores;
  }
  return std::nullopt;
}

std::string Keybinds::format_list(const std::vector<KeySpec>& keys) const {
  std::ostringstream out;
  bool first = true;
  for (const KeySpec& k : keys) {
    const std::string tok = k.token();
    if (tok.empty()) {
      continue;
    }
    if (!first) {
      out << ',';
    }
    first = false;
    out << tok;
  }
  return out.str();
}

bool Keybinds::set_list(std::vector<KeySpec>& dest, std::string_view csv) {
  std::vector<KeySpec> parsed;
  std::string token;
  auto flush = [&] {
    // trim
    std::size_t b = 0;
    while (b < token.size() && std::isspace(static_cast<unsigned char>(token[b]))) {
      ++b;
    }
    std::size_t e = token.size();
    while (e > b && std::isspace(static_cast<unsigned char>(token[e - 1]))) {
      --e;
    }
    if (e > b) {
      if (auto spec = KeySpec::from_token(std::string_view(token).substr(b, e - b))) {
        parsed.push_back(*spec);
      }
    }
    token.clear();
  };

  for (char c : csv) {
    if (c == ',' || std::isspace(static_cast<unsigned char>(c))) {
      flush();
    } else {
      token.push_back(c);
    }
  }
  flush();

  // Reject empty / all-invalid lists so a typo does not wipe defaults.
  if (parsed.empty()) {
    return false;
  }
  remove_from_others(parsed, &dest);
  dest = std::move(parsed);
  return true;
}

void Keybinds::remove_from_others(const std::vector<KeySpec>& claimed,
                                  const std::vector<KeySpec>* keep) {
  auto maybe_strip = [&](std::vector<KeySpec>& list) {
    if (&list == keep) {
      return;
    }
    strip_specs(list, claimed);
  };
  maybe_strip(left);
  maybe_strip(right);
  maybe_strip(soft_drop);
  maybe_strip(hard_drop);
  maybe_strip(sonic_drop);
  maybe_strip(rotate_cw);
  maybe_strip(rotate_ccw);
  maybe_strip(hold);
  maybe_strip(pause);
  maybe_strip(quit);
  maybe_strip(restart);
  maybe_strip(settings);
  maybe_strip(scores);
}

Input::Input(InputConfig config) : config_(config) {}

void Input::reset() {
  clear_dirs();
  out_.clear();
}

void Input::queue(Action action) { out_.push_back(action); }

std::vector<Action> Input::drain() {
  std::vector<Action> out;
  out.swap(out_);
  return out;
}

Action Input::action_for(Dir dir) {
  switch (dir) {
    case Dir::Left:
      return Action::Left;
    case Dir::Right:
      return Action::Right;
    case Dir::SoftDrop:
      return Action::SoftDrop;
    case Dir::None:
      break;
  }
  return Action::None;
}

void Input::clear_dirs() {
  left_ = Stick{};
  right_ = Stick{};
  soft_ = Stick{};
}

void Input::press_dir(Dir dir) {
  Stick* stick = nullptr;
  switch (dir) {
    case Dir::Left:
      right_ = Stick{};
      stick = &left_;
      break;
    case Dir::Right:
      left_ = Stick{};
      stick = &right_;
      break;
    case Dir::SoftDrop:
      stick = &soft_;
      break;
    case Dir::None:
      return;
  }

  if (stick->held) {
    // Second+ event for this direction = real hold (terminal key-repeat).
    stick->repeating = true;
    stick->since_event_ms = 0;
    return;
  }

  // Fresh press: exactly one step. Repeats wait for key-repeat + move timer.
  *stick = Stick{};
  stick->held = true;
  stick->repeating = false;
  stick->since_event_ms = 0;
  stick->step_acc_ms = 0;
  queue(action_for(dir));
}

void Input::tick_dir(Stick& stick, Dir dir, int elapsed_ms, int interval_ms) {
  if (!stick.held) {
    return;
  }

  stick.since_event_ms += elapsed_ms;
  if (stick.since_event_ms >= config_.release_ms) {
    stick = Stick{};
    return;
  }

  // Single taps must not get a second block from the timer.
  if (!stick.repeating) {
    return;
  }

  stick.step_acc_ms += elapsed_ms;
  if (stick.step_acc_ms >= interval_ms) {
    stick.step_acc_ms -= interval_ms;
    if (stick.step_acc_ms >= interval_ms) {
      stick.step_acc_ms = interval_ms - 1;
    }
    queue(action_for(dir));
  }
}

void Input::tick(int elapsed_ms) {
  if (elapsed_ms < 0) {
    elapsed_ms = 0;
  }
  tick_dir(left_, Dir::Left, elapsed_ms, config_.move_interval_ms);
  tick_dir(right_, Dir::Right, elapsed_ms, config_.move_interval_ms);
  tick_dir(soft_, Dir::SoftDrop, elapsed_ms, config_.soft_drop_interval_ms);
}

void Input::on_key(const KeyEvent& ev) {
  const auto action = to_action(ev);
  if (!action.has_value()) {
    return;
  }

  switch (*action) {
    case Action::Left:
      press_dir(Dir::Left);
      return;
    case Action::Right:
      press_dir(Dir::Right);
      return;
    case Action::SoftDrop:
      press_dir(Dir::SoftDrop);
      return;
    case Action::HardDrop:
      soft_ = Stick{};
      queue(Action::HardDrop);
      return;
    case Action::SonicDrop:
      soft_ = Stick{};
      queue(Action::SonicDrop);
      return;
    case Action::RotateCW:
    case Action::RotateCCW:
    case Action::Hold:
    case Action::Settings:
    case Action::Scores:
      queue(*action);
      return;
    case Action::Pause:
    case Action::Quit:
    case Action::Restart:
      clear_dirs();
      queue(*action);
      return;
    case Action::None:
      return;
  }
}

std::optional<Action> Input::to_action(const KeyEvent& ev) const {
  return config_.keys.action_for(ev);
}

}  // namespace tp
