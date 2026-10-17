#include "battle/ProtocolCodec.hpp"

#include <algorithm>
#include <type_traits>

namespace tp {
namespace {

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t v) {
  out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<std::uint8_t>(v & 0xff));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
  out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xff));
  out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xff));
  out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<std::uint8_t>(v & 0xff));
}

void append_u64(std::vector<std::uint8_t>& out, std::uint64_t v) {
  for (int i = 7; i >= 0; --i) {
    out.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xff));
  }
}

void append_i64(std::vector<std::uint8_t>& out, std::int64_t v) {
  append_u64(out, static_cast<std::uint64_t>(v));
}

void append_i32(std::vector<std::uint8_t>& out, std::int32_t v) {
  append_u32(out, static_cast<std::uint32_t>(v));
}

std::uint16_t read_u16(const std::uint8_t* p) {
  return static_cast<std::uint16_t>((std::uint16_t(p[0]) << 8) | p[1]);
}

std::uint32_t read_u32(const std::uint8_t* p) {
  return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) | (std::uint32_t(p[2]) << 8) |
         std::uint32_t(p[3]);
}

std::uint64_t read_u64(const std::uint8_t* p) {
  std::uint64_t v = 0;
  for (int i = 0; i < 8; ++i) {
    v = (v << 8) | p[i];
  }
  return v;
}

std::int64_t read_i64(const std::uint8_t* p) {
  return static_cast<std::int64_t>(read_u64(p));
}

std::int32_t read_i32(const std::uint8_t* p) {
  return static_cast<std::int32_t>(read_u32(p));
}

void set_err(CodecError* err, CodecError e) {
  if (err) {
    *err = e;
  }
}

}  // namespace

std::optional<Frame> try_decode_frame(std::vector<std::uint8_t>& buf, CodecError* err) {
  set_err(err, CodecError::Ok);
  if (buf.size() < 2) {
    return std::nullopt;
  }
  const std::size_t body_len = read_u16(buf.data());
  if (body_len > kMaxFrameBody) {
    set_err(err, CodecError::TooLarge);
    return std::nullopt;
  }
  if (buf.size() < 2 + body_len) {
    return std::nullopt;
  }
  if (body_len < 2) {
    set_err(err, CodecError::BadPayload);
    return std::nullopt;
  }
  Frame f;
  f.msg_type = buf[2];
  f.flags = buf[3];
  if (f.flags != 0) {
    set_err(err, CodecError::NonzeroFlags);
    return std::nullopt;
  }
  f.payload.assign(buf.begin() + 4, buf.begin() + 2 + static_cast<std::ptrdiff_t>(body_len));
  buf.erase(buf.begin(), buf.begin() + 2 + static_cast<std::ptrdiff_t>(body_len));
  return f;
}

std::vector<std::uint8_t> encode_frame(const Frame& frame, CodecError* err) {
  set_err(err, CodecError::Ok);
  const std::size_t body_len = 2 + frame.payload.size();
  if (body_len > kMaxFrameBody) {
    set_err(err, CodecError::TooLarge);
    return {};
  }
  std::vector<std::uint8_t> out;
  out.reserve(2 + body_len);
  append_u16(out, static_cast<std::uint16_t>(body_len));
  out.push_back(frame.msg_type);
  out.push_back(frame.flags);
  out.insert(out.end(), frame.payload.begin(), frame.payload.end());
  return out;
}

std::vector<std::uint8_t> encode_hello(const WireHello& h) {
  Frame f;
  f.msg_type = kMsgHello;
  append_u16(f.payload, h.client_ver);
  append_u16(f.payload, h.proto_ver);
  append_u32(f.payload, h.build_id);
  return encode_frame(f);
}

std::optional<WireHelloAck> decode_hello_ack(const Frame& frame) {
  if (frame.msg_type != kMsgHelloAck || frame.payload.size() != 10) {
    return std::nullopt;
  }
  WireHelloAck a;
  a.server_ver = read_u16(frame.payload.data());
  a.proto_min = read_u16(frame.payload.data() + 2);
  a.proto_max = read_u16(frame.payload.data() + 4);
  a.features = read_u32(frame.payload.data() + 6);
  return a;
}

