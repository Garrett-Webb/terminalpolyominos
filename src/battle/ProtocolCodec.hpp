#pragma once

#include "battle/ProtocolEvents.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tp {

// Wire layout matches tpoly_battle_server proto.rs / battle-mode-server.md §2.
inline constexpr std::size_t kMaxFrameBody = 4096;

inline constexpr std::uint8_t kMsgHello = 1;
inline constexpr std::uint8_t kMsgHelloAck = 2;
inline constexpr std::uint8_t kMsgConsent = 3;
inline constexpr std::uint8_t kMsgTokenShow = 4;
inline constexpr std::uint8_t kMsgJoinFriend = 5;
inline constexpr std::uint8_t kMsgQueueRandom = 6;
inline constexpr std::uint8_t kMsgDequeue = 7;
inline constexpr std::uint8_t kMsgLobbyState = 8;
inline constexpr std::uint8_t kMsgMatchStart = 9;
inline constexpr std::uint8_t kMsgClearEvent = 10;
inline constexpr std::uint8_t kMsgPose = 11;
inline constexpr std::uint8_t kMsgGarbageApply = 12;
inline constexpr std::uint8_t kMsgTopOut = 13;
inline constexpr std::uint8_t kMsgForfeit = 14;
inline constexpr std::uint8_t kMsgMatchOver = 15;
inline constexpr std::uint8_t kMsgRematchAccept = 16;
inline constexpr std::uint8_t kMsgRematchLeave = 17;
inline constexpr std::uint8_t kMsgPing = 18;
inline constexpr std::uint8_t kMsgPong = 19;
inline constexpr std::uint8_t kMsgError = 20;
inline constexpr std::uint8_t kMsgBye = 21;
inline constexpr std::uint8_t kMsgOppSnapshot = 22;
inline constexpr std::uint8_t kMsgStackSnap = 23;

inline constexpr std::uint8_t kClearFlagB2b = 1;
inline constexpr std::uint8_t kClearFlagPc = 2;

struct WireHello {
  std::uint16_t client_ver = 1;
  std::uint16_t proto_ver = 1;
  std::uint32_t build_id = 0;
};

struct WireHelloAck {
  std::uint16_t server_ver = 0;
  std::uint16_t proto_min = 0;
  std::uint16_t proto_max = 0;
  std::uint32_t features = 0;
};

struct Frame {
  std::uint8_t msg_type = 0;
  std::uint8_t flags = 0;
  std::vector<std::uint8_t> payload;
};

enum class CodecError {
  Ok = 0,
  Truncated,
  TooLarge,
  NonzeroFlags,
  BadPayload,
  UnknownType,
};

[[nodiscard]] std::optional<Frame> try_decode_frame(std::vector<std::uint8_t>& buf, CodecError* err = nullptr);
[[nodiscard]] std::vector<std::uint8_t> encode_frame(const Frame& frame, CodecError* err = nullptr);

[[nodiscard]] std::vector<std::uint8_t> encode_hello(const WireHello& h);
[[nodiscard]] std::optional<WireHelloAck> decode_hello_ack(const Frame& frame);

[[nodiscard]] std::vector<std::uint8_t> encode_outbound(const OutboundEvent& ev, CodecError* err = nullptr);
[[nodiscard]] std::optional<InboundEvent> decode_inbound(const Frame& frame, CodecError* err = nullptr);

// Absolute MatchOver from wire (before mapping to EvMatchOver::Reason).
struct WireMatchOver {
  std::uint8_t reason = 0;
  std::uint8_t winner_side = 0;
  std::int32_t rematch_deadline_ms = 15000;
};

[[nodiscard]] EvMatchOver map_match_over(const WireMatchOver& w, bool self_is_host);

}  // namespace tp
