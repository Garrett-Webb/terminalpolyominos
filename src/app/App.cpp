#include "app/App.hpp"

#include "battle/LocalMediatorTransport.hpp"

#if defined(TP_BATTLE_INSECURE)
#include "net/InsecureMediatorTransport.hpp"
#endif

#include <chrono>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <random>
#include <string>
#include <thread>
#include <utility>

namespace tp {
namespace {

GameConfig gameplay_config(const Settings& settings, const Terminal& term) {
  GameConfig cfg = settings.game;
  cfg.colors_256 = term.colors_256();
  if (!term.color_enabled()) {
    cfg.piece_lock_flash_ms = 0;
  }
  return cfg;
}

}  // namespace

App::App(Terminal& term)
    : term_(term),
      settings_(Settings::load_or_create()),
      input_(settings_.input),
      renderer_(term),
      game_(gameplay_config(settings_, term)) {
  (void)scores_.load();
  scores_view_ = settings_.game.randomizer;
}

std::uint64_t App::fresh_seed() const {
  std::random_device rd;
  const auto now = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  return now ^ (static_cast<std::uint64_t>(rd()) << 1);
}

bool App::terminal_ok_size() const {
  if (screen_ == Screen::Battle) {
    return Renderer::fits_battle_min(term_.size());
  }
  return Renderer::fits_min(term_.size());
}

void App::apply_settings(const Settings& s) {
  settings_ = s;
  input_.set_config(settings_.input);
  input_.reset();
  game_.set_config(gameplay_config(settings_, term_));
  setup_keyboard();
}

void App::setup_keyboard() {
  const KeyboardProtocol pref = settings_.input.keyboard_protocol;
  if (pref == KeyboardProtocol::Legacy) {
    term_.disable_keyboard_protocol();
    input_.set_key_up_aware(false);
    return;
  }

  // Prefer enhanced when Auto or Kitty; fall back silently if unsupported.
  if (term_.detect_keyboard_protocol(/*timeout_ms=*/80) && term_.enable_keyboard_protocol()) {
    input_.set_key_up_aware(true);
    return;
  }
  term_.disable_keyboard_protocol();
  input_.set_key_up_aware(false);
}

void App::pause_if_not_focused() {
  if (screen_ == Screen::Battle) {
    return;  // Battle does not auto-pause
  }
  if (screen_ != Screen::Playing || game_.state().phase != Phase::Playing) {
    return;
  }
  game_.apply(Action::Pause);
  input_.reset();
  ui_dirty_ = true;
}

void App::open_battle() {
  // Placeholder transport until Consent; replaced on Y/O.
  battle_ = std::make_unique<BattleSession>(std::make_unique<LocalMediatorTransport>());
  battle_menu_.open();
  screen_ = Screen::Battle;
  input_.reset();
  renderer_.invalidate();
  ui_dirty_ = true;
}

void App::begin_battle_with(std::unique_ptr<MediatorTransport> transport) {
  battle_ = std::make_unique<BattleSession>(std::move(transport));
  battle_->on_consent_accepted();
}

void App::close_battle() {
  battle_.reset();
  battle_menu_.clear_abort();
  screen_ = Screen::Title;
  input_.reset();
  renderer_.invalidate();
  ui_dirty_ = true;
}

void App::open_settings() {
  settings_return_ = screen_;
  pause_if_not_focused();
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

void App::open_scores() {
  scores_view_ = settings_.game.randomizer;
  screen_ = Screen::Scores;
  input_.reset();
  renderer_.invalidate();
  ui_dirty_ = true;
}

void App::close_scores() {
  screen_ = Screen::Title;
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
    case SettingsMenu::Result::ClearedScores:
      scores_.clear_all();
      (void)scores_.save();
      ui_dirty_ = true;
      break;
  }
}

void App::start_game() {
  input_.reset();
  game_.reset(fresh_seed());
  last_phase_ = game_.state().phase;
  pending_name_edit_ = false;
  pending_rank_ = 0;
  name_buf_.clear();
  screen_ = Screen::Playing;
  renderer_.invalidate();
  ui_dirty_ = true;
}

void App::on_game_over_edge() {
  ScoreEntry e;
  e.score = game_.state().score;
  e.level = game_.state().level;
  e.lines = game_.state().lines;
  e.lines_per_level = game_.config().lines_per_level;
  e.name = HighScores::default_name_from_env();
  e.unix_time = static_cast<std::int64_t>(std::time(nullptr));
  e.elapsed_ms = game_.state().play_ms;

  pending_board_ = game_.config().randomizer;
  pending_mode_ = game_.config().play_mode;
  const auto rank = scores_.consider(pending_board_, pending_mode_, e);
  if (!rank.has_value()) {
    pending_name_edit_ = false;
    pending_rank_ = 0;
    return;
  }
  // Keep in memory only until the player confirms (Enter) or discards (Esc).
  pending_rank_ = *rank;
  pending_name_edit_ = true;
  name_buf_ = HighScores::default_name_from_env();
  ui_dirty_ = true;
}

void App::handle_name_key(const KeyEvent& ev) {
  if (ev.type != KeyEventType::Press) {
    return;
  }
  auto finish_edit = [&] {
    pending_name_edit_ = false;
    pending_rank_ = 0;
    name_buf_.clear();
    ui_dirty_ = true;
  };

  if (ev.key == Key::Esc) {
    (void)scores_.remove(pending_board_, pending_mode_, pending_rank_);
    finish_edit();
    return;
  }
  if (ev.key == Key::Enter) {
    const std::string saved =
        name_buf_.empty() ? std::string("---") : HighScores::sanitize_name(name_buf_, true);
    (void)scores_.set_name(pending_board_, pending_mode_, pending_rank_, saved);
    (void)scores_.save();
    finish_edit();
    return;
  }
  if (ev.key == Key::Backspace) {
    if (!name_buf_.empty()) {
      name_buf_.pop_back();
      ui_dirty_ = true;
    }
    return;
  }
  if (ev.key == Key::Char) {
    const unsigned char uc = ev.ch;
    if ((std::isalnum(uc) || uc == '_' || uc == '-') &&
        name_buf_.size() < static_cast<std::size_t>(kHighScoreNameMax)) {
      name_buf_.push_back(static_cast<char>(uc));
      ui_dirty_ = true;
    }
  }
}

void App::handle_scores_key(const KeyEvent& ev) {
  if (ev.type != KeyEventType::Press) {
    return;
  }
  if (ev.key == Key::Esc) {
    close_scores();
    return;
  }
  if (settings_.input.keys.action_for(ev) == Action::Quit) {
    close_scores();
    return;
  }
  if (ev.key == Key::Left) {
    scores_view_ = HighScores::cycle_randomizer(scores_view_, -1);
    renderer_.invalidate();
    ui_dirty_ = true;
    return;
  }
  if (ev.key == Key::Right) {
    scores_view_ = HighScores::cycle_randomizer(scores_view_, 1);
    renderer_.invalidate();
    ui_dirty_ = true;
    return;
  }
  if (ev.key == Key::Char) {
    const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(ev.ch)));
    if (c == 'h' || c == '[') {
      scores_view_ = HighScores::cycle_randomizer(scores_view_, -1);
      renderer_.invalidate();
      ui_dirty_ = true;
    } else if (c == 'l' || c == ']') {
      scores_view_ = HighScores::cycle_randomizer(scores_view_, 1);
      renderer_.invalidate();
      ui_dirty_ = true;
    }
  }
}