std::vector<std::uint8_t> encode_outbound(const OutboundEvent& ev, CodecError* err) {
  set_err(err, CodecError::Ok);
  Frame f;
  f.flags = 0;
  std::visit(
      [&](const auto& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, EvConsent>) {
          f.msg_type = kMsgConsent;
        } else if constexpr (std::is_same_v<T, EvJoinFriend>) {
          f.msg_type = kMsgJoinFriend;
          if (e.token.size() > 255) {
            set_err(err, CodecError::BadPayload);
            return;
          }
          f.payload.push_back(static_cast<std::uint8_t>(e.token.size()));
          f.payload.insert(f.payload.end(), e.token.begin(), e.token.end());
        } else if constexpr (std::is_same_v<T, EvQueueRandom>) {
          f.msg_type = kMsgQueueRandom;
        } else if constexpr (std::is_same_v<T, EvDequeue>) {
          f.msg_type = kMsgDequeue;
        } else if constexpr (std::is_same_v<T, EvClearEvent>) {
          f.msg_type = kMsgClearEvent;
          append_u32(f.payload, e.seq);
          f.payload.push_back(static_cast<std::uint8_t>(std::clamp(e.lines_cleared, 0, 255)));
          f.payload.push_back(static_cast<std::uint8_t>(e.spin));
          f.payload.push_back(static_cast<std::uint8_t>(std::clamp(e.combo, 0, 255)));
          std::uint8_t flags = 0;
          if (e.b2b) {
            flags |= kClearFlagB2b;
          }
          if (e.perfect_clear) {
            flags |= kClearFlagPc;
          }
          f.payload.push_back(flags);
          append_i32(f.payload, 0);  // board_hash advisory
        } else if constexpr (std::is_same_v<T, EvPose>) {
          f.msg_type = kMsgPose;
          append_u32(f.payload, e.seq);
          f.payload.push_back(static_cast<std::uint8_t>(e.x));
          f.payload.push_back(static_cast<std::uint8_t>(e.y));
          f.payload.push_back(e.r);
          f.payload.push_back(e.piece_id);
        } else if constexpr (std::is_same_v<T, EvTopOut>) {
          f.msg_type = kMsgTopOut;
        } else if constexpr (std::is_same_v<T, EvForfeit>) {
          f.msg_type = kMsgForfeit;
        } else if constexpr (std::is_same_v<T, EvRematchAccept>) {
          f.msg_type = kMsgRematchAccept;
        } else if constexpr (std::is_same_v<T, EvRematchLeave>) {
          f.msg_type = kMsgRematchLeave;
        } else if constexpr (std::is_same_v<T, EvPing>) {
          f.msg_type = kMsgPing;
          append_i64(f.payload, e.echo);
        } else if constexpr (std::is_same_v<T, EvBye>) {
          f.msg_type = kMsgBye;
        }
      },
      ev);
  if (err && *err != CodecError::Ok) {
    return {};
  }
  return encode_frame(f, err);
}

