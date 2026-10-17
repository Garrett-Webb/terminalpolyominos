#pragma once

#include "battle/MediatorTransport.hpp"
#include "battle/OppBoardView.hpp"
#include "game/Game.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace tp {

enum class BattleSessionPhase : std::uint8_t {
  Idle = 0,
  Connecting,
  Lobby,
  MatchReady,
  Playing,
  Result,
  Rematch,
  Error,
};

enum class BattleResultKind : std::uint8_t {
  None = 0,
  Win,
  Lose,
  Draw,
  Disconnect,
};

struct BattleSessionView {
  BattleSessionPhase phase = BattleSessionPhase::Idle;
  bool offline = true;
  std::string my_token;
  std::string status;
  std::string error;
  const GameState* self = nullptr;
  GameConfig self_config{};
  OppBoardView opp{};
  BattleHud hud{};
  int countdown_ms = 0;  // MatchReady or Rematch remaining
  BattleResultKind result = BattleResultKind::None;
};

class BattleSession {
 public:
  explicit BattleSession(std::unique_ptr<MediatorTransport> transport);

  void on_consent_accepted();
  void join_friend(std::string_view token);
  void queue_random();
  void cancel_pending();
  void request_forfeit();
  void rematch_accept();
  void rematch_leave();
  void apply_action(Action action);
  void tick(int elapsed_ms);

  [[nodiscard]] const BattleSessionView& view() const { return view_; }
  [[nodiscard]] bool wants_leave_to_title() const { return leave_title_; }
  [[nodiscard]] bool playing() const { return phase_ == BattleSessionPhase::Playing; }
  void clear_leave_flag() { leave_title_ = false; }

 private:
  void handle_inbound(const InboundEvent& ev);
  void drain_transport();
  void sync_view();
  void start_self_match(std::uint64_t seed, int countdown_ms);
  void on_self_topout();
  GameConfig make_config() const;

  std::unique_ptr<MediatorTransport> transport_;
  Game self_;
  BattleSessionPhase phase_ = BattleSessionPhase::Idle;
  BattleSessionView view_{};
  bool leave_title_ = false;
  int countdown_ms_ = 0;
  int rematch_ms_ = 0;
  std::uint32_t clear_seq_ = 1;
  BattleResultKind result_ = BattleResultKind::None;
  int toast_ms_ = 0;
  bool self_is_host_ = true;
  int ping_accum_ms_ = 0;
};

}  // namespace tp
