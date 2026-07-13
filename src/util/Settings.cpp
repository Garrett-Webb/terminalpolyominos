#include "util/Settings.hpp"

#include "util/Xdg.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace tp {
namespace {

std::string trim(std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() && std::isspace(static_cast<unsigned char>(in[b]))) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1]))) {
    --e;
  }
  return std::string(in.substr(b, e - b));
}

bool parse_int(std::string_view s, int& out) {
  if (s.empty()) {
    return false;
  }
  try {
    std::size_t idx = 0;
    const int v = std::stoi(std::string(s), &idx);
    if (idx != s.size()) {
      return false;
    }
    out = v;
    return true;
  } catch (...) {
    return false;
  }
}

void apply_kv(Settings& s, const std::string& key, const std::string& value) {
  auto& kb = s.input.keys;
  if (key == "key_left") {
    (void)kb.set_list(kb.left, value);
    return;
  }
  if (key == "key_right") {
    (void)kb.set_list(kb.right, value);
    return;
  }
  if (key == "key_soft_drop") {
    (void)kb.set_list(kb.soft_drop, value);
    return;
  }
  if (key == "key_hard_drop") {
    (void)kb.set_list(kb.hard_drop, value);
    return;
  }
  if (key == "key_sonic_drop") {
    (void)kb.set_list(kb.sonic_drop, value);
    return;
  }
  if (key == "key_rotate_cw") {
    (void)kb.set_list(kb.rotate_cw, value);
    return;
  }
  if (key == "key_rotate_ccw") {
    (void)kb.set_list(kb.rotate_ccw, value);
    return;
  }
  if (key == "key_hold") {
    (void)kb.set_list(kb.hold, value);
    return;
  }
  if (key == "key_pause") {
    (void)kb.set_list(kb.pause, value);
    return;
  }
  if (key == "key_quit") {
    (void)kb.set_list(kb.quit, value);
    return;
  }
  if (key == "key_restart") {
    (void)kb.set_list(kb.restart, value);
    return;
  }
  if (key == "key_settings") {
    (void)kb.set_list(kb.settings, value);
    return;
  }
  if (key == "randomizer") {
    const std::string v = trim(value);
    std::string lower;
    lower.reserve(v.size());
    for (unsigned char c : v) {
      lower.push_back(static_cast<char>(std::tolower(c)));
    }
    if (lower == "7bag" || lower == "7-bag" || lower == "seven" || lower == "bag7") {
      s.game.randomizer = Randomizer::SevenBag;
    } else if (lower == "7+1" || lower == "7plus1" || lower == "seven_plus_one" ||
               lower == "bag7+1") {
      s.game.randomizer = Randomizer::SevenPlusOne;
    } else if (lower == "random" || lower == "full" || lower == "fullrandom" ||
               lower == "full_random" || lower == "pure") {
      s.game.randomizer = Randomizer::FullRandom;
    } else if (lower == "torture") {
      s.game.randomizer = Randomizer::Torture;
    } else if (lower == "funk") {
      s.game.randomizer = Randomizer::Funk;
    } else if (lower == "freak") {
      s.game.randomizer = Randomizer::Freak;
    }
    return;
  }

  int n = 0;
  if (!parse_int(value, n) || n < 0) {
    return;
  }
  if (key == "move_interval_ms") {
    s.input.move_interval_ms = n == 0 ? 1 : n;
  } else if (key == "soft_drop_interval_ms") {
    s.input.soft_drop_interval_ms = n == 0 ? 1 : n;
  } else if (key == "release_ms") {
    s.input.release_ms = n == 0 ? 1 : n;
  } else if (key == "lock_delay_ms") {
    s.game.lock_delay_ms = n;
  } else if (key == "lines_per_level") {
    s.game.lines_per_level = n == 0 ? 1 : n;
  } else if (key == "next_count") {
    if (n < 1) {
      n = 1;
    } else if (n > kNextQueueMax) {
      n = kNextQueueMax;
    }
    s.game.next_count = n;
  }
}

