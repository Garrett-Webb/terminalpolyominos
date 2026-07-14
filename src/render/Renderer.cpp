#include "render/Renderer.hpp"

#include "game/Piece.hpp"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <initializer_list>
#include <string>
#include <string_view>

namespace tp {
namespace {

constexpr int kHintRows = 2;
constexpr int kTopPad = 0;
constexpr int kGap = 2;
constexpr int kBlockAspect = 3;

int cell_width_for(int scale) { return kBlockAspect * scale; }
int cell_height_for(int scale) { return scale; }

int side_inner_width(int cell_w) { return std::max(4 * cell_w, 14); }

bool layout_fits(TermSize sz, int scale) {
  const int cell_w = cell_width_for(scale);
  const int cell_h = cell_height_for(scale);
  const int side_inner = side_inner_width(cell_w);
  const int side = side_inner + 2;
  const int field_w = kBoardWidth * cell_w + 2;
  const int field_h = kVisibleRows * cell_h + 2;
  const int need_cols = side + kGap + field_w + kGap + side;
  const int need_rows = kTopPad + field_h + kHintRows;
  return sz.cols >= need_cols && sz.rows >= need_rows;
}

}  // namespace

void Renderer::Canvas::begin(TermSize sz) {
  const int rows = std::max(1, sz.rows);
  const int cols = std::max(1, sz.cols);
  if (rows != rows_ || cols != cols_) {
    rows_ = rows;
    cols_ = cols;
    cur_.assign(static_cast<std::size_t>(rows_ * cols_), Glyph{});
    prev_.assign(static_cast<std::size_t>(rows_ * cols_), Glyph{});
    force_full_ = true;
    clear_next_ = false;
  } else if (clear_next_) {
    // Blank slate so leftover glyphs from another screen are erased.
    cur_.assign(static_cast<std::size_t>(rows_ * cols_), Glyph{});
    force_full_ = true;
    clear_next_ = false;
  } else {
    // Keep last frame as the base; paint overwrites. Avoids wiping the whole
    // terminal grid every tick (expensive at large sizes).
    cur_ = prev_;
  }
  row_ = 1;
  col_ = 1;
  pen_ = Glyph{};
}

void Renderer::Canvas::request_clear() { clear_next_ = true; }

void Renderer::Canvas::cup(int row, int col) {
  row_ = row;
  col_ = col;
}

void Renderer::Canvas::reset() { pen_ = Glyph{}; }
void Renderer::Canvas::bold() { pen_.bold = true; }
void Renderer::Canvas::dim() { pen_.dim = true; }

void Renderer::Canvas::fg(int color) {
  if (color < 0) {
    pen_.fg = -1;
  } else if (color > 15) {
    pen_.fg = 7;
  } else {
    pen_.fg = static_cast<std::int8_t>(color);
  }
}

void Renderer::Canvas::bg(int color) {
  if (color < 0) {
    pen_.bg = -1;
  } else if (color > 15) {
    pen_.bg = 7;
  } else {
    pen_.bg = static_cast<std::int8_t>(color);
  }
}

void Renderer::Canvas::put(char ch) {
  if (row_ < 1 || col_ < 1 || row_ > rows_ || col_ > cols_) {
    ++col_;
    return;
  }
  Glyph g = pen_;
  g.ch = ch;
  cur_[static_cast<std::size_t>((row_ - 1) * cols_ + (col_ - 1))] = g;
  ++col_;
}

void Renderer::Canvas::text(std::string_view s) {
  for (char ch : s) {
    put(ch);
  }
}

void Renderer::Canvas::text(char ch) { put(ch); }

void Renderer::Canvas::number(int value) {
  char tmp[32];
  const int n = std::snprintf(tmp, sizeof(tmp), "%d", value);
  if (n > 0) {
    text(std::string_view(tmp, static_cast<std::size_t>(n)));
  }
}

void Renderer::Canvas::emit_sgr(std::string& out, const Glyph& g) const {
  out.append("\x1b[0m");
  if (g.bold) {
    out.append("\x1b[1m");
  }
  if (g.dim) {
    out.append("\x1b[2m");
  }
  if (g.fg >= 0) {
    char tmp[16];
    int n = 0;
    if (g.fg >= 8) {
      n = std::snprintf(tmp, sizeof(tmp), "\x1b[%dm", 90 + (g.fg - 8));
    } else {
      n = std::snprintf(tmp, sizeof(tmp), "\x1b[%dm", 30 + g.fg);
    }
    if (n > 0) {
      out.append(tmp, static_cast<std::size_t>(n));
    }
  }
  if (g.bg >= 0) {
    char tmp[16];
    int n = 0;
    if (g.bg >= 8) {
      n = std::snprintf(tmp, sizeof(tmp), "\x1b[%dm", 100 + (g.bg - 8));
    } else {
      n = std::snprintf(tmp, sizeof(tmp), "\x1b[%dm", 40 + g.bg);
    }
    if (n > 0) {
      out.append(tmp, static_cast<std::size_t>(n));
    }
  }
}

void Renderer::Canvas::present(Terminal& term) {
  out_.clear();
  out_.reserve(static_cast<std::size_t>(rows_ * cols_ / 8 + 64));
  out_.append("\x1b[?25l");

  if (force_full_) {
    out_.append("\x1b[H\x1b[J");
    Glyph last{};
    bool have_last = false;
    for (int r = 1; r <= rows_; ++r) {
      // Trim trailing blanks per row; still emit interior spaces so words stay spaced.
      int last_col = 0;
      for (int c = cols_; c >= 1; --c) {
        const Glyph& g = cur_[static_cast<std::size_t>((r - 1) * cols_ + (c - 1))];
        if (!(g.ch == ' ' && g.fg < 0 && g.bg < 0 && !g.bold && !g.dim)) {
          last_col = c;
          break;
        }
      }
      have_last = false;
      for (int c = 1; c <= last_col; ++c) {
        const Glyph& g = cur_[static_cast<std::size_t>((r - 1) * cols_ + (c - 1))];
        const bool cont =
            have_last && last.fg == g.fg && last.bg == g.bg && last.bold == g.bold &&
            last.dim == g.dim;
        if (!cont) {
          char cup[32];
          const int n = std::snprintf(cup, sizeof(cup), "\x1b[%d;%dH", r, c);
          if (n > 0) {
            out_.append(cup, static_cast<std::size_t>(n));
          }
          emit_sgr(out_, g);
          last = g;
          have_last = true;
        }
        out_.push_back(g.ch);
      }
    }
    out_.append("\x1b[0m");
  } else {
    Glyph last{};
    bool have_pen = false;
    int run_r = 0;
    int run_c = 0;
    bool any = false;

    for (int r = 1; r <= rows_; ++r) {
      for (int c = 1; c <= cols_; ++c) {
        const std::size_t i = static_cast<std::size_t>((r - 1) * cols_ + (c - 1));
        const Glyph& g = cur_[i];
        if (g == prev_[i]) {
          have_pen = false;
          continue;
        }
        any = true;
        const bool cont =
            have_pen && run_r == r && run_c == c && last.fg == g.fg && last.bg == g.bg &&
            last.bold == g.bold && last.dim == g.dim;
        if (!cont) {
          char cup[32];
          const int n = std::snprintf(cup, sizeof(cup), "\x1b[%d;%dH", r, c);
          if (n > 0) {
            out_.append(cup, static_cast<std::size_t>(n));
          }
          emit_sgr(out_, g);
          have_pen = true;
        }
        out_.push_back(g.ch);
        run_r = r;
        run_c = c + 1;
        last = g;
      }
      have_pen = false;
    }
    out_.append("\x1b[0m");

    if (!any) {
      last_empty_ = true;
      force_full_ = false;
      return;  // Nothing changed — skip the write syscall entirely.
    }
  }

  term.present(out_);
  prev_ = cur_;
  force_full_ = false;
  last_empty_ = false;
}

Renderer::Renderer(Terminal& term) : term_(term) {}

void Renderer::invalidate() { canvas_.request_clear(); }

bool Renderer::fits_min(TermSize sz) { return layout_fits(sz, 1); }

Layout Renderer::compute_layout(TermSize sz) {
  Layout lay;
  int scale = 1;
  for (int s = 1; s <= 12; ++s) {
    if (layout_fits(sz, s)) {
      scale = s;
    } else {
      break;
    }
  }

  lay.scale = scale;
  lay.cell_w = cell_width_for(scale);
  lay.cell_h = cell_height_for(scale);
  lay.side_inner_w = side_inner_width(lay.cell_w);
  lay.field_w = kBoardWidth * lay.cell_w + 2;
  lay.field_h = kVisibleRows * lay.cell_h + 2;

  const int side = lay.side_inner_w + 2;
  const int total_w = side + kGap + lay.field_w + kGap + side;
  const int total_h = kTopPad + lay.field_h + kHintRows;

  const int origin_col = std::max(1, (sz.cols - total_w) / 2 + 1);
  const int origin_row = std::max(1, (sz.rows - total_h) / 2 + 1);

  lay.hold_col = origin_col;
  lay.hold_row = origin_row + kTopPad;
  lay.field_col = origin_col + side + kGap;
  lay.field_row = lay.hold_row;
  lay.next_col = lay.field_col + lay.field_w + kGap;
  lay.next_row = lay.field_row;
  lay.hint_row = lay.field_row + lay.field_h + 1;
  return lay;
}

void Renderer::hline(Canvas& f, int row, int col, int inner_width, char edge, char fill) {
  f.cup(row, col);
  f.text(edge);
  for (int i = 0; i < inner_width; ++i) {
    f.text(fill);
  }
  f.text(edge);
}

void Renderer::draw_box_title(Canvas& f, int row, int col, int inner_w, std::string_view title) {
  f.cup(row, col);
  f.text('+');
  const int title_len = static_cast<int>(title.size());
  const int pad = std::max(0, inner_w - title_len);
  const int left = pad / 2;
  const int right = pad - left;
  for (int i = 0; i < left; ++i) {
    f.text('-');
  }
  if (term_.color_enabled()) {
    f.bold();
  }
  f.text(title);
  f.reset();
  for (int i = 0; i < right; ++i) {
    f.text('-');
  }
  f.text('+');
}

void Renderer::cell(Canvas& f, const Layout& lay, int row, int col, bool filled, int color,
                    bool ghost, bool flash) {
  for (int dy = 0; dy < lay.cell_h; ++dy) {
    f.cup(row + dy, col);
    if (flash) {
      if (term_.color_enabled()) {
        f.bg(7);  // white
        for (int i = 0; i < lay.cell_w; ++i) {
          f.text(' ');
        }
        f.reset();
      } else {
        for (int i = 0; i < lay.cell_w; ++i) {
          f.text('#');
        }
      }
      continue;
    }
    if (!filled) {
      for (int i = 0; i < lay.cell_w; ++i) {
        f.text(' ');
      }
      continue;
    }
    if (ghost) {
      if (term_.color_enabled()) {
        f.dim();
        f.fg(color);
      }
      if (lay.cell_w <= 2) {
        f.text(term_.color_enabled() ? "[]" : "()");
      } else {
        f.text('[');
        for (int i = 0; i < lay.cell_w - 2; ++i) {
          f.text(term_.color_enabled() ? '.' : ' ');
        }
        f.text(']');
      }
      f.reset();
      continue;
    }
    if (term_.color_enabled()) {
      f.bg(color);
      for (int i = 0; i < lay.cell_w; ++i) {
        f.text(' ');
      }
      f.reset();
    } else {
      if (lay.cell_w <= 2) {
        f.text("[]");
      } else {
        f.text('[');
        for (int i = 0; i < lay.cell_w - 2; ++i) {
          f.text('#');
        }
        f.text(']');
      }
    }
  }
}

void Renderer::piece_preview(Canvas& f, const Layout& lay, int row, int col, int panel_w,
                             const PieceSpec& spec, bool freak_colors) {
  Offset cells[kMaxPieceCells];
  int n = 0;
  piece_cells(spec, 0, cells, n);
  if (n <= 0) {
    return;
  }
  int min_x = cells[0].x, max_x = cells[0].x, min_y = cells[0].y, max_y = cells[0].y;
  for (int i = 1; i < n; ++i) {
    min_x = std::min(min_x, cells[i].x);
    max_x = std::max(max_x, cells[i].x);
    min_y = std::min(min_y, cells[i].y);
    max_y = std::max(max_y, cells[i].y);
  }

  bool grid[4][4]{};
  for (int i = 0; i < n; ++i) {
    const int gy = cells[i].y - min_y;
    const int gx = cells[i].x - min_x;
    if (gy >= 0 && gy < 4 && gx >= 0 && gx < 4) {
      grid[gy][gx] = true;
    }
  }
  const int h = max_y - min_y + 1;
  const int w = max_x - min_x + 1;

  const int slot_h = 4 * lay.cell_h;
  const int piece_w = w * lay.cell_w;
  const int piece_h = h * lay.cell_h;
  const int offset_x = std::max(0, (panel_w - piece_w) / 2);
  const int offset_y = std::max(0, (slot_h - piece_h) / 2);

  // Clear the slot so previous (differently shaped) pieces don't leave crumbs.
  for (int y = 0; y < slot_h; ++y) {
    f.cup(row + y, col);
    for (int x = 0; x < panel_w; ++x) {
      f.text(' ');
    }
  }

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if (!grid[y][x]) {
        continue;
      }
      cell(f, lay, row + offset_y + y * lay.cell_h, col + offset_x + x * lay.cell_w, true,
           piece_color(spec, freak_colors), false);
    }
  }
}