void App::handle_action(Action action) {
  if (pending_name_edit_) {
    return;
  }

  if (action == Action::Settings) {
    if (screen_ == Screen::Title || screen_ == Screen::Playing) {
      open_settings();
    }
    return;
  }

  if (action == Action::Quit) {
    if (screen_ == Screen::Title || screen_ == Screen::Scores) {
      quit_ = true;
      return;
    }
    if (game_.state().phase == Phase::Paused || game_.state().phase == Phase::GameOver ||
        game_.state().phase == Phase::Finished) {
      quit_ = true;
      return;
    }
    return;
  }

  if (screen_ == Screen::Title) {
    if (action == Action::Restart) {
      start_game();
    } else if (action == Action::Scores) {
      open_scores();
    } else if (action == Action::Battle) {
      open_battle();
    }
    return;
  }

  if (screen_ == Screen::Battle) {
    return;
  }

  if (screen_ == Screen::Scores) {
    return;
  }

  if (game_.state().phase == Phase::GameOver || game_.state().phase == Phase::Finished) {
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
    if (ev.key == Key::FocusOut) {
      pause_if_not_focused();
      continue;
    }
    if (ev.key == Key::FocusIn) {
      continue;
    }

    // Title / scores / battle UI / name-edit: press only. Settings accepts Repeat for hold-nav.
    const bool battle_ui = screen_ == Screen::Battle && battle_ &&
                           (battle_menu_.phase() != BattleMenuPhase::Session ||
                            !battle_->playing());
    const bool ui_press_only = screen_ == Screen::Title || screen_ == Screen::Scores ||
                               pending_name_edit_ || battle_ui ||
                               (screen_ == Screen::Battle &&
                                battle_menu_.phase() == BattleMenuPhase::ForfeitConfirm);
    if (ui_press_only && ev.type != KeyEventType::Press) {
      continue;
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

    if (screen_ == Screen::Battle && battle_) {
      const bool playing = battle_->playing() && battle_menu_.phase() == BattleMenuPhase::Session;
      if (!playing || ev.key == Key::Esc || battle_menu_.phase() == BattleMenuPhase::ForfeitConfirm) {
        battle_menu_.on_key(ev, *battle_);
        const BattleConsentChoice choice = battle_menu_.take_consent_choice();
        if (choice == BattleConsentChoice::Local) {
          begin_battle_with(std::make_unique<LocalMediatorTransport>());
        }
#if defined(TP_BATTLE_INSECURE)
        else if (choice == BattleConsentChoice::Online) {
          const char* host_env = std::getenv("TP_BATTLE_HOST");
          const char* port_env = std::getenv("TP_BATTLE_PORT");
          std::string host = host_env && host_env[0] ? host_env : "127.0.0.1";
          std::uint16_t port = 9000;
          if (port_env && port_env[0]) {
            const int p = std::atoi(port_env);
            if (p > 0 && p < 65536) {
              port = static_cast<std::uint16_t>(p);
            }
          }
          begin_battle_with(std::make_unique<InsecureMediatorTransport>(std::move(host), port));
        }
#endif
        if (battle_menu_.wants_abort_to_title() || battle_->wants_leave_to_title()) {
          close_battle();
        }
        ui_dirty_ = true;
        if (!playing) {
          continue;
        }
        // Esc handled by menu (forfeit confirm); don't also feed Input Pause.
        if (ev.key == Key::Esc || battle_menu_.phase() == BattleMenuPhase::ForfeitConfirm) {
          continue;
        }
      }
      // Playing: fall through to input_ for DAS.
    }

    if (pending_name_edit_ && screen_ == Screen::Playing) {
      handle_name_key(ev);
      continue;
    }

    if (screen_ == Screen::Scores) {
      handle_scores_key(ev);
      continue;
    }

    if (screen_ == Screen::Title && ev.key == Key::Esc) {
      quit_ = true;
      return;
    }

    // Title / game-over: Enter (and Space on title) always start/retry before keybinds.
    // Press only - with Kitty protocol, Enter Release after name-save must not restart.
    if (ev.type == KeyEventType::Press && screen_ == Screen::Title &&
        (ev.key == Key::Enter || ev.key == Key::Space)) {
      start_game();
      continue;
    }
    if (ev.type == KeyEventType::Press && screen_ == Screen::Playing &&
        (game_.state().phase == Phase::GameOver || game_.state().phase == Phase::Finished) &&
        ev.key == Key::Enter) {
      start_game();
      continue;
    }

    input_.on_key(ev);
    for (Action action : input_.drain()) {
      if (screen_ == Screen::Battle && battle_ && battle_->playing() &&
          battle_menu_.phase() == BattleMenuPhase::Session) {
        battle_->apply_action(action);
        ui_dirty_ = true;
      } else {
        handle_action(action);
      }
      if (quit_) {
        return;
      }
    }
  }
}

int App::run() {
  term_.enter_alt_screen();
  term_.hide_cursor();
  setup_keyboard();

  using clock = std::chrono::steady_clock;
  auto prev = clock::now();
  last_size_ = term_.size();
  last_phase_ = game_.state().phase;

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

    if (screen_ != Screen::Settings && screen_ != Screen::Scores && !pending_name_edit_ &&
        !(screen_ == Screen::Battle && battle_menu_.phase() != BattleMenuPhase::Session)) {
      input_.tick(elapsed_ms);
      for (Action action : input_.drain()) {
        if (screen_ == Screen::Battle && battle_ && battle_->playing() &&
            battle_menu_.phase() == BattleMenuPhase::Session) {
          battle_->apply_action(action);
          ui_dirty_ = true;
        } else {
          handle_action(action);
        }
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
    } else if (screen_ == Screen::Title || screen_ == Screen::Settings ||
               screen_ == Screen::Scores) {
      // Static until interaction dirties UI.
    } else if (screen_ == Screen::Battle && battle_) {
      battle_->tick(elapsed_ms);
      if (battle_->wants_leave_to_title()) {
        close_battle();
        need_draw = true;
      } else {
        need_draw = true;  // battle views change often
      }
    } else {
      if (game_.state().phase == Phase::Playing && elapsed_ms > 0) {
        game_.tick(elapsed_ms);
      }
      if (game_.consume_dirty()) {
        need_draw = true;
      }
    }

    if (screen_ == Screen::Playing) {
      const Phase ph = game_.state().phase;
      if ((ph == Phase::GameOver || ph == Phase::Finished) &&
          last_phase_ != Phase::GameOver && last_phase_ != Phase::Finished) {
        on_game_over_edge();
        need_draw = true;
      }
      last_phase_ = ph;
    }

    if (need_draw) {
      if (!size_ok) {
        renderer_.draw_too_small(settings_.input.keys);
      } else if (screen_ == Screen::Title) {
        renderer_.draw_title(scores_, settings_.game.randomizer, settings_.game.play_mode,
                             settings_.input.keys);
      } else if (screen_ == Screen::Settings) {
        menu_.ensure_visible(settings_viewport_rows(sz.rows));
        renderer_.draw_settings(menu_.view());
      } else if (screen_ == Screen::Scores) {
        renderer_.draw_scores(scores_, scores_view_, settings_.input.keys);
      } else if (screen_ == Screen::Battle && battle_) {
        const auto& sv = battle_->view();
        const bool show_board = sv.phase == BattleSessionPhase::Playing ||
                                sv.phase == BattleSessionPhase::MatchReady ||
                                sv.phase == BattleSessionPhase::Rematch;
        if (show_board && battle_menu_.phase() != BattleMenuPhase::Consent) {
          BattleHud hud = sv.hud;
          if (sv.phase == BattleSessionPhase::MatchReady) {
            hud.banner = "GET READY";
          } else if (sv.phase == BattleSessionPhase::Rematch) {
            hud.banner = "REMATCH";
          }
          if (sv.self != nullptr) {
            renderer_.draw_battle(*sv.self, sv.self_config, sv.opp, hud, settings_.input.keys);
          }
          if (battle_menu_.phase() == BattleMenuPhase::ForfeitConfirm ||
              sv.phase == BattleSessionPhase::Rematch ||
              sv.phase == BattleSessionPhase::MatchReady) {
            // Prompt drawn after board for forfeit/rematch/ready text.
            const auto mv = battle_menu_.view(*battle_);
            if (battle_menu_.phase() == BattleMenuPhase::ForfeitConfirm ||
                sv.phase == BattleSessionPhase::Rematch) {
              renderer_.draw_battle_prompt(mv.title, mv.line1, mv.line2, mv.line3, {},
                                           settings_.input.keys);
            }
          }
        } else {
          const auto mv = battle_menu_.view(*battle_);
          std::string entry;
          if (mv.show_token_entry) {
            entry = mv.token_buf;
          }
          renderer_.draw_battle_prompt(mv.title, mv.line1, mv.line2, mv.line3, entry,
                                       settings_.input.keys);
        }
      } else {
        GameOverExtras extras;
        const GameOverExtras* extras_ptr = nullptr;
        const Phase ph = game_.state().phase;
        if ((ph == Phase::GameOver || ph == Phase::Finished) && pending_rank_ > 0) {
          extras.is_high_score = true;
          extras.rank = pending_rank_;
          extras.editing_name = pending_name_edit_;
          extras.name_buf = name_buf_;
          extras.board = pending_board_;
          extras.mode = pending_mode_;
          extras_ptr = &extras;
        }
        renderer_.draw_game(game_.state(), game_.config(), settings_.input.keys, extras_ptr);
      }
      ui_dirty_ = false;
    }

    const bool idle = screen_ == Screen::Title || screen_ == Screen::Settings ||
                      screen_ == Screen::Scores || !size_ok ||
                      (screen_ == Screen::Playing && game_.state().phase != Phase::Playing) ||
                      (screen_ == Screen::Battle && battle_ && !battle_->playing()) ||
                      pending_name_edit_;
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