const char* randomizer_token(Randomizer r) {
  switch (r) {
    case Randomizer::SevenPlusOne:
      return "7+1";
    case Randomizer::FullRandom:
      return "random";
    case Randomizer::Torture:
      return "torture";
    case Randomizer::Funk:
      return "funk";
    case Randomizer::Freak:
      return "freak";
    case Randomizer::SevenBag:
      break;
  }
  return "7bag";
}

std::string read_file(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    return {};
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

}  // namespace

std::string Settings::default_rc_text() {
  Settings d;
  return d.serialize();
}

std::string Settings::serialize() const {
  const auto& k = input.keys;
  std::ostringstream out;
  out << "# terminalpolyominos / tpoly settings\n"
      << "# Location: ~/.config/tpoly/.tpolyrc  (XDG_CONFIG_HOME/tpoly/.tpolyrc)\n"
      << "# Lines are key=value. Comments start with #.\n"
      << "#\n"
      << "# Input (milliseconds)\n"
      << "move_interval_ms=" << input.move_interval_ms << '\n'
      << "soft_drop_interval_ms=" << input.soft_drop_interval_ms << '\n'
      << "release_ms=" << input.release_ms << '\n'
      << "#\n"
      << "# Game\n"
      << "lock_delay_ms=" << game.lock_delay_ms << '\n'
      << "lines_per_level=" << game.lines_per_level << '\n'
      << "next_count=" << game.next_count << "   # 1.." << kNextQueueMax << '\n'
      << "randomizer=" << randomizer_token(game.randomizer)
      << "   # 7bag | 7+1 | random\n"
      << "#\n"
      << "# Keys — comma-separated tokens (left/right/up/down/space/enter/esc, or a letter).\n"
      << "# Ctrl+C always quits. Invalid tokens are ignored; empty lists keep defaults.\n"
      << "key_left=" << k.format_list(k.left) << '\n'
      << "key_right=" << k.format_list(k.right) << '\n'
      << "key_soft_drop=" << k.format_list(k.soft_drop) << '\n'
      << "key_hard_drop=" << k.format_list(k.hard_drop) << '\n'
      << "key_sonic_drop=" << k.format_list(k.sonic_drop) << '\n'
      << "key_rotate_cw=" << k.format_list(k.rotate_cw) << '\n'
      << "key_rotate_ccw=" << k.format_list(k.rotate_ccw) << '\n'
      << "key_hold=" << k.format_list(k.hold) << '\n'
      << "key_pause=" << k.format_list(k.pause) << '\n'
      << "key_quit=" << k.format_list(k.quit) << '\n'
      << "key_restart=" << k.format_list(k.restart) << '\n'
      << "key_settings=" << k.format_list(k.settings) << '\n';
  return out.str();
}

Settings Settings::parse(const std::string& text) {
  Settings s;
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    const auto hash = line.find('#');
    if (hash != std::string::npos) {
      line.resize(hash);
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string key = trim(std::string_view(line).substr(0, eq));
    const std::string value = trim(std::string_view(line).substr(eq + 1));
    apply_kv(s, key, value);
  }
  return s;
}

bool Settings::save() const {
  if (!ensure_dir(tpoly_config_dir())) {
    return false;
  }
  const std::string path = tpoly_rc_path();
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    return false;
  }
  out << serialize();
  return static_cast<bool>(out);
}

Settings Settings::load_or_create() {
  const std::string primary = tpoly_rc_path();
  const std::string legacy = tpoly_rc_legacy_path();

  std::string text = read_file(primary);
  if (text.empty()) {
    text = read_file(legacy);
  }

  if (text.empty()) {
    Settings fresh;
    (void)fresh.save();  // write defaults under ~/.config/tpoly/.tpolyrc
    return fresh;
  }

  Settings s = parse(text);
  // Upgrade older rc files missing newer keys.
  if (read_file(primary).empty() || text.find("key_settings=") == std::string::npos ||
      text.find("randomizer=") == std::string::npos ||
      text.find("next_count=") == std::string::npos) {
    (void)s.save();
  }
  return s;
}

}  // namespace tp
