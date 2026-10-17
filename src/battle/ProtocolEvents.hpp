#pragma once

#include "battle/OppBoardView.hpp"
#include "game/SrsKicks.hpp"
#include "game/Types.hpp"

#include <array>
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
  std::uint32_t seq = 0;
  std::int8_t x = 0;
  std::int8_t y = 0;
  std::uint8_t r = 0;
  std::uint8_t piece_id = 0;
};
struct EvStackSnap {
  std::uint32_t seq = 0;
  std::array<std::uint8_t, kStackSnapBytes> bits{};
};
struct EvTopOut {};
struct EvForfeit {};
struct EvRematchAccept {};
struct EvRematchLeave {};
struct EvPing {
  std::int64_t echo = 0;
};
struct EvBye {};

using OutboundEvent = std::variant<EvConsent, EvJoinFriend, EvQueueRandom, EvDequeue, EvClearEvent,
                                   EvPose, EvStackSnap, EvTopOut, EvForfeit, EvRematchAccept,
                                   EvRematchLeave, EvPing, EvBye>;

struct EvTokenShow {
  std::string token;
  std::int64_t expires_unix_ms = 0;
};
struct EvLobbyState {
  bool peer_present = false;
  std::uint8_t role = 0;  // 0 host, 1 joiner (wire)
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
  // Absolute wire fields (server MatchOver); optional for local transport.
  std::uint8_t wire_reason = 0;
  std::uint8_t winner_side = 255;
};
struct EvRematchStart {
  // Both accepted; new MatchStart will follow or this carries seeds.
  std::uint64_t seed_self = 1;
  std::uint64_t seed_opp = 2;
  int ready_countdown_ms = 3000;
};
struct EvError {
  std::uint16_t code = 0;
  std::string text;
};
struct EvPong {
  std::int64_t server_time_ms = 0;
};
struct EvPoseRelay {
  EvPose pose;
};

using InboundEvent =
    std::variant<EvTokenShow, EvLobbyState, EvMatchStart, EvGarbageApply, EvOppSnapshot, EvMatchOver,
                 EvRematchStart, EvError, EvPong, EvPoseRelay, EvBye>;

}  // namespace tp