void Renderer::draw_title(const HighScores& scores, Randomizer current) {
  const TermSize sz = term_.size();
  canvas_.begin(sz);

  constexpr int kContentMax = 48;
  // logo(6), wordmark, blank, tagline, blank, actions, blank, score hdr + 3, blank, CTA
  constexpr int kBlockH = 18;
  const int content_w = std::min(kContentMax, std::max(1, sz.cols));
  const int col = sz.cols > content_w ? (sz.cols - content_w) / 2 + 1 : 1;
  int y = sz.rows > kBlockH + 2 ? (sz.rows - kBlockH) / 2 + 1 : 1;

  auto pad_line = [&](int r, std::string_view s) {
    canvas_.cup(r, col);
    if (static_cast<int>(s.size()) > content_w) {
      canvas_.text(s.substr(0, static_cast<std::size_t>(content_w)));
      return;
    }
    canvas_.text(s);
    for (int i = static_cast<int>(s.size()); i < content_w; ++i) {
      canvas_.text(' ');
    }
  };

  static constexpr std::string_view kLogo[] = {
      " _               _",
      "| |_ _ __   ___ | |_   _",
      "| __| '_ \\ / _ \\| | | | |",
      "| |_| |_) | (_) | | |_| |",
      " \\__| .__/ \\___/|_|\\__, |",
      "    |_|            |___/",
  };

  canvas_.reset();
  if (term_.color_enabled()) {
    canvas_.bold();
    canvas_.fg(6);
  }
  for (std::string_view row : kLogo) {
    pad_line(y++, row);
  }
  pad_line(y++, "terminalpolyominos");
  canvas_.reset();

  pad_line(y++, "");
  pad_line(y++, "Lightweight full-featured stacking for terminals");

  pad_line(y++, "");
  pad_line(y++, "Enter / r play   o settings   h scores");

  pad_line(y++, "");
  const auto& board = scores.board(current);
  {
    std::string hdr = "Top ";
    hdr += HighScores::token(current);
    if (term_.color_enabled()) {
      canvas_.dim();
    }
    pad_line(y++, hdr);
    canvas_.reset();
  }
  if (board.empty()) {
    if (term_.color_enabled()) {
      canvas_.dim();
    }
    pad_line(y++, "  (none)");
    canvas_.reset();
    pad_line(y++, "");
    pad_line(y++, "");
  } else {
    const int n = std::min(3, static_cast<int>(board.size()));
    for (int i = 0; i < 3; ++i) {
      if (i < n) {
        const auto& e = board[static_cast<std::size_t>(i)];
        char line[48];
        std::snprintf(line, sizeof(line), " %d. %-8s  %d", i + 1, e.name.c_str(), e.score);
        if (term_.color_enabled()) {
          canvas_.dim();
        }
        pad_line(y++, line);
        canvas_.reset();
      } else {
        pad_line(y++, "");
      }
    }
  }

  pad_line(y++, "");
  if (term_.color_enabled()) {
    canvas_.fg(3);
  }
  pad_line(y++, "Press Enter to play");
  canvas_.reset();

  canvas_.present(term_);
}

