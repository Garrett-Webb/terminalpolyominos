#include "battle/ProtocolCodec.hpp"
#include "test_assert.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <string>
#include <variant>
#include <vector>

namespace {

std::vector<std::uint8_t> read_file(const char* path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return {};
  }
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),
                                   std::istreambuf_iterator<char>());
}

void test_hello_golden() {
  tp::WireHello h;
  h.client_ver = 1;
  h.proto_ver = 1;
  h.build_id = 0x2A;
  const auto bytes = tp::encode_hello(h);
  const std::array<std::uint8_t, 12> expected = {
      0x00, 0x0A, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2A};
  TP_CHECK(bytes.size() == expected.size());
  TP_CHECK(std::memcmp(bytes.data(), expected.data(), expected.size()) == 0);

  // Prefer in-tree fixture; fall back to sibling server repo path.
  auto disk = read_file("tests/testdata/frames/hello.bin");
  if (disk.empty()) {
    disk = read_file("../tpoly_battle_server/testdata/frames/hello.bin");
  }
  if (!disk.empty()) {
    TP_CHECK(disk.size() == expected.size());
    TP_CHECK(std::memcmp(disk.data(), expected.data(), expected.size()) == 0);
  }
}

void test_frame_fail_closed() {
  std::vector<std::uint8_t> too_big = {0x10, 0x01, 0x01, 0x00};  // body_len = 4097
  tp::CodecError err = tp::CodecError::Ok;
  TP_CHECK(!tp::try_decode_frame(too_big, &err));
  TP_CHECK(err == tp::CodecError::TooLarge);

  std::vector<std::uint8_t> flags = {0x00, 0x02, 0x03, 0x01};  // flags nonzero
  err = tp::CodecError::Ok;
  TP_CHECK(!tp::try_decode_frame(flags, &err));
  TP_CHECK(err == tp::CodecError::NonzeroFlags);

  tp::Frame unknown;
  unknown.msg_type = 0xFE;
  err = tp::CodecError::Ok;
  TP_CHECK(!tp::decode_inbound(unknown, &err));
  TP_CHECK(err == tp::CodecError::UnknownType);
}

void test_consent_roundtrip() {
  auto bytes = tp::encode_outbound(tp::EvConsent{});
  TP_CHECK(!bytes.empty());
  tp::CodecError err = tp::CodecError::Ok;
  auto frame = tp::try_decode_frame(bytes, &err);
  TP_CHECK(frame.has_value());
  TP_CHECK(frame->msg_type == tp::kMsgConsent);
  TP_CHECK(frame->payload.empty());
  TP_CHECK(bytes.empty());
}

void test_match_start_roundtrip() {
  tp::Frame f;
  f.msg_type = tp::kMsgMatchStart;
  // match_id=99, seed_self=1, seed_opp=2, ready=3000
  f.payload = {0, 0, 0, 0, 0, 0, 0, 99, 0, 0, 0, 0, 0, 0, 0, 1,
               0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0x0B, 0xB8};
  tp::CodecError err = tp::CodecError::Ok;
  auto inbound = tp::decode_inbound(f, &err);
  TP_CHECK(inbound.has_value());
  TP_CHECK(std::holds_alternative<tp::EvMatchStart>(*inbound));
  const auto& m = std::get<tp::EvMatchStart>(*inbound);
  TP_CHECK(m.match_id == 99);
  TP_CHECK(m.seed_self == 1);
  TP_CHECK(m.seed_opp == 2);
  TP_CHECK(m.ready_countdown_ms == 3000);
}

void test_clear_and_garbage() {
  tp::EvClearEvent c;
  c.seq = 9;
  c.lines_cleared = 4;
  c.combo = 2;
  c.b2b = true;
  auto bytes = tp::encode_outbound(c);
  tp::CodecError err = tp::CodecError::Ok;
  auto frame = tp::try_decode_frame(bytes, &err);
  TP_CHECK(frame.has_value());
  TP_CHECK(frame->msg_type == tp::kMsgClearEvent);
  TP_CHECK(frame->payload.size() == 12);
  TP_CHECK(frame->payload[4] == 4);
  TP_CHECK(frame->payload[7] == tp::kClearFlagB2b);

  tp::Frame g;
  g.msg_type = tp::kMsgGarbageApply;
  g.payload = {4, 3, 0, 0, 0, 7};
  auto inbound = tp::decode_inbound(g, &err);
  TP_CHECK(inbound.has_value());
  const auto& ga = std::get<tp::EvGarbageApply>(*inbound);
  TP_CHECK(ga.rows == 4);
  TP_CHECK(ga.hole_col == 3);
  TP_CHECK(ga.event_seq == 7);
}

void test_match_over_map() {
  tp::WireMatchOver w;
  w.reason = 0;       // top-out
  w.winner_side = 0;  // host
  w.rematch_deadline_ms = 15000;
  auto as_host = tp::map_match_over(w, true);
  TP_CHECK(as_host.reason == tp::EvMatchOver::Reason::OppTopOut);
  auto as_joiner = tp::map_match_over(w, false);
  TP_CHECK(as_joiner.reason == tp::EvMatchOver::Reason::SelfTopOut);
}

void test_token_show_lobby() {
  tp::Frame f;
  f.msg_type = tp::kMsgTokenShow;
  const std::string tok = "ABCD-EFGH";
  f.payload.push_back(static_cast<std::uint8_t>(tok.size()));
  f.payload.insert(f.payload.end(), tok.begin(), tok.end());
  for (int i = 0; i < 8; ++i) {
    f.payload.push_back(0);
  }
  auto inbound = tp::decode_inbound(f);
  TP_CHECK(inbound.has_value());
  TP_CHECK(std::get<tp::EvTokenShow>(*inbound).token == tok);

  tp::Frame lobby;
  lobby.msg_type = tp::kMsgLobbyState;
  lobby.payload = {1, 0};
  inbound = tp::decode_inbound(lobby);
  TP_CHECK(std::get<tp::EvLobbyState>(*inbound).peer_present);
  TP_CHECK(std::get<tp::EvLobbyState>(*inbound).role == 0);
}

void test_pose_ping_bye() {
  tp::EvPose p;
  p.seq = 1;
  p.x = 3;
  p.y = -2;
  p.r = 1;
  p.piece_id = 5;
  auto bytes = tp::encode_outbound(p);
  auto frame = tp::try_decode_frame(bytes);
  TP_CHECK(frame && frame->msg_type == tp::kMsgPose);

  // Inbound pose relay
  auto relay = tp::decode_inbound(*frame);
  TP_CHECK(relay && std::holds_alternative<tp::EvPoseRelay>(*relay));

  tp::EvPing ping;
  ping.echo = 42;
  bytes = tp::encode_outbound(ping);
  frame = tp::try_decode_frame(bytes);
  TP_CHECK(frame && frame->payload.size() == 8);

  bytes = tp::encode_outbound(tp::EvBye{});
  frame = tp::try_decode_frame(bytes);
  TP_CHECK(frame && frame->msg_type == tp::kMsgBye);
}

}  // namespace

int main() {
  test_hello_golden();
  test_frame_fail_closed();
  test_consent_roundtrip();
  test_match_start_roundtrip();
  test_clear_and_garbage();
  test_match_over_map();
  test_token_show_lobby();
  test_pose_ping_bye();
  return tp::test::summary("tp_battle_codec_tests");
}
