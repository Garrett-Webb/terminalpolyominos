#include "terminal/Terminal.hpp"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <thread>

#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace tp {
namespace {

Terminal* g_active = nullptr;
termios g_original{};
bool g_have_original = false;
bool g_alt_screen = false;
bool g_keyboard_pushed = false;

void write_raw(const char* seq) {
  (void)!::write(STDOUT_FILENO, seq, std::strlen(seq));
}

void pop_keyboard_mode() {
  if (g_keyboard_pushed) {
    write_raw("\x1b[<u");
    g_keyboard_pushed = false;
  }
}

void restore_tty_now() {
  pop_keyboard_mode();
  if (g_alt_screen) {
    // Leave alt screen + show cursor + reset attrs.
    write_raw("\x1b[?1049l\x1b[?25h\x1b[0m");
    g_alt_screen = false;
  } else {
    write_raw("\x1b[?25h\x1b[0m");
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

bool term_looks_256color(const char* term) {
  if (term == nullptr || term[0] == '\0') {
    return false;
  }
  if (std::strstr(term, "256color") != nullptr) {
    return true;
  }
  // Common TERM values that support indexed 256 without the substring.
  static constexpr const char* kKnown[] = {
      "foot", "foot-extra", "xterm-ghostty", "ghostty", "alacritty", "wezterm",
  };
  for (const char* k : kKnown) {
    if (std::strcmp(term, k) == 0) {
      return true;
    }
  }
  const char* colorterm = std::getenv("COLORTERM");
  if (colorterm != nullptr &&
      (std::strcmp(colorterm, "truecolor") == 0 ||
       std::strcmp(colorterm, "24bit") == 0)) {
    // Truecolor terminals almost always handle 38;5 as well.
    return true;
  }
  return false;
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

  // Deliberately do not set O_NONBLOCK on stdin. Raw mode with VMIN=0/VTIME=0
  // already makes read() return immediately when no input is pending, so the
  // flag is unnecessary for non-blocking key polling. Setting O_NONBLOCK on
  // stdin also marks the shared TTY open-file description non-blocking, which
  // makes stdout return short writes / EAGAIN once the ~1KB macOS tty output
  // buffer fills and corrupted frames if those writes are not retried.

  color_ = !env_truthy_no_color();
  const char* term = std::getenv("TERM");
  if (term != nullptr && std::strcmp(term, "dumb") == 0) {
    color_ = false;
  }
  colors_256_ = color_ && term_looks_256color(term);

  ok_ = true;
}

Terminal::~Terminal() {
  if (g_active == this) {
    g_active = nullptr;
  }
  if (!ok_) {
    return;
  }
  disable_keyboard_protocol();
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
  // Write every byte. Short writes and EINTR/EAGAIN must be retried, otherwise
  // the tail of the frame is silently dropped and the display corrupts -
  // especially with macOS's small (~1KB) tty output buffer vs a full frame.
  const char* data = s.data();
  std::size_t remaining = s.size();

  while (remaining > 0) {
    const ssize_t n = ::write(STDOUT_FILENO, data, remaining);

    if (n > 0) {
      data += n;
      remaining -= static_cast<std::size_t>(n);
      continue;
    }

    if (n < 0 && errno == EINTR) {
      continue;
    }

    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      // stdout is non-blocking; wait for room rather than dropping bytes
      pollfd pfd{};
      pfd.fd = STDOUT_FILENO;
      pfd.events = POLLOUT;
      (void)::poll(&pfd, 1, -1);
      continue;
    }

    // n==0 or unrecoverable error: stop to avoid spinning forever
    break;
  }
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

bool Terminal::detect_keyboard_protocol(int timeout_ms) {
  if (!ok_) {
    return false;
  }
  // Query progressive flags, then primary DA. Supported terminals answer flags
  // before (or along with) DA; unsupported answer only DA.
  write("\x1b[?u");
  write("\x1b[c");
  flush();

  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  std::string buf;
  buf.reserve(64);
  bool saw_flags = false;
  bool saw_da = false;

  auto consume = [&] {
    while (buf.size() >= 3) {
      const auto esc = buf.find("\x1b[");
      if (esc == std::string::npos) {
        buf.clear();
        return;
      }
      if (esc > 0) {
        buf.erase(0, esc);
      }
      // Find final byte.
      std::size_t i = 2;
      while (i < buf.size() && (buf[i] < 0x40 || static_cast<unsigned char>(buf[i]) > 0x7e)) {
        ++i;
      }
      if (i >= buf.size()) {
        return;  // incomplete
      }
      const char final_b = buf[i];
      const std::string_view body(buf.data() + 2, i - 2);
      if (!body.empty() && body[0] == '?' && final_b == 'u') {
        saw_flags = true;
      } else if (!body.empty() && body[0] == '?' && final_b == 'c') {
        saw_da = true;
      }
      buf.erase(0, i + 1);
      if (saw_da) {
        return;
      }
    }
  };

  while (std::chrono::steady_clock::now() < deadline && !saw_da) {
    unsigned char ch = 0;
    if (read_byte(ch)) {
      buf.push_back(static_cast<char>(ch));
      consume();
      continue;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  consume();
  return saw_flags;
}

bool Terminal::enable_keyboard_protocol() {
  if (!ok_) {
    return false;
  }
  disable_keyboard_protocol();

  char push[32];
  const int n = std::snprintf(push, sizeof(push), "\x1b[>%du", kKittyKbFlags);
  if (n > 0) {
    write(std::string_view(push, static_cast<std::size_t>(n)));
  }
  flush();
  g_keyboard_pushed = true;

  // Verify flags 2 and 8 stuck.
  write("\x1b[?u");
  flush();
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
  std::string buf;
  int flags = -1;
  while (std::chrono::steady_clock::now() < deadline && flags < 0) {
    unsigned char ch = 0;
    if (!read_byte(ch)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      continue;
    }
    buf.push_back(static_cast<char>(ch));
    // Expect CSI ? <n> u
    if (buf.size() >= 4 && buf[0] == '\x1b' && buf[1] == '[') {
      if (buf.back() == 'u' && buf[2] == '?') {
        flags = std::atoi(buf.c_str() + 3);
        break;
      }
      if (buf.back() >= 0x40 && buf.back() <= 0x7e && buf.back() != 'u') {
        buf.clear();  // unrelated sequence
      }
    } else if (buf.size() > 32) {
      buf.clear();
    }
  }

  constexpr int kNeed = 2 | 8;
  if (flags < 0 || (flags & kNeed) != kNeed) {
    disable_keyboard_protocol();
    keyboard_enhanced_ = false;
    return false;
  }

  keyboard_enhanced_ = true;
  return true;
}

void Terminal::disable_keyboard_protocol() {
  if (g_keyboard_pushed) {
    write("\x1b[<u");
    flush();
    g_keyboard_pushed = false;
  }
  keyboard_enhanced_ = false;
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

void Terminal::set_fg(int color) {
  if (!color_) {
    return;
  }
  char buf[24];
  int n = 0;
  const int c = color < 0 ? 0 : (color > 255 ? 255 : color);
  if (colors_256_) {
    n = std::snprintf(buf, sizeof(buf), "\x1b[38;5;%dm", c);
  } else if (c >= 8 && c <= 15) {
    n = std::snprintf(buf, sizeof(buf), "\x1b[%dm", 90 + (c - 8));
  } else {
    n = std::snprintf(buf, sizeof(buf), "\x1b[%dm", 30 + (c > 7 ? 7 : c));
  }
  if (n > 0) {
    write(std::string_view(buf, static_cast<std::size_t>(n)));
  }
}

void Terminal::set_bg(int color) {
  if (!color_) {
    return;
  }
  char buf[24];
  int n = 0;
  const int c = color < 0 ? 0 : (color > 255 ? 255 : color);
  if (colors_256_) {
    n = std::snprintf(buf, sizeof(buf), "\x1b[48;5;%dm", c);
  } else if (c >= 8 && c <= 15) {
    n = std::snprintf(buf, sizeof(buf), "\x1b[%dm", 100 + (c - 8));
  } else {
    n = std::snprintf(buf, sizeof(buf), "\x1b[%dm", 40 + (c > 7 ? 7 : c));
  }
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
  disable_keyboard_protocol();
  write("\x1b[?1049l");
  alt_screen_ = false;
  g_alt_screen = false;
}

}  // namespace tp