void Renderer::draw_scores(const HighScores& scores, Randomizer viewing) {
  const TermSize sz = term_.size();
  canvas_.begin(sz);

  const int width = std::min(58, std::max(48, sz.cols - 4));
  const int col = std::max(1, (sz.cols - width) / 2 + 1);
  int row = 2;

  // ASCII only — multi-byte glyphs (e.g. em dash) occupy several canvas cells but one
  // terminal column, which skews overwrite when cycling board names.
  auto pad_line = [&](int r, std::string_view s) {
    canvas_.cup(r, col);
    if (static_cast<int>(s.size()) >= width) {
      canvas_.text(s.substr(0, static_cast<std::size_t>(width)));
      return;
    }
    canvas_.text(s);
    for (int i = static_cast<int>(s.size()); i < width; ++i) {
      canvas_.text(' ');
    }
  };

  auto format_date = [](std::int64_t unix_time, char* out, std::size_t out_n) {
    if (out_n < 11) {
      return;
    }
    const std::time_t t = static_cast<std::time_t>(unix_time);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    if (localtime_r(&t, &tm_buf) == nullptr) {
      std::snprintf(out, out_n, "----/--/--");
      return;
    }
#endif
    if (std::strftime(out, out_n, "%Y-%m-%d", &tm_buf) == 0) {
      std::snprintf(out, out_n, "----/--/--");
    }
  };

  // Wipe first with default attrs so shorter titles never leave bold crumbs.
  canvas_.reset();
  pad_line(row, "");
  if (term_.color_enabled()) {
    canvas_.bold();
    canvas_.fg(6);
  }
  {
    // Fixed token field (longest token is "torture") so every board paints the same span.
    char title[48];
    std::snprintf(title, sizeof(title), "HIGH SCORES - %-7s", HighScores::token(viewing));
    pad_line(row, title);
  }
  canvas_.reset();
  ++row;

  canvas_.reset();
  pad_line(row, "");
  if (term_.color_enabled()) {
    canvas_.dim();
  }
  pad_line(row, "Left/Right cycle board     Esc / q back");
  canvas_.reset();
  row += 2;

  canvas_.reset();
  pad_line(row, "");
  if (term_.color_enabled()) {
    canvas_.dim();
  }
  pad_line(row, " #  Name      Score  Lv  Lines  Date");
  canvas_.reset();
  ++row;

  const auto& board = scores.board(viewing);
  for (int i = 0; i < kHighScoreBoardMax; ++i) {
    canvas_.reset();
    if (i < static_cast<int>(board.size())) {
      const auto& e = board[static_cast<std::size_t>(i)];
      char date[16];
      format_date(e.unix_time, date, sizeof(date));
      char line[72];
      std::snprintf(line, sizeof(line), "%2d  %-8s %6d %3d %6d  %s", i + 1, e.name.c_str(),
                    e.score, e.level, e.lines, date);
      pad_line(row + i, line);
    } else if (i == 0 && board.empty()) {
      pad_line(row + i, "  (empty)");
    } else {
      pad_line(row + i, "");
    }
  }

  canvas_.present(term_);
}

