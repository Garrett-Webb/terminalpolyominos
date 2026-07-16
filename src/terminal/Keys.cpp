#include "terminal/Keys.hpp"

#include <cstdlib>
#include <string_view>

namespace tp {
namespace {

// Kitty PUA functional keys (also accept legacy C0 numbers for Esc/Enter/…).
constexpr int kKittyEsc = 57344;
constexpr int kKittyEnter = 57345;
constexpr int kKittyTab = 57346;
constexpr int kKittyBackspace = 57347;
constexpr int kKittyLeft = 57350;
constexpr int kKittyRight = 57351;
constexpr int kKittyUp = 57352;
constexpr int kKittyDown = 57353;
constexpr int kKittyKpLeft = 57417;
constexpr int kKittyKpRight = 57418;
constexpr int kKittyKpUp = 57419;
constexpr int kKittyKpDown = 57420;
constexpr int kKittyKpEnter = 57414;

bool parse_int_field(std::string_view s, int& out) {
  if (s.empty()) {
    out = 0;
    return true;
  }
  char* end = nullptr;
  const long v = std::strtol(std::string(s).c_str(), &end, 10);
  if (end == s.data() || *end != '\0') {
    return false;
  }
  out = static_cast<int>(v);
  return true;
}

// Split "a:b:c" -> first subfield only as int (empty -> default_val).
int first_subfield(std::string_view field, int default_val) {
  const auto colon = field.find(':');
  const std::string_view head = colon == std::string_view::npos ? field : field.substr(0, colon);
  if (head.empty()) {
    return default_val;
  }
  int v = default_val;
  if (!parse_int_field(head, v)) {
    return default_val;
  }
  return v;
}

int second_subfield(std::string_view field, int default_val) {
  const auto colon = field.find(':');
  if (colon == std::string_view::npos) {
    return default_val;
  }
  const std::string_view rest = field.substr(colon + 1);
  const auto colon2 = rest.find(':');
  const std::string_view head = colon2 == std::string_view::npos ? rest : rest.substr(0, colon2);
  if (head.empty()) {
    return default_val;
  }
  int v = default_val;
  if (!parse_int_field(head, v)) {
    return default_val;
  }
  return v;
}

}  // namespace

void KeyDecoder::reset() {
  mode_ = Mode::Normal;
  csi_.clear();
  queue_.clear();
}

KeyEventType KeyDecoder::parse_event_type(int raw) {
  if (raw == 2) {
    return KeyEventType::Repeat;
  }
  if (raw == 3) {
    return KeyEventType::Release;
  }
  return KeyEventType::Press;
}

int KeyDecoder::parse_mods(int raw) {
  // Kitty encodes modifiers as (1 + bitfield). Default missing field = 1 -> no mods.
  if (raw <= 1) {
    return 0;
  }
  return raw - 1;
}

void KeyDecoder::emit_from_codepoint(int code, KeyEventType type, int mods) {
  KeyEvent ev;
  ev.type = type;
  ev.mods = mods;

  if ((mods & kModCtrl) != 0 && (code == 'c' || code == 'C' || code == 99 || code == 67)) {
    ev.key = Key::CtrlC;
    queue_.push_back(ev);
    return;
  }

  switch (code) {
    case 27:
    case kKittyEsc:
      ev.key = Key::Esc;
      break;
    case 13:
    case kKittyEnter:
    case kKittyKpEnter:
      ev.key = Key::Enter;
      break;
    case 9:
    case kKittyTab:
      // Tab unused by gameplay; ignore.
      return;
    case 127:
    case 8:
    case kKittyBackspace:
      ev.key = Key::Backspace;
      break;
    case 32:
      ev.key = Key::Space;
      break;
    case kKittyLeft:
    case kKittyKpLeft:
      ev.key = Key::Left;
      break;
    case kKittyRight:
    case kKittyKpRight:
      ev.key = Key::Right;
      break;
    case kKittyUp:
    case kKittyKpUp:
      ev.key = Key::Up;
      break;
    case kKittyDown:
    case kKittyKpDown:
      ev.key = Key::Down;
      break;
    default:
      if (code >= 32 && code < 127) {
        ev.key = Key::Char;
        ev.ch = static_cast<char>(code);
      } else {
        return;  // unknown functional key
      }
      break;
  }
  queue_.push_back(ev);
}

void KeyDecoder::finish_csi(unsigned char final_byte) {
  mode_ = Mode::Normal;
  const std::string params = csi_;
  csi_.clear();

  // Ignore device replies we may see if detection overlapped: CSI ? … c / CSI ? … u
  if (!params.empty() && params[0] == '?') {
    return;
  }

  if (final_byte == 'u') {
    // CSI code[:alts] ; mods[:type] ; text u
    std::string_view p(params);
    std::string_view fields[3];
    int nfields = 0;
    std::size_t start = 0;
    for (std::size_t i = 0; i <= p.size() && nfields < 3; ++i) {
      if (i == p.size() || p[i] == ';') {
        fields[nfields++] = p.substr(start, i - start);
        start = i + 1;
      }
    }
    if (nfields == 0) {
      return;
    }
    const int code = first_subfield(fields[0], 0);
    int mods_raw = 1;
    int type_raw = 1;
    if (nfields >= 2) {
      mods_raw = first_subfield(fields[1], 1);
      type_raw = second_subfield(fields[1], 1);
    }
    emit_from_codepoint(code, parse_event_type(type_raw), parse_mods(mods_raw));
    return;
  }

  // Legacy / enhanced functional: CSI [params] A|B|C|D|H|F|~
  // Forms: CSI A  |  CSI 1;mods A  |  CSI 1;mods:type A (rare)
  Key key = Key::None;
  switch (final_byte) {
    case 'A':
      key = Key::Up;
      break;
    case 'B':
      key = Key::Down;
      break;
    case 'C':
      key = Key::Right;
      break;
    case 'D':
      key = Key::Left;
      break;
    default:
      return;
  }

  int mods_raw = 1;
  int type_raw = 1;
  if (!params.empty()) {
    // Take last `;`-separated field as mods[:type]; ignore leading "1".
    std::string_view p(params);
    const auto last_semi = p.rfind(';');
    const std::string_view last =
        last_semi == std::string_view::npos ? p : p.substr(last_semi + 1);
    mods_raw = first_subfield(last, 1);
    type_raw = second_subfield(last, 1);
    // Plain "1" alone means no mods (CSI 1 A).
    if (last_semi == std::string_view::npos && first_subfield(p, 1) == 1 &&
        p.find(':') == std::string_view::npos) {
      mods_raw = 1;
      type_raw = 1;
    }
  }

  KeyEvent ev;
  ev.key = key;
  ev.type = parse_event_type(type_raw);
  ev.mods = parse_mods(mods_raw);
  queue_.push_back(ev);
}

void KeyDecoder::feed(unsigned char byte) {
  switch (mode_) {
    case Mode::Normal:
      if (byte == 0x1b) {
        mode_ = Mode::Esc;
        return;
      }
      if (byte == 0x03) {
        queue_.push_back(KeyEvent{Key::CtrlC, 0, KeyEventType::Press, kModCtrl});
        return;
      }
      if (byte == 0x0d || byte == 0x0a) {
        queue_.push_back(KeyEvent{Key::Enter, 0});
        return;
      }
      if (byte == 0x7f || byte == 0x08) {
        queue_.push_back(KeyEvent{Key::Backspace, 0});
        return;
      }
      if (byte == ' ') {
        queue_.push_back(KeyEvent{Key::Space, 0});
        return;
      }
      if (byte >= 32 && byte < 127) {
        queue_.push_back(KeyEvent{Key::Char, static_cast<char>(byte)});
        return;
      }
      break;

    case Mode::Esc:
      if (byte == '[') {
        mode_ = Mode::Csi;
        csi_.clear();
        return;
      }
      // Lone Esc (or unknown ESC-seq start): emit Esc, re-feed byte.
      queue_.push_back(KeyEvent{Key::Esc, 0});
      mode_ = Mode::Normal;
      feed(byte);
      break;

    case Mode::Csi:
      // Accumulate until a final byte (0x40–0x7E), excluding intermediates we store.
      if (byte >= 0x40 && byte <= 0x7e) {
        finish_csi(byte);
        return;
      }
      // Parameter / intermediate bytes.
      if (csi_.size() < 64) {
        csi_.push_back(static_cast<char>(byte));
      } else {
        // Overflow - drop sequence.
        mode_ = Mode::Normal;
        csi_.clear();
      }
      break;
  }
}

bool KeyDecoder::poll(KeyEvent& out) {
  if (queue_.empty()) {
    // If we're stuck after ESC with no follower this frame, flush as Esc.
    if (mode_ == Mode::Esc) {
      mode_ = Mode::Normal;
      out = KeyEvent{Key::Esc, 0};
      return true;
    }
    return false;
  }
  out = queue_.front();
  queue_.erase(queue_.begin());
  return true;
}

}  // namespace tp
