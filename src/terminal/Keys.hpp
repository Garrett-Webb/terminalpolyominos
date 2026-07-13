#pragma once

#include <cstdint>
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

struct KeyEvent {
  Key key = Key::None;
  char ch = 0;  // valid when key == Char (lowercase folded by Input if desired)
};

// Parses a stream of raw stdin bytes into key events (handles CSI arrows).
class KeyDecoder {
 public:
  void feed(unsigned char byte);
  [[nodiscard]] bool poll(KeyEvent& out);

 private:
  enum class Mode : std::uint8_t { Normal, Esc, Csi };

  Mode mode_ = Mode::Normal;
  std::vector<KeyEvent> queue_;
};

}  // namespace tp
