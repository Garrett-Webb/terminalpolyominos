#pragma once

#include "app/SettingsMenu.hpp"
#include "battle/OppBoardView.hpp"
#include "game/Game.hpp"
#include "input/Input.hpp"
#include "terminal/Terminal.hpp"
#include "util/HighScores.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tp {

inline constexpr int kMinTermRows = 24;
inline constexpr int kMinTermCols = 68;
inline constexpr int kMinBattleRows = 24;
inline constexpr int kMinBattleCols = 80;

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

struct BattleLayout {
  Layout self{};
  int opp_row = 1;
  int opp_col = 1;
  int opp_cell_w = 1;
  int opp_cell_h = 1;
  int opp_inner_w = 10;
  int opp_inner_h = 20;
  int opp_box_w = 12;
  int meter_row = 1;
  int meter_col = 1;
  int meter_inner_w = 4;
  int meter_box_w = 6;
  int hint_row = 1;
  bool opp_scale1 = false;
};

struct GameOverExtras {
  bool is_high_score = false;
  int rank = 0;
  bool editing_name = false;
  std::string_view name_buf{};
  Randomizer board = Randomizer::SevenBag;
  PlayMode mode = PlayMode::Endless;
};

class Renderer {
 public:
  explicit Renderer(Terminal& term);

  void draw_title(const HighScores& scores, Randomizer current, PlayMode play_mode,
                  const Keybinds& keybinds);
  void draw_scores(const HighScores& scores, Randomizer viewing, const Keybinds& keybinds);
  void draw_too_small(const Keybinds& keybinds);
  void draw_game(const GameState& state, const GameConfig& config, const Keybinds& keybinds,
                 const GameOverExtras* game_over = nullptr);
  void draw_battle(const GameState& self, const GameConfig& config, const OppBoardView& opp,
                   const BattleHud& hud, const Keybinds& keybinds);
  void draw_battle_prompt(std::string_view title, std::string_view line1, std::string_view line2,
                          std::string_view line3, std::string_view token_entry,
                          const Keybinds& keybinds);
  void draw_settings(const SettingsMenuView& menu);

  void invalidate();

  [[nodiscard]] static Layout compute_layout(TermSize sz);
  [[nodiscard]] static bool fits_min(TermSize sz);
  [[nodiscard]] static BattleLayout compute_battle_layout(TermSize sz);
  [[nodiscard]] static bool fits_battle_min(TermSize sz);

 private:
  struct Glyph {
    char ch = ' ';
    std::int16_t fg = -1;
    std::int16_t bg = -1;
    bool bold = false;
    bool dim = false;

    bool operator==(const Glyph& o) const {
      return ch == o.ch && fg == o.fg && bg == o.bg && bold == o.bold && dim == o.dim;
    }
  };

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
            bool flash = false, char mono_fill = '#');
  void piece_preview(Canvas& f, const Layout& lay, int row, int col, int panel_w,
                     const PieceSpec& spec, bool freak_colors);
  void hline(Canvas& f, int row, int col, int inner_width, char edge, char fill);
  void draw_box_title(Canvas& f, int row, int col, int inner_w, std::string_view title);

  Terminal& term_;
  Canvas canvas_;
};

}  // namespace tp
