#pragma once

#include <cstddef>
#include <string_view>
#include <termios.h>

namespace tp {

struct TermSize {
  int rows = 0;
  int cols = 0;
};

// Owns tty state for the process lifetime. Restores the terminal on destroy,
// atexit, and common fatal signals.
class Terminal {
 public:
  Terminal();
  ~Terminal();

  Terminal(const Terminal&) = delete;
  Terminal& operator=(const Terminal&) = delete;

  [[nodiscard]] bool ok() const { return ok_; }
  [[nodiscard]] bool color_enabled() const { return color_; }
  [[nodiscard]] TermSize size() const;

  void write(std::string_view s);
  void flush();

  // Atomic-ish frame present: optional synchronized update, then bytes, then flush.
  void present(std::string_view frame);

  // Non-blocking: returns true and sets ch when a byte is available.
  [[nodiscard]] bool read_byte(unsigned char& ch);

  void clear_screen();
  void move_cursor(int row, int col);  // 1-based
  void hide_cursor();
  void show_cursor();
  void set_fg(int ansi_color);  // 0–7 or 8–15 bright if supported via 90–97
  void set_bg(int ansi_color);  // 0–7 normal, 8–15 bright (100–107)
  void set_bold(bool on);
  void set_dim(bool on);
  void reset_attrs();

  // Enter/leave alternate screen (full-screen apps).
  void enter_alt_screen();
  void leave_alt_screen();

 private:
  bool ok_ = false;
  bool color_ = false;
  bool alt_screen_ = false;
  bool raw_ = false;
  termios original_{};
};

}  // namespace tp
