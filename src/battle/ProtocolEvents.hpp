#pragma once

#include "battle/OppBoardView.hpp"
#include "game/SrsKicks.hpp"
#include "game/Types.hpp"

#include <cstdint>
#include <string>
#include <variant>

namespace tp {

// Shared event PODs for LocalMediatorTransport and (later) TLS transport.
// No sockets / encode here — pure data.

struct EvConsent {};
struct EvJoinFriend {
  std::string token;
};
struct EvQueueRandom {};
struct EvDequeue {};
struct EvClearEvent {
  std::uint32_t seq = 0;
  int lines_cleared = 0;
  SpinType spin = SpinType::None;
  int combo = 0;
  bool b2b = false;
  bool perfect_clear = false;
  int raw_send = 0;    // before cancel (advisory for real server)
  int lines_sent = 0;  // after cancel — apply this many to opponent
};
struct EvPose {
  // Reserved for online silhouette streaming; unused in hot-seat v1.
};
struct EvTopOut {};
struct EvForfeit {};
struct EvRematchAccept {};
struct EvRematchLeave {};
struct EvPing {};

using OutboundEvent = std::variant<EvConsent, EvJoinFriend, EvQueueRandom, EvDequeue, EvClearEvent,
                                   EvPose, EvTopOut, EvForfeit, EvRematchAccept, EvRematchLeave,
                                   EvPing>;

struct EvTokenShow {
  std::string token;
  std::int64_t expires_unix_ms = 0;
};
struct EvLobbyState {
  bool peer_present = false;
};
struct EvMatchStart {
  std::uint64_t match_id = 0;
  std::uint64_t seed_self = 1;
  std::uint64_t seed_opp = 2;
  int ready_countdown_ms = 3000;
};
struct EvGarbageApply {
  int rows = 0;
  int hole_col = 0;
  std::uint32_t event_seq = 0;
};
struct EvOppSnapshot {
  OppBoardView view;
};
struct EvMatchOver {
  enum class Reason : std::uint8_t {
    SelfTopOut = 0,
    OppTopOut,
    SelfForfeit,
    OppForfeit,
    Draw,
    Disconnect,
  };
  Reason reason = Reason::SelfTopOut;
  int rematch_deadline_ms = 15000;
};
struct EvRematchStart {
  // Both accepted; new MatchStart will follow or this carries seeds.
  std::uint64_t seed_self = 1;
  std::uint64_t seed_opp = 2;
  int ready_countdown_ms = 3000;
};
struct EvError {
  std::string text;
};
struct EvPong {
  std::int64_t server_time_ms = 0;
};

using InboundEvent =
    std::variant<EvTokenShow, EvLobbyState, EvMatchStart, EvGarbageApply, EvOppSnapshot, EvMatchOver,
                 EvRematchStart, EvError, EvPong>;

}  // namespace tp
