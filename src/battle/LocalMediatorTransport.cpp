#include "battle/LocalMediatorTransport.hpp"

#include "battle/OppBoardView.hpp"

#include <type_traits>
#include <utility>

namespace tp {
namespace {

GameConfig battle_game_config() {
  GameConfig cfg;
  cfg.battle = true;
  cfg.clear_flash_ms = 0;
  cfg.piece_lock_flash_ms = 0;
  return cfg;
}

}  // namespace

LocalMediatorTransport::LocalMediatorTransport() : opp_(battle_game_config()) {}

bool LocalMediatorTransport::connect() {
  close();
  alive_ = true;
  consented_ = false;
  stage_ = Stage::Idle;
  inbox_.clear();
  return true;
}

void LocalMediatorTransport::close() {
  alive_ = false;
  consented_ = false;
  stage_ = Stage::Closed;
  opp_alive_ = false;
  inbox_.clear();
  self_accepted_rematch_ = false;
}

void LocalMediatorTransport::push(InboundEvent ev) { inbox_.push_back(std::move(ev)); }

int LocalMediatorTransport::random_hole() {
  return static_cast<int>(rng_() % static_cast<unsigned>(kBoardWidth));
}

void LocalMediatorTransport::begin_match(std::uint64_t seed_self, std::uint64_t seed_opp,
                                         int countdown_ms) {
  opp_.set_config(battle_game_config());
  opp_.reset(seed_opp);
  opp_alive_ = true;
  stage_ = Stage::Ready;
  ready_left_ms_ = countdown_ms;
  self_accepted_rematch_ = false;
  EvMatchStart start;
  start.match_id = seed_self ^ seed_opp;
  start.seed_self = seed_self;
  start.seed_opp = seed_opp;
  start.ready_countdown_ms = countdown_ms;
  push(start);
  push(EvOppSnapshot{make_opp_view(opp_.state())});
}

void LocalMediatorTransport::send(const OutboundEvent& ev) {
  if (!alive_) {
    return;
  }
  std::visit(
      [&](const auto& msg) {
        using T = std::decay_t<decltype(msg)>;
        if constexpr (std::is_same_v<T, EvConsent>) {
          consented_ = true;
          stage_ = Stage::Lobby;
          EvTokenShow show;
          show.token = token_;
          push(show);
          push(EvLobbyState{false});
        } else if constexpr (std::is_same_v<T, EvJoinFriend> || std::is_same_v<T, EvQueueRandom>) {
          if (!consented_ || stage_ != Stage::Lobby) {
            return;
          }
          // Hot-seat: any join/queue starts a match immediately.
          const std::uint64_t a = 0xC0FFEEu ^ static_cast<std::uint64_t>(rng_());
          const std::uint64_t b = a + 1;
          begin_match(a, b, 3000);
        } else if constexpr (std::is_same_v<T, EvDequeue>) {
          if (stage_ == Stage::Lobby) {
            // no-op
          }
        } else if constexpr (std::is_same_v<T, EvClearEvent>) {
          if (stage_ == Stage::Playing) {
            on_clear_from_self(msg);
          }
        } else if constexpr (std::is_same_v<T, EvTopOut>) {
          if (stage_ == Stage::Playing) {
            stage_ = Stage::Rematch;
            EvMatchOver over;
            over.reason = EvMatchOver::Reason::SelfTopOut;
            over.rematch_deadline_ms = 15000;
            push(over);
          }
        } else if constexpr (std::is_same_v<T, EvForfeit>) {
          if (stage_ == Stage::Playing || stage_ == Stage::Ready) {
            stage_ = Stage::Rematch;
            EvMatchOver over;
            over.reason = EvMatchOver::Reason::SelfForfeit;
            over.rematch_deadline_ms = 15000;
            push(over);
          }
        } else if constexpr (std::is_same_v<T, EvRematchAccept>) {
          if (stage_ == Stage::Rematch) {
            self_accepted_rematch_ = true;
            // Hot-seat: local peer auto-accepts with you.
            const std::uint64_t a = static_cast<std::uint64_t>(rng_());
            const std::uint64_t b = a + 7;
            begin_match(a, b, 3000);
          }
        } else if constexpr (std::is_same_v<T, EvRematchLeave>) {
          stage_ = Stage::Lobby;
          opp_alive_ = false;
          push(EvLobbyState{false});
          EvTokenShow show;
          show.token = token_;
          push(show);
        } else if constexpr (std::is_same_v<T, EvPing>) {
          push(EvPong{});
        } else if constexpr (std::is_same_v<T, EvPose> || std::is_same_v<T, EvStackSnap>) {
          // ignore — hot-seat uses local OppSnapshot
        } else if constexpr (std::is_same_v<T, EvBye>) {
          alive_ = false;
        }
      },
      ev);
}

void LocalMediatorTransport::on_clear_from_self(const EvClearEvent& clear) {
  if (!opp_alive_) {
    return;
  }
  if (clear.lines_sent > 0) {
    opp_.queue_garbage(clear.lines_sent, random_hole());
  }
  push(EvOppSnapshot{make_opp_view(opp_.state())});
}

void LocalMediatorTransport::check_opp_topout() {
  if (!opp_alive_) {
    return;
  }
  if (opp_.state().phase == Phase::GameOver) {
    opp_alive_ = false;
    stage_ = Stage::Rematch;
    EvMatchOver over;
    over.reason = EvMatchOver::Reason::OppTopOut;
    over.rematch_deadline_ms = 15000;
    push(over);
  }
}

void LocalMediatorTransport::tick(int elapsed_ms) {
  if (!alive_ || elapsed_ms <= 0) {
    return;
  }
  if (stage_ == Stage::Ready) {
    ready_left_ms_ -= elapsed_ms;
    if (ready_left_ms_ <= 0) {
      ready_left_ms_ = 0;
      stage_ = Stage::Playing;
    }
    return;
  }
  if (!opp_alive_ || stage_ != Stage::Playing) {
    return;
  }
  opp_.tick(elapsed_ms);
  auto out = opp_.consume_outgoing();
  if (out.has_value() && out->lines_sent > 0) {
    EvGarbageApply g;
    g.rows = out->lines_sent;
    g.hole_col = random_hole();
    g.event_seq = event_seq_++;
    push(g);
  }
  push(EvOppSnapshot{make_opp_view(opp_.state())});
  check_opp_topout();
}

void LocalMediatorTransport::poll(std::vector<InboundEvent>& out) {
  out.insert(out.end(), inbox_.begin(), inbox_.end());
  inbox_.clear();
}

}  // namespace tp