void Renderer::draw_too_small() {
  const TermSize sz = term_.size();
  canvas_.begin(sz);
  canvas_.cup(1, 1);
  canvas_.text("Terminal too small for terminalpolyominos.");
  canvas_.cup(2, 1);
  canvas_.text("Need at least ");
  canvas_.number(kMinTermCols);
  canvas_.text("x");
  canvas_.number(kMinTermRows);
  canvas_.text(". Resize or q to quit.");
  canvas_.present(term_);
}

void Renderer::draw_game(const GameState& state, bool freak_colors,
                         const GameOverExtras* game_over) {
  const TermSize sz = term_.size();
  const Layout lay = compute_layout(sz);
  canvas_.begin(sz);

  const int inner_w = kBoardWidth * lay.cell_w;
  const int field_inner_h = kVisibleRows * lay.cell_h;
  const int field_bottom = lay.field_row + field_inner_h + 1;
  const int side_inner_h = field_inner_h;

  draw_box_title(canvas_, lay.hold_row, lay.hold_col, lay.side_inner_w, "HOLD");
  for (int y = 0; y < side_inner_h; ++y) {
    canvas_.cup(lay.hold_row + 1 + y, lay.hold_col);
    canvas_.text('|');
    canvas_.cup(lay.hold_row + 1 + y, lay.hold_col + 1 + lay.side_inner_w);
    canvas_.text('|');
  }
  hline(canvas_, field_bottom, lay.hold_col, lay.side_inner_w, '+', '-');

  const int hold_content_col = lay.hold_col + 1;
  if (state.hold.has_value()) {
    piece_preview(canvas_, lay, lay.hold_row + 2, hold_content_col, lay.side_inner_w, *state.hold,
                  freak_colors);
  } else {
    const int slot_h = 4 * lay.cell_h;
    for (int y = 0; y < slot_h; ++y) {
      canvas_.cup(lay.hold_row + 2 + y, hold_content_col);
      for (int x = 0; x < lay.side_inner_w; ++x) {
        canvas_.text(' ');
      }
    }
  }

  const int stats_row = field_bottom - 4;
  auto draw_stat = [&](int row, const char* label, int value) {
    canvas_.cup(row, hold_content_col);
    canvas_.text(label);
    canvas_.number(value);
    // Erase leftover digits when the number gets shorter.
    for (int i = 0; i < 8; ++i) {
      canvas_.text(' ');
    }
  };
  draw_stat(stats_row, "Score ", state.score);
  draw_stat(stats_row + 1, "Level ", state.level);
  draw_stat(stats_row + 2, "Lines ", state.lines);
  canvas_.cup(stats_row + 3, hold_content_col);
  if (state.combo > 0) {
    canvas_.text("Combo ");
    canvas_.number(state.combo);
  } else {
    canvas_.text("Combo -");
  }
  for (int i = 0; i < 4; ++i) {
    canvas_.text(' ');
  }
  if (state.b2b_ready) {
    if (term_.color_enabled()) {
      canvas_.bold();
      canvas_.fg(3);
    }
    canvas_.text("B2B");
    canvas_.reset();
  } else {
    canvas_.text("   ");
  }
  for (int i = 0; i < 4; ++i) {
    canvas_.text(' ');
  }

  hline(canvas_, lay.field_row, lay.field_col, inner_w, '+', '-');

  bool ghost_mask[kBoardHeight][kBoardWidth]{};
  bool active_mask[kBoardHeight][kBoardWidth]{};
  bool lock_flash_mask[kBoardHeight][kBoardWidth]{};
  int active_color = 7;
  int ghost_color = 7;

  if (state.lock_flash_ms > 0 && state.lock_flash.alive) {
    Offset cells[kMaxPieceCells];
    int n = 0;
    piece_cells(state.lock_flash.spec, state.lock_flash.rotation, cells, n);
    for (int i = 0; i < n; ++i) {
      const int x = state.lock_flash.x + cells[i].x;
      const int y = state.lock_flash.y + cells[i].y;
      if (x >= 0 && x < kBoardWidth && y >= 0 && y < kBoardHeight) {
        lock_flash_mask[y][x] = true;
      }
    }
  }

  if (state.ghost.alive) {
    Offset cells[kMaxPieceCells];
    int n = 0;
    piece_cells(state.ghost.spec, state.ghost.rotation, cells, n);
    ghost_color = piece_color(state.ghost.spec, freak_colors);
    for (int i = 0; i < n; ++i) {
      const int x = state.ghost.x + cells[i].x;
      const int y = state.ghost.y + cells[i].y;
      if (x >= 0 && x < kBoardWidth && y >= 0 && y < kBoardHeight) {
        ghost_mask[y][x] = true;
      }
    }
  }
  if (state.active.alive) {
    Offset cells[kMaxPieceCells];
    int n = 0;
    piece_cells(state.active.spec, state.active.rotation, cells, n);
    active_color = piece_color(state.active.spec, freak_colors);
    for (int i = 0; i < n; ++i) {
      const int x = state.active.x + cells[i].x;
      const int y = state.active.y + cells[i].y;
      if (x >= 0 && x < kBoardWidth && y >= 0 && y < kBoardHeight) {
        active_mask[y][x] = true;
      }
    }
  }

  for (int vr = 0; vr < kVisibleRows; ++vr) {
    const int by = vr + kBufferRows;
    const int row = lay.field_row + 1 + vr * lay.cell_h;

    for (int dy = 0; dy < lay.cell_h; ++dy) {
      canvas_.cup(row + dy, lay.field_col);
      canvas_.text('|');
      canvas_.cup(row + dy, lay.field_col + 1 + inner_w);
      canvas_.text('|');
    }

    for (int x = 0; x < kBoardWidth; ++x) {
      const int col = lay.field_col + 1 + x * lay.cell_w;
      if (state.clear_flash_ms > 0 && state.clear_rows[static_cast<std::size_t>(by)]) {
        cell(canvas_, lay, row, col, true, 7, false, true);
      } else if (lock_flash_mask[by][x]) {
        // White flash only — caller disables anim when color is off.
        cell(canvas_, lay, row, col, true, 7, false, true);
      } else if (active_mask[by][x]) {
        cell(canvas_, lay, row, col, true, active_color, false);
      } else if (state.board.at(x, by).filled) {
        cell(canvas_, lay, row, col, true, state.board.at(x, by).color, false);
      } else if (ghost_mask[by][x]) {
        cell(canvas_, lay, row, col, true, ghost_color, true);
      } else {
        cell(canvas_, lay, row, col, false, 7, false);
      }
    }
  }

  hline(canvas_, field_bottom, lay.field_col, inner_w, '+', '-');

  draw_box_title(canvas_, lay.next_row, lay.next_col, lay.side_inner_w, "NEXT");
  for (int y = 0; y < side_inner_h; ++y) {
    canvas_.cup(lay.next_row + 1 + y, lay.next_col);
    canvas_.text('|');
    canvas_.cup(lay.next_row + 1 + y, lay.next_col + 1 + lay.side_inner_w);
    canvas_.text('|');
  }
  hline(canvas_, field_bottom, lay.next_col, lay.side_inner_w, '+', '-');

  const int next_content_col = lay.next_col + 1;
  // Pack slots with no gap row so 5 previews fit in the 20-cell panel at scale 1.
  const int preview_stride = 4 * lay.cell_h;
  const int shown = std::max(1, std::min(state.next_count, kNextQueueMax));
  for (int i = 0; i < shown; ++i) {
    piece_preview(canvas_, lay, lay.next_row + 1 + i * preview_stride, next_content_col,
                  lay.side_inner_w, state.next[static_cast<std::size_t>(i)], freak_colors);
  }
  // Erase unused lower slots when the queue is shorter than max.
  const int used_h = shown * preview_stride;
  for (int y = used_h; y < side_inner_h; ++y) {
    canvas_.cup(lay.next_row + 1 + y, next_content_col);
    for (int x = 0; x < lay.side_inner_w; ++x) {
      canvas_.text(' ');
    }
  }

  const int overlay_row = lay.field_row + std::max(1, field_inner_h / 2 - 5);
  const int overlay_col = lay.field_col + 1;
  const int overlay_w = inner_w;

  auto clear_row = [&](int r) {
    canvas_.cup(r, overlay_col);
    for (int i = 0; i < overlay_w; ++i) {
      canvas_.text(' ');
    }
  };

  auto center_text = [&](int r, std::string_view text, int fg = -1, bool bold = false) {
    clear_row(r);
    const int left = std::max(0, (overlay_w - static_cast<int>(text.size())) / 2);
    canvas_.cup(r, overlay_col + left);
    if (fg >= 0 && term_.color_enabled()) {
      if (bold) {
        canvas_.bold();
      }
      canvas_.fg(fg);
    }
    canvas_.text(text);
    canvas_.reset();
  };

  if (state.phase == Phase::Paused) {
    center_text(overlay_row, "PAUSED", 3, true);
    center_text(overlay_row + 1, "p/esc resume  o settings  q quit");
  } else if (state.phase == Phase::GameOver) {
    int r = overlay_row;
    center_text(r++, "GAME OVER", 1, true);
    clear_row(r++);
    center_text(r++, "Score  " + std::to_string(state.score));
    center_text(r++, "Level  " + std::to_string(state.level));
    center_text(r++, "Lines  " + std::to_string(state.lines));
    center_text(r++, "Combo  " + std::to_string(state.combo) +
                         (state.b2b_ready ? "   B2B ready" : ""));
    clear_row(r++);

    auto draw_counts = [&](int row, std::initializer_list<PieceType> types) {
      clear_row(row);
      std::string text;
      for (PieceType t : types) {
        if (!text.empty()) {
          text += "  ";
        }
        text += piece_name(t);
        text += ':';
        text += std::to_string(state.pieces_placed[static_cast<std::size_t>(t)]);
      }
      const int left = std::max(0, (overlay_w - static_cast<int>(text.size())) / 2);
      int col = overlay_col + left;
      for (PieceType t : types) {
        const std::string name = piece_name(t);
        const std::string num =
            std::to_string(state.pieces_placed[static_cast<std::size_t>(t)]);
        canvas_.cup(row, col);
        if (term_.color_enabled()) {
          canvas_.bold();
          canvas_.fg(piece_color(t));
        }
        canvas_.text(name);
        canvas_.reset();
        col += static_cast<int>(name.size());
        canvas_.cup(row, col);
        canvas_.text(':');
        canvas_.text(num);
        col += 1 + static_cast<int>(num.size()) + 2;
      }
    };

    draw_counts(r++, {PieceType::I, PieceType::O, PieceType::T, PieceType::S});
    draw_counts(r++, {PieceType::Z, PieceType::J, PieceType::L, PieceType::Custom});
    clear_row(r++);
    if (game_over != nullptr && game_over->is_high_score) {
      std::string banner = "NEW HIGH SCORE  #";
      banner += std::to_string(game_over->rank);
      banner += "  (";
      banner += HighScores::token(game_over->board);
      banner += ")";
      center_text(r++, banner, 3, true);
      if (game_over->editing_name) {
        std::string prompt = "Name: ";
        prompt += game_over->name_buf.empty() ? "_" : std::string(game_over->name_buf);
        prompt += "   Enter save  Esc discard";
        center_text(r++, prompt, 2, false);
      } else {
        clear_row(r++);
      }
    }
    center_text(r++, game_over != nullptr && game_over->editing_name
                         ? "(save or discard score to continue)"
                         : "r/Enter retry  q quit");
  }

  constexpr char kHint[] = "z/x rotate  c hold  space sonic  up hard  p pause  o settings  q quit";
  const int hint_len = static_cast<int>(sizeof(kHint) - 1);
  const int hint_col = lay.field_col + std::max(0, (lay.field_w - hint_len) / 2);
  canvas_.cup(lay.hint_row, std::max(1, hint_col));
  if (term_.color_enabled()) {
    canvas_.dim();
  }
  canvas_.text(kHint);
  canvas_.reset();

  canvas_.present(term_);
}

