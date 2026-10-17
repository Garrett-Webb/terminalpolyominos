// Optional localhost smoke vs tpoly_battle_server --insecure.
// Skips (exit 0) if TP_BATTLE_SMOKE is unset or connect fails.

#include "battle/ProtocolEvents.hpp"
#include "net/InsecureMediatorTransport.hpp"
#include "test_assert.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

namespace {

bool wait_for(tp::InsecureMediatorTransport& t, const std::function<bool(const tp::InboundEvent&)>& pred,
              int timeout_ms = 2000) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    std::vector<tp::InboundEvent> evs;
    t.poll(evs);
    for (const auto& ev : evs) {
      if (pred(ev)) {
        return true;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return false;
}

}  // namespace

int main() {
  if (!std::getenv("TP_BATTLE_SMOKE")) {
    std::printf("tp_battle_insecure_smoke: skipped (set TP_BATTLE_SMOKE=1)\n");
    return 0;
  }

  const char* host = std::getenv("TP_BATTLE_HOST");
  const char* port_s = std::getenv("TP_BATTLE_PORT");
  const std::string h = host && host[0] ? host : "127.0.0.1";
  std::uint16_t port = 9000;
  if (port_s && port_s[0]) {
    port = static_cast<std::uint16_t>(std::atoi(port_s));
  }

  tp::InsecureMediatorTransport a(h, port);
  tp::InsecureMediatorTransport b(h, port);
  if (!a.connect() || !b.connect()) {
    std::printf("tp_battle_insecure_smoke: skipped (no server at %s:%u)\n", h.c_str(), port);
    return 0;
  }

  a.send(tp::EvConsent{});
  std::string token;
  TP_CHECK(wait_for(a, [&](const tp::InboundEvent& ev) {
    if (const auto* t = std::get_if<tp::EvTokenShow>(&ev)) {
      token = t->token;
      return true;
    }
    return false;
  }));
  TP_CHECK(!token.empty());

  b.send(tp::EvConsent{});
  TP_CHECK(wait_for(b, [](const tp::InboundEvent& ev) {
    return std::holds_alternative<tp::EvTokenShow>(ev);
  }));

  tp::EvJoinFriend join;
  join.token = token;
  b.send(join);

  bool a_start = false;
  bool b_start = false;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (std::chrono::steady_clock::now() < deadline && !(a_start && b_start)) {
    std::vector<tp::InboundEvent> evs;
    a.poll(evs);
    for (const auto& ev : evs) {
      if (std::holds_alternative<tp::EvMatchStart>(ev)) {
        a_start = true;
      }
    }
    evs.clear();
    b.poll(evs);
    for (const auto& ev : evs) {
      if (std::holds_alternative<tp::EvMatchStart>(ev)) {
        b_start = true;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  TP_CHECK(a_start && b_start);

  a.close();
  b.close();
  return tp::test::summary("tp_battle_insecure_smoke");
}
