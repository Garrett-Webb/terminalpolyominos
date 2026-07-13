#include "app/App.hpp"

#include <chrono>
#include <random>
#include <thread>

namespace tp {
namespace {

GameConfig gameplay_config(const Settings& settings, const Terminal& term) {
  GameConfig cfg = settings.game;
  // Hard-drop flash is white bg only; skip entirely without color (no '#' spam).
  if (!term.color_enabled()) {
    cfg.hard_drop_flash_ms = 0;
  }
  return cfg;
}

}  // namespace

App::App(Terminal& term)
    : term_(term),
      settings_(Settings::load_or_create()),
      input_(settings_.input),
      renderer_(term),
      game_(gameplay_config(settings_, term)) {}

std::uint64_t App::fresh_seed() const {
  std::random_device rd;
  const auto now = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  return now ^ (static_cast<std::uint64_t>(rd()) << 1);
}

bool App::terminal_ok_size() const {
  return Renderer::fits_min(term_.size());
}

void App::apply_settings(const Settings& s) {
  settings_ = s;
  input_.set_config(settings_.input);
  input_.reset();
  game_.set_config(gameplay_config(settings_, term_));
}

void App::open_settings() {
  settings_return_ = screen_;
  if (screen_ == Screen::Playing && game_.state().phase == Phase::Playing) {
    game_.apply(Action::Pause);
  }
  menu_.open(settings_);
  screen_ = Screen::Settings;
  input_.reset();
  renderer_.invalidate();
  ui_dirty_ = true;
}

void App::close_settings() {
  screen_ = settings_return_;
  input_.reset();
  renderer_.invalidate();
  ui_dirty_ = true;
}

void App::handle_settings_result() {
  switch (menu_.take_result()) {
    case SettingsMenu::Result::None:
      break;
    case SettingsMenu::Result::Saved:
      apply_settings(menu_.draft());
      (void)settings_.save();
      ui_dirty_ = true;
      break;
    case SettingsMenu::Result::Back:
      close_settings();
      break;
  }
}

void App::start_game() {
  input_.reset();
  game_.reset(fresh_seed());
  screen_ = Screen::Playing;
  renderer_.invalidate();
  ui_dirty_ = true;
}

void App::handle_action(Action action) {
  if (action == Action::Settings) {
    if (screen_ == Screen::Title || screen_ == Screen::Playing) {
      open_settings();
    }
    return;
  }

  if (action == Action::Quit) {
    if (screen_ == Screen::Title) {
      quit_ = true;
      return;
    }
    if (game_.state().phase == Phase::Paused || game_.state().phase == Phase::GameOver) {
      quit_ = true;
      return;
    }
    return;
  }

  if (screen_ == Screen::Title) {
    if (action == Action::Restart) {
      start_game();
    }
    return;
  }

  if (game_.state().phase == Phase::GameOver) {
    if (action == Action::Restart) {
      start_game();
    }
    return;
  }

  if (game_.state().phase == Phase::Paused) {
    if (action == Action::Pause) {
      game_.apply(Action::Pause);
      input_.reset();
      ui_dirty_ = true;
    }
    return;
  }

  game_.apply(action);
}

void App::pump_keys() {
  unsigned char byte = 0;
  while (term_.read_byte(byte)) {
    keys_.feed(byte);
  }

  KeyEvent ev;
  while (keys_.poll(ev)) {
    if (ev.key == Key::CtrlC) {
      quit_ = true;
      return;
    }

    if (screen_ == Screen::Settings) {
      menu_.on_key(ev);
      handle_settings_result();
      ui_dirty_ = true;
      if (quit_) {
        return;
      }
      continue;
    }

    if (screen_ == Screen::Title && ev.key == Key::Esc) {
      quit_ = true;
      return;
    }
    input_.on_key(ev);
    for (Action action : input_.drain()) {
      handle_action(action);
      if (quit_) {
        return;
      }
    }
  }
}

int App::run() {
  term_.enter_alt_screen();
  term_.hide_cursor();

  using clock = std::chrono::steady_clock;
  auto prev = clock::now();
  last_size_ = term_.size();

  constexpr int kFrameMs = 33;
  constexpr int kIdleFrameMs = 100;

  while (!quit_) {
    pump_keys();

    const auto now = clock::now();
    int elapsed_ms = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - prev).count());
    prev = now;
    if (elapsed_ms < 0) {
      elapsed_ms = 0;
    }
    if (elapsed_ms > 50) {
      elapsed_ms = 50;
    }

    if (screen_ != Screen::Settings) {
      input_.tick(elapsed_ms);
      for (Action action : input_.drain()) {
        handle_action(action);
        if (quit_) {
          break;
        }
      }
    }

    const TermSize sz = term_.size();
    if (sz.rows != last_size_.rows || sz.cols != last_size_.cols) {
      last_size_ = sz;
      ui_dirty_ = true;
    }

    const bool size_ok = terminal_ok_size();
    bool need_draw = ui_dirty_;

    if (!size_ok) {
      need_draw = true;
    } else if (screen_ == Screen::Title || screen_ == Screen::Settings) {
      // Static until interaction dirties UI.
    } else {
      if (game_.state().phase == Phase::Playing && elapsed_ms > 0) {
        game_.tick(elapsed_ms);
      }
      if (game_.consume_dirty()) {
        need_draw = true;
      }
    }

    if (need_draw) {
      if (!size_ok) {
        renderer_.draw_too_small();
      } else if (screen_ == Screen::Title) {
        renderer_.draw_title();
      } else if (screen_ == Screen::Settings) {
        renderer_.draw_settings(menu_.view());
      } else {
        renderer_.draw_game(game_.state(), game_.config().freak_colors);
      }
      ui_dirty_ = false;
    }

    const bool idle = screen_ == Screen::Title || screen_ == Screen::Settings || !size_ok ||
                      (screen_ == Screen::Playing && game_.state().phase != Phase::Playing);
    const int target_ms = idle ? kIdleFrameMs : kFrameMs;

    const auto frame_end = clock::now();
    const auto used = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - now);
    if (used.count() < target_ms) {
      std::this_thread::sleep_for(std::chrono::milliseconds(target_ms - used.count()));
    }
  }

  term_.leave_alt_screen();
  term_.show_cursor();
  term_.reset_attrs();
  term_.flush();
  return 0;
}

}  // namespace tp