namespace {

const char* settings_label(SettingsItem item) {
  switch (item) {
    case SettingsItem::MoveInterval:
      return "Move interval";
    case SettingsItem::SoftDropInterval:
      return "Soft-drop interval";
    case SettingsItem::ReleaseMs:
      return "Release delay";
    case SettingsItem::LockDelay:
      return "Lock delay";
    case SettingsItem::LinesPerLevel:
      return "Lines per level";
    case SettingsItem::NextCount:
      return "Next queue";
    case SettingsItem::Randomizer:
      return "Randomizer";
    case SettingsItem::FreakColors:
      return "Freak colors";
    case SettingsItem::KeyLeft:
      return "Key left";
    case SettingsItem::KeyRight:
      return "Key right";
    case SettingsItem::KeySoftDrop:
      return "Key soft drop";
    case SettingsItem::KeyHardDrop:
      return "Key hard drop";
    case SettingsItem::KeySonicDrop:
      return "Key sonic drop";
    case SettingsItem::KeyRotateCw:
      return "Key rotate CW";
    case SettingsItem::KeyRotateCcw:
      return "Key rotate CCW";
    case SettingsItem::KeyHold:
      return "Key hold";
    case SettingsItem::KeyPause:
      return "Key pause";
    case SettingsItem::KeyQuit:
      return "Key quit";
    case SettingsItem::KeyRestart:
      return "Key restart";
    case SettingsItem::KeySettings:
      return "Key settings";
    case SettingsItem::ClearScores:
      return "Clear high scores";
    case SettingsItem::Save:
      return "Save";
    case SettingsItem::Reset:
      return "Reset defaults";
    case SettingsItem::Back:
      return "Back";
    case SettingsItem::Count:
      break;
  }
  return "";
}

std::string settings_value(const Settings& s, SettingsItem item) {
  const auto& k = s.input.keys;
  switch (item) {
    case SettingsItem::MoveInterval:
      return std::to_string(s.input.move_interval_ms) + " ms";
    case SettingsItem::SoftDropInterval:
      return std::to_string(s.input.soft_drop_interval_ms) + " ms";
    case SettingsItem::ReleaseMs:
      return std::to_string(s.input.release_ms) + " ms";
    case SettingsItem::LockDelay:
      return std::to_string(s.game.lock_delay_ms) + " ms";
    case SettingsItem::LinesPerLevel:
      return std::to_string(s.game.lines_per_level);
    case SettingsItem::NextCount:
      return std::to_string(s.game.next_count);
    case SettingsItem::Randomizer:
      switch (s.game.randomizer) {
        case Randomizer::SevenPlusOne:
          return "7+1 bag";
        case Randomizer::FullRandom:
          return "full random";
        case Randomizer::Torture:
          return "torture";
        case Randomizer::Funk:
          return "funk";
        case Randomizer::Freak:
          return "freak";
        case Randomizer::SevenBag:
          break;
      }
      return "7-bag";
    case SettingsItem::FreakColors:
      return s.game.freak_colors ? "on" : "off";
    case SettingsItem::KeyLeft:
      return k.format_list(k.left);
    case SettingsItem::KeyRight:
      return k.format_list(k.right);
    case SettingsItem::KeySoftDrop:
      return k.format_list(k.soft_drop);
    case SettingsItem::KeyHardDrop:
      return k.format_list(k.hard_drop);
    case SettingsItem::KeySonicDrop:
      return k.format_list(k.sonic_drop);
    case SettingsItem::KeyRotateCw:
      return k.format_list(k.rotate_cw);
    case SettingsItem::KeyRotateCcw:
      return k.format_list(k.rotate_ccw);
    case SettingsItem::KeyHold:
      return k.format_list(k.hold);
    case SettingsItem::KeyPause:
      return k.format_list(k.pause);
    case SettingsItem::KeyQuit:
      return k.format_list(k.quit);
    case SettingsItem::KeyRestart:
      return k.format_list(k.restart);
    case SettingsItem::KeySettings:
      return k.format_list(k.settings);
    case SettingsItem::ClearScores:
    case SettingsItem::Save:
    case SettingsItem::Reset:
    case SettingsItem::Back:
    case SettingsItem::Count:
      return {};
  }
  return {};
}

}  // namespace

