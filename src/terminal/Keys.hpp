#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tp {

enum class Key : std::uint8_t {
  None = 0,
  Left,
  Right,
  Up,
  Down,
  Space,
  Enter,
  Esc,
  Backspace,
  Char,  // printable; see KeyEvent::ch
  CtrlC,
};

enum class KeyEventType : std::uint8_t {
  Press = 1,
  Repeat = 2,
  Release = 3,
};

// Modifier bitfield (Kitty encoding without the +1 offset).
inline constexpr int kModShift = 1;
inline constexpr int kModAlt = 2;
inline constexpr int kModCtrl = 4;
inline constexpr int kModSuper = 8;

struct KeyEvent {
  Key key = Key::None;
  char ch = 0;  // valid when key == Char (lowercase folded by Input if desired)
  KeyEventType type = KeyEventType::Press;
  int mods = 0;  // bitfield; 0 = none
};

// Parses a stream of raw stdin bytes into key events (legacy CSI + Kitty CSI u).
class KeyDecoder {
 public:
  void feed(unsigned char byte);
  [[nodiscard]] bool poll(KeyEvent& out);
  void reset();

 private:
  enum class Mode : std::uint8_t { Normal, Esc, Csi };

  void finish_csi(unsigned char final_byte);
  void emit_from_codepoint(int code, KeyEventType type, int mods);
  [[nodiscard]] static KeyEventType parse_event_type(int raw);
  [[nodiscard]] static int parse_mods(int raw);

  Mode mode_ = Mode::Normal;
  std::string csi_;
  std::vector<KeyEvent> queue_;
};

}  // namespace tp
