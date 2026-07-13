#pragma once

#include "app/SettingsMenu.hpp"
#include "game/Game.hpp"
#include "input/Input.hpp"
#include "render/Renderer.hpp"
#include "terminal/Keys.hpp"
#include "terminal/Terminal.hpp"
#include "util/Settings.hpp"

#include <cstdint>

namespace tp {

class App {
 public:
  explicit App(Terminal& term);

  int run();

 private:
  enum class Screen : std::uint8_t { Title, Playing, Settings };

  void pump_keys();
  void handle_action(Action action);
  void handle_settings_result();
  void open_settings();
  void close_settings();
  void apply_settings(const Settings& s);
  void start_game();
  [[nodiscard]] bool terminal_ok_size() const;
  [[nodiscard]] std::uint64_t fresh_seed() const;

  Terminal& term_;
  Settings settings_;
  SettingsMenu menu_;
  KeyDecoder keys_;
  Input input_;
  Renderer renderer_;
  Game game_;
  Screen screen_ = Screen::Title;
  Screen settings_return_ = Screen::Title;
  bool quit_ = false;
  bool ui_dirty_ = true;
  TermSize last_size_{};
};

}  // namespace tp
