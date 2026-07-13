#include "terminal/Terminal.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace tp {
namespace {

Terminal* g_active = nullptr;
termios g_original{};
bool g_have_original = false;
bool g_alt_screen = false;

void restore_tty_now() {
  if (g_alt_screen) {
    // Leave alt screen + show cursor + reset attrs.
    const char* seq = "\x1b[?1049l\x1b[?25h\x1b[0m";
    (void)!write(STDOUT_FILENO, seq, std::strlen(seq));
    g_alt_screen = false;
  } else {
    const char* seq = "\x1b[?25h\x1b[0m";
    (void)!write(STDOUT_FILENO, seq, std::strlen(seq));
  }
  if (g_have_original) {
    (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original);
  }
}

void on_signal(int sig) {
  restore_tty_now();
  _Exit(128 + sig);
}

void install_signal_handlers() {
  struct sigaction sa {};
  sa.sa_handler = on_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);
  sigaction(SIGQUIT, &sa, nullptr);
  sigaction(SIGABRT, &sa, nullptr);
  // SIGTSTP left alone so job control still works; we restore on resume if needed later.
}

bool env_truthy_no_color() {
  const char* v = std::getenv("NO_COLOR");
  return v != nullptr && v[0] != '\0';
}

bool stdout_is_tty() { return ::isatty(STDOUT_FILENO) == 1; }
bool stdin_is_tty() { return ::isatty(STDIN_FILENO) == 1; }

}  // namespace

Terminal::Terminal() {
  if (!stdin_is_tty() || !stdout_is_tty()) {
    std::fputs("terminalpolyominos: stdin and stdout must be a TTY\n", stderr);
    return;
  }

  if (tcgetattr(STDIN_FILENO, &original_) != 0) {
    std::perror("tcgetattr");
    return;
  }

  g_original = original_;
  g_have_original = true;
  g_active = this;
  std::atexit(restore_tty_now);
  install_signal_handlers();

  termios raw = original_;
  raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON | ISIG | IEXTEN));
  raw.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL | BRKINT | INPCK | ISTRIP));
  raw.c_oflag &= static_cast<tcflag_t>(~(OPOST));
  raw.c_cflag |= static_cast<tcflag_t>(CS8);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
    std::perror("tcsetattr");
    return;
  }
  raw_ = true;

  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  if (flags >= 0) {
    (void)fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
  }

  color_ = !env_truthy_no_color();
  const char* term = std::getenv("TERM");
  if (term != nullptr && std::strcmp(term, "dumb") == 0) {
    color_ = false;
  }

  ok_ = true;
}

Terminal::~Terminal() {
  if (g_active == this) {
    g_active = nullptr;
  }
  if (!ok_) {
    return;
  }
  if (alt_screen_) {
    leave_alt_screen();
  }
  show_cursor();
  reset_attrs();
  flush();
  if (raw_) {
    (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
    raw_ = false;
  }
  // Keep g_have_original so atexit is idempotent / safe.
}

TermSize Terminal::size() const {
  winsize ws {};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0) {
    return {24, 80};
  }
  return {static_cast<int>(ws.ws_row), static_cast<int>(ws.ws_col)};
}

void Terminal::write(std::string_view s) {
  if (s.empty()) {
    return;
  }
  (void)!::write(STDOUT_FILENO, s.data(), s.size());
}

void Terminal::flush() {
  (void)std::fflush(stdout);
}

void Terminal::present(std::string_view frame) {
  // DECSET 2026 synchronized update (ignored by terminals that don't support it).
  write("\x1b[?2026h");
  write(frame);
  write("\x1b[?2026l");
  flush();
}

bool Terminal::read_byte(unsigned char& ch) {
  const ssize_t n = ::read(STDIN_FILENO, &ch, 1);
  if (n == 1) {
    return true;
  }
  return false;
}

void Terminal::clear_screen() { write("\x1b[2J\x1b[H"); }

void Terminal::move_cursor(int row, int col) {
  char buf[32];
  const int n = std::snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
  if (n > 0) {
    write(std::string_view(buf, static_cast<std::size_t>(n)));
  }
}

void Terminal::hide_cursor() { write("\x1b[?25l"); }
void Terminal::show_cursor() { write("\x1b[?25h"); }

void Terminal::set_fg(int ansi_color) {
  if (!color_) {
    return;
  }
  char buf[16];
  int n = 0;
  if (ansi_color >= 8 && ansi_color <= 15) {
    n = std::snprintf(buf, sizeof(buf), "\x1b[%dm", 90 + (ansi_color - 8));
  } else {
    const int c = ansi_color < 0 ? 0 : (ansi_color > 7 ? 7 : ansi_color);
    n = std::snprintf(buf, sizeof(buf), "\x1b[%dm", 30 + c);
  }
  if (n > 0) {
    write(std::string_view(buf, static_cast<std::size_t>(n)));
  }
}

void Terminal::set_bg(int ansi_color) {
  if (!color_) {
    return;
  }
  const int c = ansi_color < 0 ? 0 : (ansi_color > 7 ? 7 : ansi_color);
  char buf[16];
  const int n = std::snprintf(buf, sizeof(buf), "\x1b[%dm", 40 + c);
  if (n > 0) {
    write(std::string_view(buf, static_cast<std::size_t>(n)));
  }
}

void Terminal::set_bold(bool on) {
  if (!color_) {
    return;
  }
  write(on ? "\x1b[1m" : "\x1b[22m");
}

void Terminal::set_dim(bool on) {
  if (!color_) {
    return;
  }
  write(on ? "\x1b[2m" : "\x1b[22m");
}

void Terminal::reset_attrs() { write("\x1b[0m"); }

void Terminal::enter_alt_screen() {
  write("\x1b[?1049h");
  alt_screen_ = true;
  g_alt_screen = true;
}

void Terminal::leave_alt_screen() {
  write("\x1b[?1049l");
  alt_screen_ = false;
  g_alt_screen = false;
}

}  // namespace tp
