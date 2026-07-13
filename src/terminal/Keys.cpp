#include "terminal/Keys.hpp"

namespace tp {

void KeyDecoder::feed(unsigned char byte) {
  switch (mode_) {
    case Mode::Normal:
      if (byte == 0x1b) {
        mode_ = Mode::Esc;
        return;
      }
      if (byte == 0x03) {
        queue_.push_back(KeyEvent{Key::CtrlC, 0});
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
        return;
      }
      // Lone Esc (or unknown ESC-seq start): emit Esc, re-feed byte.
      queue_.push_back(KeyEvent{Key::Esc, 0});
      mode_ = Mode::Normal;
      feed(byte);
      break;

    case Mode::Csi:
      mode_ = Mode::Normal;
      switch (byte) {
        case 'A':
          queue_.push_back(KeyEvent{Key::Up, 0});
          break;
        case 'B':
          queue_.push_back(KeyEvent{Key::Down, 0});
          break;
        case 'C':
          queue_.push_back(KeyEvent{Key::Right, 0});
          break;
        case 'D':
          queue_.push_back(KeyEvent{Key::Left, 0});
          break;
        default:
          // Unknown CSI — ignore.
          break;
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
