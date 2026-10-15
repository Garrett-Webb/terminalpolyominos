#pragma once

#include "battle/BattleSession.hpp"
#include "terminal/Keys.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace tp {

enum class BattleMenuPhase : std::uint8_t {
  Consent = 0,
  Session,         // lobby / ready / play / rematch driven by BattleSession
  ForfeitConfirm,
};

struct BattleMenuView {
  BattleMenuPhase phase = BattleMenuPhase::Consent;
  std::string_view title{};
  std::string_view line1{};
  std::string_view line2{};
  std::string_view line3{};
  std::string token_buf{};
  bool show_token_entry = false;
};

class BattleMenu {
 public:
  void open();
  void on_key(const KeyEvent& ev, BattleSession& session);
  [[nodiscard]] BattleMenuView view(const BattleSession& session) const;
  [[nodiscard]] BattleMenuPhase phase() const { return phase_; }
  [[nodiscard]] bool wants_abort_to_title() const { return abort_title_; }
  void clear_abort() { abort_title_ = false; }

 private:
  BattleMenuPhase phase_ = BattleMenuPhase::Consent;
  std::string token_buf_;
  bool abort_title_ = false;
};

}  // namespace tp
