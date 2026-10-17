#include "battle/BattleSession.hpp"

#include "battle/ProtocolCodec.hpp"

#include <type_traits>
#include <utility>
#include <vector>

namespace tp {

BattleSession::BattleSession(std::unique_ptr<MediatorTransport> transport)
    : transport_(std::move(transport)), self_(make_config()) {
  sync_view();
}

GameConfig BattleSession::make_config() const {
  GameConfig cfg;
  cfg.battle = true;
  cfg.clear_flash_ms = 100;
  cfg.piece_lock_flash_ms = 100;
  return cfg;
}

void BattleSession::on_consent_accepted() {
  leave_title_ = false;
  result_ = BattleResultKind::None;
  phase_ = BattleSessionPhase::Connecting;
  if (!transport_ || !transport_->connect()) {
    phase_ = BattleSessionPhase::Error;
    view_.error = "Could not start battle session";
    sync_view();
    return;
  }
  transport_->send(EvConsent{});
  drain_transport();
  if (phase_ == BattleSessionPhase::Connecting) {
    phase_ = BattleSessionPhase::Lobby;
  }
  sync_view();
}

void BattleSession::join_friend(std::string_view token) {
  if (phase_ != BattleSessionPhase::Lobby || !transport_) {
    return;
  }
  EvJoinFriend join;
  join.token = std::string(token);
  transport_->send(join);
  drain_transport();
  sync_view();
}

void BattleSession::queue_random() {
  if (phase_ != BattleSessionPhase::Lobby || !transport_) {
    return;
  }
  transport_->send(EvQueueRandom{});
  drain_transport();
  sync_view();
}

void BattleSession::cancel_pending() {
  if (!transport_) {
    return;
  }
  transport_->send(EvDequeue{});
  drain_transport();
  sync_view();
}

void BattleSession::request_forfeit() {
  if (!transport_) {
    return;
  }
  if (phase_ == BattleSessionPhase::Playing || phase_ == BattleSessionPhase::MatchReady) {
    transport_->send(EvForfeit{});
    drain_transport();
    sync_view();
  }
}

void BattleSession::rematch_accept() {
  if (phase_ != BattleSessionPhase::Rematch || !transport_) {
    return;
  }
  transport_->send(EvRematchAccept{});
  drain_transport();
  sync_view();
}

void BattleSession::rematch_leave() {
  if (!transport_) {
    leave_title_ = true;
    return;
  }
  if (phase_ == BattleSessionPhase::Rematch || phase_ == BattleSessionPhase::Result) {
    transport_->send(EvRematchLeave{});
    drain_transport();
    if (phase_ == BattleSessionPhase::Lobby) {
      // stay in lobby
    }
    sync_view();
    return;
  }
  transport_->close();
  leave_title_ = true;
  phase_ = BattleSessionPhase::Idle;
  sync_view();
}

void BattleSession::apply_action(Action action) {
  if (phase_ != BattleSessionPhase::Playing) {
    return;
  }
  if (action == Action::Pause || action == Action::Settings || action == Action::Quit ||
      action == Action::Restart || action == Action::Scores || action == Action::Battle) {
    return;
  }
  self_.apply(action);
}

void BattleSession::start_self_match(std::uint64_t seed, int countdown_ms) {
  self_.set_config(make_config());
  self_.reset(seed);
  countdown_ms_ = countdown_ms;
  phase_ = BattleSessionPhase::MatchReady;
  result_ = BattleResultKind::None;
  view_.hud.last_sent = 0;
  view_.hud.last_cancelled = 0;
}

void BattleSession::on_self_topout() {
  if (phase_ != BattleSessionPhase::Playing || !transport_) {
    return;
  }
  transport_->send(EvTopOut{});
  drain_transport();
}

void BattleSession::handle_inbound(const InboundEvent& ev) {
  std::visit(
      [&](const auto& msg) {
        using T = std::decay_t<decltype(msg)>;
        if constexpr (std::is_same_v<T, EvTokenShow>) {
          view_.my_token = msg.token;
          if (phase_ == BattleSessionPhase::Connecting) {
            phase_ = BattleSessionPhase::Lobby;
          }
        } else if constexpr (std::is_same_v<T, EvLobbyState>) {
          phase_ = BattleSessionPhase::Lobby;
          self_is_host_ = msg.role == 0;
          view_.status = msg.peer_present ? "Opponent present" : "Waiting…";
        } else if constexpr (std::is_same_v<T, EvMatchStart>) {
          start_self_match(msg.seed_self, msg.ready_countdown_ms);
        } else if constexpr (std::is_same_v<T, EvGarbageApply>) {
          self_.queue_garbage(msg.rows, msg.hole_col);
          view_.hud.pending_incoming = self_.pending_incoming();
        } else if constexpr (std::is_same_v<T, EvOppSnapshot>) {
          view_.opp = msg.view;
        } else if constexpr (std::is_same_v<T, EvPoseRelay>) {
          // Pose-only preview: WP5 may draw active piece; keep session stable for now.
          (void)msg;
        } else if constexpr (std::is_same_v<T, EvMatchOver>) {
          EvMatchOver mapped = msg;
          if (msg.winner_side != 255) {
            WireMatchOver w;
            w.reason = msg.wire_reason;
            w.winner_side = msg.winner_side;
            w.rematch_deadline_ms = msg.rematch_deadline_ms;
            mapped = map_match_over(w, self_is_host_);
          }
          phase_ = BattleSessionPhase::Result;
          rematch_ms_ = mapped.rematch_deadline_ms;
          switch (mapped.reason) {
            case EvMatchOver::Reason::OppTopOut:
            case EvMatchOver::Reason::OppForfeit:
              result_ = BattleResultKind::Win;
              break;
            case EvMatchOver::Reason::Draw:
              result_ = BattleResultKind::Draw;
              break;
            case EvMatchOver::Reason::Disconnect:
              result_ = BattleResultKind::Disconnect;
              break;
            case EvMatchOver::Reason::SelfTopOut:
            case EvMatchOver::Reason::SelfForfeit:
            default:
              result_ = BattleResultKind::Lose;
              break;
          }
          phase_ = BattleSessionPhase::Rematch;
        } else if constexpr (std::is_same_v<T, EvRematchStart>) {
          start_self_match(msg.seed_self, msg.ready_countdown_ms);
        } else if constexpr (std::is_same_v<T, EvError>) {
          phase_ = BattleSessionPhase::Error;
          view_.error = msg.text;
        } else if constexpr (std::is_same_v<T, EvPong>) {
          // ignore
        } else if constexpr (std::is_same_v<T, EvBye>) {
          phase_ = BattleSessionPhase::Error;
          view_.error = "Peer disconnected";
        }
      },
      ev);
}

void BattleSession::drain_transport() {
  if (!transport_) {
    return;
  }
  std::vector<InboundEvent> events;
  transport_->poll(events);
  for (const InboundEvent& ev : events) {
    handle_inbound(ev);
  }
}

void BattleSession::tick(int elapsed_ms) {
  if (!transport_ || elapsed_ms < 0) {
    return;
  }

  if (!transport_->offline() && transport_->alive() &&
      (phase_ == BattleSessionPhase::Lobby || phase_ == BattleSessionPhase::MatchReady ||
       phase_ == BattleSessionPhase::Playing || phase_ == BattleSessionPhase::Rematch)) {
    ping_accum_ms_ += elapsed_ms;
    if (ping_accum_ms_ >= 5000) {
      ping_accum_ms_ = 0;
      EvPing ping;
      ping.echo = static_cast<std::int64_t>(clear_seq_);
      transport_->send(ping);
    }
  }

  if (phase_ == BattleSessionPhase::MatchReady) {
    countdown_ms_ -= elapsed_ms;
    transport_->tick(elapsed_ms);
    if (countdown_ms_ <= 0) {
      countdown_ms_ = 0;
      phase_ = BattleSessionPhase::Playing;
    }
    drain_transport();
    sync_view();
    return;
  }

  if (phase_ == BattleSessionPhase::Rematch) {
    rematch_ms_ -= elapsed_ms;
    if (rematch_ms_ <= 0) {
      rematch_leave();
      if (phase_ == BattleSessionPhase::Lobby) {
        sync_view();
        return;
      }
      leave_title_ = true;
    }
    drain_transport();
    sync_view();
    return;
  }

  if (phase_ == BattleSessionPhase::Playing) {
    self_.tick(elapsed_ms);
    transport_->tick(elapsed_ms);

    auto outgoing = self_.consume_outgoing();
    if (outgoing.has_value()) {
      EvClearEvent clear;
      clear.seq = clear_seq_++;
      clear.lines_cleared = outgoing->lines_cleared;
      clear.combo = outgoing->combo;
      clear.b2b = outgoing->b2b;
      clear.perfect_clear = outgoing->perfect_clear;
      clear.raw_send = outgoing->raw_send;
      clear.lines_sent = outgoing->lines_sent;
      transport_->send(clear);
      view_.hud.last_sent = outgoing->lines_sent;
      view_.hud.last_cancelled = outgoing->cancelled;
      toast_ms_ = 800;
    }

    if (toast_ms_ > 0) {
      toast_ms_ -= elapsed_ms;
      if (toast_ms_ <= 0) {
        toast_ms_ = 0;
        view_.hud.last_sent = 0;
        view_.hud.last_cancelled = 0;
      }
    }

    if (self_.state().phase == Phase::GameOver) {
      on_self_topout();
    }

    drain_transport();
    sync_view();
    return;
  }

  drain_transport();
  sync_view();
}

void BattleSession::sync_view() {
  view_.phase = phase_;
  view_.offline = transport_ ? transport_->offline() : true;
  view_.self = &self_.state();
  view_.self_config = self_.config();
  view_.hud.pending_incoming = self_.pending_incoming();
  view_.hud.offline = view_.offline;
  view_.countdown_ms = (phase_ == BattleSessionPhase::MatchReady) ? countdown_ms_
                       : (phase_ == BattleSessionPhase::Rematch)     ? rematch_ms_
                                                                     : 0;
  view_.result = result_;
  if (phase_ == BattleSessionPhase::Lobby && view_.status.empty()) {
    view_.status = "Enter token / R random";
  }
}

}  // namespace tp
