#include "app/BattleMenu.hpp"

#include <cctype>

namespace tp {

bool BattleMenu::online_available() {
#if defined(TP_BATTLE_INSECURE)
  return true;
#else
  return false;
#endif
}

void BattleMenu::open() {
  phase_ = BattleMenuPhase::Consent;
  token_buf_.clear();
  abort_title_ = false;
  consent_choice_ = BattleConsentChoice::None;
}

BattleConsentChoice BattleMenu::take_consent_choice() {
  const BattleConsentChoice c = consent_choice_;
  consent_choice_ = BattleConsentChoice::None;
  return c;
}

void BattleMenu::on_key(const KeyEvent& ev, BattleSession& session) {
  if (ev.type != KeyEventType::Press) {
    return;
  }

  if (phase_ == BattleMenuPhase::Consent) {
    if (ev.key == Key::Esc || (ev.key == Key::Char && (ev.ch == 'n' || ev.ch == 'N'))) {
      abort_title_ = true;
      consent_choice_ = BattleConsentChoice::Abort;
      return;
    }
    if (online_available() && ev.key == Key::Char && (ev.ch == 'o' || ev.ch == 'O')) {
      consent_choice_ = BattleConsentChoice::Online;
      phase_ = BattleMenuPhase::Session;
      return;
    }
    if (ev.key == Key::Enter || (ev.key == Key::Char && (ev.ch == 'y' || ev.ch == 'Y'))) {
      consent_choice_ = BattleConsentChoice::Local;
      phase_ = BattleMenuPhase::Session;
      return;
    }
    return;
  }

  if (phase_ == BattleMenuPhase::ForfeitConfirm) {
    if (ev.key == Key::Char && (ev.ch == 'y' || ev.ch == 'Y')) {
      session.request_forfeit();
      phase_ = BattleMenuPhase::Session;
      return;
    }
    if (ev.key == Key::Esc || (ev.key == Key::Char && (ev.ch == 'n' || ev.ch == 'N'))) {
      phase_ = BattleMenuPhase::Session;
      return;
    }
    return;
  }

  // Session-driven screens.
  const auto sp = session.view().phase;

  if (sp == BattleSessionPhase::Lobby) {
    if (ev.key == Key::Esc) {
      session.rematch_leave();
      abort_title_ = true;
      return;
    }
    if (ev.key == Key::Char && (ev.ch == 'r' || ev.ch == 'R')) {
      session.queue_random();
      return;
    }
    if (ev.key == Key::Enter) {
      if (token_buf_.empty()) {
        session.queue_random();
      } else {
        session.join_friend(token_buf_);
      }
      return;
    }
    if (ev.key == Key::Backspace) {
      if (!token_buf_.empty()) {
        token_buf_.pop_back();
      }
      return;
    }
    if (ev.key == Key::Char) {
      const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(ev.ch)));
      const bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || c == '-';
      if (ok && token_buf_.size() < 9) {
        token_buf_.push_back(c);
      }
    }
    return;
  }

  if (sp == BattleSessionPhase::Playing || sp == BattleSessionPhase::MatchReady) {
    if (ev.key == Key::Esc) {
      phase_ = BattleMenuPhase::ForfeitConfirm;
      return;
    }
    return;
  }

  if (sp == BattleSessionPhase::Rematch || sp == BattleSessionPhase::Result) {
    if (ev.key == Key::Enter) {
      session.rematch_accept();
      return;
    }
    if (ev.key == Key::Esc || (ev.key == Key::Char && (ev.ch == 'q' || ev.ch == 'Q'))) {
      session.rematch_leave();
      return;
    }
    return;
  }

  if (sp == BattleSessionPhase::Error) {
    if (ev.key == Key::Enter || ev.key == Key::Esc) {
      abort_title_ = true;
    }
  }
}

BattleMenuView BattleMenu::view(const BattleSession& session) const {
  BattleMenuView v;
  v.phase = phase_;
  v.token_buf = token_buf_;

  if (phase_ == BattleMenuPhase::Consent) {
    v.title = "BATTLE";
    if (online_available()) {
      v.line1 = "Y = hot-seat (offline)    O = online (insecure TCP)";
      v.line2 = "N / Esc = back";
    } else {
      v.line1 = "Hot-seat practice (offline). No network.";
      v.line2 = "Y / Enter = continue    N / Esc = back";
    }
    return v;
  }
  if (phase_ == BattleMenuPhase::ForfeitConfirm) {
    v.title = "FORFEIT?";
    v.line1 = "Y = forfeit    N / Esc = resume";
    return v;
  }

  const auto& sv = session.view();
  switch (sv.phase) {
    case BattleSessionPhase::Lobby:
      v.title = "LOBBY";
      v.line1 = "Your token:";
      v.line2 = sv.my_token.empty() ? "…" : sv.my_token;
      v.line3 = "Type friend token + Enter | R random | Esc leave";
      v.show_token_entry = true;
      break;
    case BattleSessionPhase::MatchReady:
      v.title = "GET READY";
      v.line1 = "Starting…";
      break;
    case BattleSessionPhase::Rematch:
    case BattleSessionPhase::Result: {
      v.title = "MATCH OVER";
      switch (sv.result) {
        case BattleResultKind::Win:
          v.line1 = "YOU WIN";
          break;
        case BattleResultKind::Lose:
          v.line1 = "YOU LOSE";
          break;
        case BattleResultKind::Draw:
          v.line1 = "DRAW";
          break;
        case BattleResultKind::Disconnect:
          v.line1 = "DISCONNECT";
          break;
        case BattleResultKind::None:
          v.line1 = "…";
          break;
      }
      v.line2 = "Enter rematch    Esc leave";
      break;
    }
    case BattleSessionPhase::Error:
      v.title = "ERROR";
      v.line1 = sv.error.empty() ? "Unknown error" : sv.error;
      v.line2 = "Enter / Esc = title";
      break;
    default:
      break;
  }
  return v;
}

}  // namespace tp