std::optional<InboundEvent> decode_inbound(const Frame& frame, CodecError* err) {
  set_err(err, CodecError::Ok);
  const auto& p = frame.payload;
  switch (frame.msg_type) {
    case kMsgTokenShow: {
      if (p.empty() || p.size() < 1u + p[0] + 8u) {
        set_err(err, CodecError::BadPayload);
        return std::nullopt;
      }
      const std::size_t n = p[0];
      EvTokenShow t;
      t.token.assign(reinterpret_cast<const char*>(p.data() + 1), n);
      t.expires_unix_ms = read_i64(p.data() + 1 + n);
      return t;
    }
    case kMsgLobbyState: {
      if (p.size() != 2) {
        set_err(err, CodecError::BadPayload);
        return std::nullopt;
      }
      EvLobbyState s;
      s.peer_present = p[0] != 0;
      s.role = p[1];
      return s;
    }
    case kMsgMatchStart: {
      if (p.size() != 28) {
        set_err(err, CodecError::BadPayload);
        return std::nullopt;
      }
      EvMatchStart m;
      m.match_id = read_u64(p.data());
      m.seed_self = read_u64(p.data() + 8);
      m.seed_opp = read_u64(p.data() + 16);
      m.ready_countdown_ms = read_i32(p.data() + 24);
      return m;
    }
    case kMsgGarbageApply: {
      if (p.size() != 6) {
        set_err(err, CodecError::BadPayload);
        return std::nullopt;
      }
      EvGarbageApply g;
      g.rows = p[0];
      g.hole_col = p[1];
      g.event_seq = read_u32(p.data() + 2);
      return g;
    }
    case kMsgPose: {
      if (p.size() != 8) {
        set_err(err, CodecError::BadPayload);
        return std::nullopt;
      }
      EvPoseRelay r;
      r.pose.seq = read_u32(p.data());
      r.pose.x = static_cast<std::int8_t>(p[4]);
      r.pose.y = static_cast<std::int8_t>(p[5]);
      r.pose.r = p[6];
      r.pose.piece_id = p[7];
      return r;
    }
    case kMsgMatchOver: {
      if (p.size() != 6) {
        set_err(err, CodecError::BadPayload);
        return std::nullopt;
      }
      WireMatchOver w;
      w.reason = p[0];
      w.winner_side = p[1];
      w.rematch_deadline_ms = read_i32(p.data() + 2);
      // Side unknown at codec layer — store absolute; session/transport maps later.
      EvMatchOver m = map_match_over(w, true);
      m.wire_reason = w.reason;
      m.winner_side = w.winner_side;
      return m;
    }
    case kMsgPong: {
      if (p.size() != 8) {
        set_err(err, CodecError::BadPayload);
        return std::nullopt;
      }
      EvPong o;
      o.server_time_ms = read_i64(p.data());
      return o;
    }
    case kMsgError: {
      if (p.size() < 3 || p.size() != 3u + p[2]) {
        set_err(err, CodecError::BadPayload);
        return std::nullopt;
      }
      EvError e;
      e.code = read_u16(p.data());
      e.text.assign(reinterpret_cast<const char*>(p.data() + 3), p[2]);
      return e;
    }
    case kMsgBye:
      if (!p.empty()) {
        set_err(err, CodecError::BadPayload);
        return std::nullopt;
      }
      return EvBye{};
    default:
      set_err(err, CodecError::UnknownType);
      return std::nullopt;
  }
}

EvMatchOver map_match_over(const WireMatchOver& w, bool self_is_host) {
  EvMatchOver m;
  m.rematch_deadline_ms = w.rematch_deadline_ms;
  m.wire_reason = w.reason;
  m.winner_side = w.winner_side;
  constexpr std::uint8_t kSideHost = 0;
  constexpr std::uint8_t kSideJoiner = 1;
  constexpr std::uint8_t kSideDraw = 2;
  constexpr std::uint8_t kReasonTopOut = 0;
  constexpr std::uint8_t kReasonForfeit = 1;
  constexpr std::uint8_t kReasonDisconnect = 2;
  constexpr std::uint8_t kReasonDraw = 3;

  if (w.reason == kReasonDraw || w.winner_side == kSideDraw) {
    m.reason = EvMatchOver::Reason::Draw;
    return m;
  }
  const bool self_won =
      (self_is_host && w.winner_side == kSideHost) || (!self_is_host && w.winner_side == kSideJoiner);
  if (w.reason == kReasonTopOut) {
    m.reason = self_won ? EvMatchOver::Reason::OppTopOut : EvMatchOver::Reason::SelfTopOut;
  } else if (w.reason == kReasonForfeit) {
    m.reason = self_won ? EvMatchOver::Reason::OppForfeit : EvMatchOver::Reason::SelfForfeit;
  } else if (w.reason == kReasonDisconnect) {
    m.reason = EvMatchOver::Reason::Disconnect;
  } else {
    m.reason = EvMatchOver::Reason::Disconnect;
  }
  return m;
}

}  // namespace tp
