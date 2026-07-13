#include "app/SettingsMenu.hpp"

#include <algorithm>
#include <cctype>

namespace tp {
namespace {

int clamp_int(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }

int& timing_ref(Settings& s, SettingsItem item) {
  switch (item) {
    case SettingsItem::MoveInterval:
      return s.input.move_interval_ms;
    case SettingsItem::SoftDropInterval:
      return s.input.soft_drop_interval_ms;
    case SettingsItem::ReleaseMs:
      return s.input.release_ms;
    case SettingsItem::LockDelay:
      return s.game.lock_delay_ms;
    case SettingsItem::LinesPerLevel:
      return s.game.lines_per_level;
    case SettingsItem::NextCount:
      return s.game.next_count;
    default:
      break;
  }
  return s.input.move_interval_ms;
}

void timing_bounds(SettingsItem item, int& lo, int& hi, int& step) {
  lo = 10;
  hi = 500;
  step = 5;
  switch (item) {
    case SettingsItem::MoveInterval:
    case SettingsItem::SoftDropInterval:
      lo = 10;
      hi = 200;
      step = 5;
      break;
    case SettingsItem::ReleaseMs:
      lo = 40;
      hi = 500;
      step = 10;
      break;
    case SettingsItem::LockDelay:
      lo = 0;
      hi = 2000;
      step = 50;
      break;
    case SettingsItem::LinesPerLevel:
      lo = 1;
      hi = 50;
      step = 1;
      break;
    case SettingsItem::NextCount:
      lo = 1;
      hi = kNextQueueMax;
      step = 1;
      break;
    default:
      break;
  }
}

}  // namespace

void SettingsMenu::open(const Settings& current) {
  draft_ = current;
  selected_ = 0;
  scroll_ = 0;
  capturing_ = false;
  dirty_ = false;
  result_ = Result::None;
  status_.clear();
}

SettingsMenu::Result SettingsMenu::take_result() {
  const Result r = result_;
  result_ = Result::None;
  return r;
}

SettingsMenuView SettingsMenu::view() const {
  return SettingsMenuView{draft_, selected_, scroll_, capturing_, dirty_, status_};
}

bool SettingsMenu::is_timing(SettingsItem item) {
  return item <= SettingsItem::NextCount;
}

bool SettingsMenu::is_keybind(SettingsItem item) {
  return item >= SettingsItem::KeyLeft && item <= SettingsItem::KeySettings;
}

void SettingsMenu::adjust(int delta) {
  const auto item = static_cast<SettingsItem>(selected_);
  if (item == SettingsItem::Randomizer) {
    const int n = static_cast<int>(Randomizer::Freak) + 1;
    int cur = static_cast<int>(draft_.game.randomizer);
    cur = (cur + delta) % n;
    if (cur < 0) {
      cur += n;
    }
    draft_.game.randomizer = static_cast<Randomizer>(cur);
    mark_dirty();
    return;
  }
  if (!is_timing(item)) {
    return;
  }
  int lo = 0;
  int hi = 0;
  int step = 1;
  timing_bounds(item, lo, hi, step);
  int& v = timing_ref(draft_, item);
  v = clamp_int(v + delta * step, lo, hi);
  if (item != SettingsItem::LockDelay && v < 1) {
    v = 1;
  }
  mark_dirty();
}

std::vector<KeySpec>* SettingsMenu::key_list(SettingsItem item) {
  auto& k = draft_.input.keys;
  switch (item) {
    case SettingsItem::KeyLeft:
      return &k.left;
    case SettingsItem::KeyRight:
      return &k.right;
    case SettingsItem::KeySoftDrop:
      return &k.soft_drop;
    case SettingsItem::KeyHardDrop:
      return &k.hard_drop;
    case SettingsItem::KeySonicDrop:
      return &k.sonic_drop;
    case SettingsItem::KeyRotateCw:
      return &k.rotate_cw;
    case SettingsItem::KeyRotateCcw:
      return &k.rotate_ccw;
    case SettingsItem::KeyHold:
      return &k.hold;
    case SettingsItem::KeyPause:
      return &k.pause;
    case SettingsItem::KeyQuit:
      return &k.quit;
    case SettingsItem::KeyRestart:
      return &k.restart;
    case SettingsItem::KeySettings:
      return &k.settings;
    default:
      return nullptr;
  }
}

void SettingsMenu::mark_dirty() {
  dirty_ = true;
  status_.clear();
}

void SettingsMenu::move_sel(int delta) {
  selected_ = (selected_ + delta + kSettingsItemCount) % kSettingsItemCount;
  // Keep selection roughly in view (renderer uses ~12 visible rows).
  constexpr int kVisible = 12;
  if (selected_ < scroll_) {
    scroll_ = selected_;
  } else if (selected_ >= scroll_ + kVisible) {
    scroll_ = selected_ - kVisible + 1;
  }
}

void SettingsMenu::reset_defaults() {
  draft_ = Settings{};
  mark_dirty();
  status_ = "Defaults restored (not saved yet)";
}

void SettingsMenu::activate() {
  const auto item = static_cast<SettingsItem>(selected_);
  if (item == SettingsItem::Randomizer) {
    adjust(1);
    return;
  }
  if (is_keybind(item)) {
    capturing_ = true;
    status_ = "Press a key...  Esc cancel";
    return;
  }
  if (item == SettingsItem::Save) {
    result_ = Result::Saved;
    dirty_ = false;
    status_ = "Saved ~/.config/tpoly/.tpolyrc";
    return;
  }
  if (item == SettingsItem::Reset) {
    reset_defaults();
    return;
  }
  if (item == SettingsItem::Back) {
    result_ = Result::Back;
    return;
  }
}

void SettingsMenu::capture(const KeyEvent& ev) {
  if (ev.key == Key::Esc) {
    capturing_ = false;
    status_ = "Rebind cancelled";
    return;
  }
  if (ev.key == Key::CtrlC || ev.key == Key::None || ev.key == Key::Backspace) {
    return;
  }

  KeySpec spec;
  if (ev.key == Key::Char) {
    spec = KeySpec{Key::Char, ev.ch};
  } else {
    spec = KeySpec{ev.key, 0};
  }
  const std::string tok = spec.token();
  if (tok.empty()) {
    status_ = "Key not supported";
    return;
  }

  auto* list = key_list(static_cast<SettingsItem>(selected_));
  if (list == nullptr) {
    capturing_ = false;
    return;
  }
  (void)draft_.input.keys.set_list(*list, tok);
  capturing_ = false;
  mark_dirty();
  status_ = "Bound to " + tok;
}

void SettingsMenu::on_key(const KeyEvent& ev) {
  if (capturing_) {
    capture(ev);
    return;
  }

  if (ev.key == Key::Esc) {
    result_ = Result::Back;
    return;
  }
  if (ev.key == Key::Up) {
    move_sel(-1);
    return;
  }
  if (ev.key == Key::Down) {
    move_sel(1);
    return;
  }
  if (ev.key == Key::Left) {
    adjust(-1);
    return;
  }
  if (ev.key == Key::Right) {
    adjust(1);
    return;
  }
  if (ev.key == Key::Enter) {
    activate();
    return;
  }
  if (ev.key == Key::Char) {
    const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(ev.ch)));
    if (c == 'k' || c == 'w') {
      move_sel(-1);
    } else if (c == 'j' || c == 's') {
      move_sel(1);
    } else if (c == 'h' || c == 'a' || c == '-') {
      adjust(-1);
    } else if (c == 'l' || c == 'd' || c == '+' || c == '=') {
      adjust(1);
    }
  }
}

}  // namespace tp
