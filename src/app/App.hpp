#pragma once

#include "app/SettingsMenu.hpp"
#include "game/Game.hpp"
#include "input/Input.hpp"
#include "render/Renderer.hpp"
#include "terminal/Keys.hpp"
#include "terminal/Terminal.hpp"
#include "util/HighScores.hpp"
#include "util/Settings.hpp"

#include <cstdint>
#include <string>

namespace tp {

class App {
 public:
  explicit App(Terminal& term);

  int run();

 private:
  enum class Screen : std::uint8_t { Title, Playing, Settings, Scores };

  void pump_keys();
  void handle_action(Action action);
  void handle_settings_result();
  void open_settings();
  void close_settings();
  void open_scores();
  void close_scores();
  void apply_settings(const Settings& s);
  void start_game();
  void on_game_over_edge();
  void handle_name_key(const KeyEvent& ev);
  void handle_scores_key(const KeyEvent& ev);
  [[nodiscard]] bool terminal_ok_size() const;
  [[nodiscard]] std::uint64_t fresh_seed() const;

  Terminal& term_;
  Settings settings_;
  SettingsMenu menu_;
  HighScores scores_;
  KeyDecoder keys_;
  Input input_;
  Renderer renderer_;
  Game game_;
  Screen screen_ = Screen::Title;
  Screen settings_return_ = Screen::Title;
  bool quit_ = false;
  bool ui_dirty_ = true;
  TermSize last_size_{};

  Phase last_phase_ = Phase::Ready;
  bool pending_name_edit_ = false;
  int pending_rank_ = 0;
  Randomizer pending_board_ = Randomizer::SevenBag;
  std::string name_buf_{};
  Randomizer scores_view_ = Randomizer::SevenBag;
};

}  // namespace tp
