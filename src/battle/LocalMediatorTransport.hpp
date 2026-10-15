#pragma once

#include "battle/MediatorTransport.hpp"
#include "game/Game.hpp"

#include <cstdint>
#include <random>
#include <vector>

namespace tp {

// In-process mediator: friend/random immediately match; owns a gravity-only opponent Game.
class LocalMediatorTransport final : public MediatorTransport {
 public:
  LocalMediatorTransport();

  bool connect() override;
  void close() override;
  void send(const OutboundEvent& ev) override;
  void poll(std::vector<InboundEvent>& out) override;
  [[nodiscard]] bool alive() const override { return alive_; }
  [[nodiscard]] bool offline() const override { return true; }
  void tick(int elapsed_ms) override;

 private:
  enum class Stage : std::uint8_t {
    Idle,
    Lobby,
    Ready,
    Playing,
    Rematch,
    Closed,
  };

  void push(InboundEvent ev);
  void begin_match(std::uint64_t seed_self, std::uint64_t seed_opp, int countdown_ms);
  void on_clear_from_self(const EvClearEvent& clear);
  void check_opp_topout();
  [[nodiscard]] int random_hole();

  Stage stage_ = Stage::Idle;
  bool alive_ = false;
  bool consented_ = false;
  std::string token_ = "LOCAL-HOT";
  Game opp_;
  bool opp_alive_ = false;
  std::uint32_t event_seq_ = 1;
  std::mt19937 rng_{1};
  std::vector<InboundEvent> inbox_;
  bool self_accepted_rematch_ = false;
  int ready_left_ms_ = 0;
};

}  // namespace tp