void Renderer::draw_settings(const SettingsMenuView& menu) {
  const TermSize sz = term_.size();
  canvas_.begin(sz);

  const int width = std::min(56, std::max(40, sz.cols - 4));
  const int col = std::max(1, (sz.cols - width) / 2 + 1);
  int row = 1;

  auto pad_text = [&](std::string_view s, int field_w) {
    if (field_w <= 0) {
      return;
    }
    if (static_cast<int>(s.size()) >= field_w) {
      canvas_.text(s.substr(0, static_cast<std::size_t>(field_w)));
      return;
    }
    canvas_.text(s);
    for (int i = static_cast<int>(s.size()); i < field_w; ++i) {
      canvas_.text(' ');
    }
  };

  canvas_.cup(row, col);
  if (term_.color_enabled()) {
    canvas_.bold();
    canvas_.fg(6);
  }
  canvas_.text("SETTINGS");
  canvas_.reset();
  // Always rewrite this slot so clearing dirty erases "(unsaved)".
  canvas_.cup(row, col + 12);
  if (menu.dirty) {
    if (term_.color_enabled()) {
      canvas_.fg(3);
    }
    pad_text("(unsaved)", 9);
    canvas_.reset();
  } else {
    pad_text("", 9);
  }
  ++row;

  canvas_.cup(row, col);
  if (term_.color_enabled()) {
    canvas_.dim();
  }
  canvas_.text("~/.config/tpoly/.tpolyrc");
  canvas_.reset();
  row += 2;

  const int footer_rows = 3;
  const int visible = std::max(8, sz.rows - row - footer_rows);
  int start = menu.scroll;
  start = std::max(0, std::min(start, kSettingsItemCount - 1));
  // Prefer keeping selection in view.
  if (menu.selected < start) {
    start = menu.selected;
  }
  if (menu.selected >= start + visible) {
    start = menu.selected - visible + 1;
  }
  start = std::max(0, std::min(start, std::max(0, kSettingsItemCount - visible)));

  constexpr int kLabelField = 18;
  for (int i = 0; i < visible && start + i < kSettingsItemCount; ++i) {
    const int idx = start + i;
    const auto item = static_cast<SettingsItem>(idx);
    const bool sel = idx == menu.selected;
    const int y = row + i;

    canvas_.cup(y, col);
    if (sel && term_.color_enabled()) {
      canvas_.bold();
      canvas_.fg(3);
    }
    canvas_.text(sel ? '>' : ' ');
    canvas_.text(' ');
    pad_text(settings_label(item), kLabelField);

    if (item == SettingsItem::ClearScores || item == SettingsItem::Save ||
        item == SettingsItem::Reset || item == SettingsItem::Back) {
      // Clear any leftover value text from scrolling past keybind rows.
      const int val_col = col + 2 + kLabelField;
      const int max_v = std::max(8, col + width - val_col);
      canvas_.cup(y, val_col);
      if (sel && menu.confirming_clear && item == SettingsItem::ClearScores) {
        if (term_.color_enabled()) {
          canvas_.fg(1);
        }
        std::string prompt = "yes? ";
        prompt += menu.clear_buf;
        pad_text(prompt, max_v);
      } else {
        pad_text("", max_v);
      }
    } else {
      const std::string val = settings_value(menu.draft, item);
      const int val_col = col + 2 + kLabelField;
      canvas_.cup(y, val_col);
      const int max_v = std::max(8, col + width - val_col);
      if (sel && menu.capturing && item >= SettingsItem::KeyLeft &&
          item <= SettingsItem::KeySettings) {
        if (term_.color_enabled()) {
          canvas_.fg(2);
        }
        pad_text("press key...", max_v);
      } else {
        pad_text(val, max_v);
      }
    }
    canvas_.reset();
  }

  const int foot = sz.rows - 2;
  const int foot_w = width;

  auto pad_foot = [&](int r, std::string_view s) {
    canvas_.reset();
    canvas_.cup(r, col);
    // Full wipe under default attrs first (kills UTF-8 / attr crumbs).
    for (int i = 0; i < foot_w; ++i) {
      canvas_.text(' ');
    }
    canvas_.cup(r, col);
    if (term_.color_enabled()) {
      canvas_.dim();
    }
    if (static_cast<int>(s.size()) >= foot_w) {
      canvas_.text(s.substr(0, static_cast<std::size_t>(foot_w)));
    } else {
      canvas_.text(s);
      for (int i = static_cast<int>(s.size()); i < foot_w; ++i) {
        canvas_.text(' ');
      }
    }
    canvas_.reset();
  };

  // ASCII only — arrow / em-dash glyphs break canvas-vs-terminal columns.
  if (menu.confirming_clear) {
    pad_foot(foot, "Type yes + Enter to wipe ALL boards - Esc cancels");
  } else if (menu.capturing) {
    pad_foot(foot, "Capturing - press a key, Esc cancels");
  } else {
    pad_foot(foot, "Up/Down move  Left/Right adjust  Enter select  Esc back");
  }

  canvas_.cup(foot + 1, col);
  canvas_.reset();
  for (int i = 0; i < foot_w; ++i) {
    canvas_.text(' ');
  }
  canvas_.cup(foot + 1, col);
  if (!menu.status.empty()) {
    if (term_.color_enabled()) {
      canvas_.fg(2);
    }
    pad_text(menu.status, foot_w);
    canvas_.reset();
  }

  canvas_.present(term_);
}

}  // namespace tp
