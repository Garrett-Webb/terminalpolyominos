#pragma once

#include "app/SettingsMenu.hpp"
#include "game/Game.hpp"
#include "terminal/Terminal.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tp {

inline constexpr int kMinTermRows = 26;
inline constexpr int kMinTermCols = 62;

struct Layout {
  int scale = 1;
  int cell_w = 3;
  int cell_h = 1;
  int field_row = 1;
  int field_col = 1;
  int hold_row = 1;
  int hold_col = 1;
  int next_row = 1;
  int next_col = 1;
  int side_inner_w = 14;
  int field_h = 22;
  int field_w = 32;
  int hint_row = 1;
};

class Renderer {
 public:
  explicit Renderer(Terminal& term);

  void draw_title();
  void draw_too_small();
  void draw_game(const GameState& state, bool freak_colors = true);
  void draw_settings(const SettingsMenuView& menu);

  // Next draw starts from a blank grid (needed when switching screens that
  // do not repaint every cell — e.g. settings → title).
  void invalidate();

  [[nodiscard]] static Layout compute_layout(TermSize sz);
  [[nodiscard]] static bool fits_min(TermSize sz);

 private:
  struct Glyph {
    char ch = ' ';
    std::int8_t fg = -1;
    std::int8_t bg = -1;
    bool bold = false;
    bool dim = false;

    bool operator==(const Glyph& o) const {
      return ch == o.ch && fg == o.fg && bg == o.bg && bold == o.bold && dim == o.dim;
    }
  };

  // Paints into a terminal-sized glyph grid, then presents only diffs.
  class Canvas {
   public:
    void begin(TermSize sz);
    void request_clear();
    void cup(int row, int col);
    void reset();
    void bold();
    void dim();
    void fg(int color);
    void bg(int color);
    void text(std::string_view s);
    void text(char ch);
    void number(int value);
    void present(Terminal& term);

    // True after a present that wrote nothing (callers may skip work next time).
    [[nodiscard]] bool last_present_was_empty() const { return last_empty_; }

   private:
    void put(char ch);
    void emit_sgr(std::string& out, const Glyph& g) const;

    int rows_ = 0;
    int cols_ = 0;
    int row_ = 1;
    int col_ = 1;
    Glyph pen_{};
    std::vector<Glyph> cur_;
    std::vector<Glyph> prev_;
    std::string out_;
    bool force_full_ = true;
    bool clear_next_ = false;
    bool last_empty_ = false;
  };

  void cell(Canvas& f, const Layout& lay, int row, int col, bool filled, int color, bool ghost,
            bool flash = false);
  // Draws a piece centered in a panel of width panel_w (columns) and height 4 cells.
  void piece_preview(Canvas& f, const Layout& lay, int row, int col, int panel_w,
                     const PieceSpec& spec, bool freak_colors);
  void hline(Canvas& f, int row, int col, int inner_width, char edge, char fill);
  void draw_box_title(Canvas& f, int row, int col, int inner_w, std::string_view title);

  Terminal& term_;
  Canvas canvas_;
};

}  // namespace tp
