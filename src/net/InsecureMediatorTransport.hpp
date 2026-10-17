#pragma once

#include "battle/MediatorTransport.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tp {

// Plain TCP mediator (dev / dogfood only). Built when TP_BATTLE_INSECURE=1.
class InsecureMediatorTransport final : public MediatorTransport {
 public:
  explicit InsecureMediatorTransport(std::string host, std::uint16_t port);
  ~InsecureMediatorTransport() override;

  bool connect() override;
  void close() override;
  void send(const OutboundEvent& ev) override;
  void poll(std::vector<InboundEvent>& out) override;
  [[nodiscard]] bool alive() const override;
  [[nodiscard]] bool offline() const override { return false; }

 private:
  void pump_reads();
  bool write_all(const std::vector<std::uint8_t>& bytes);

  std::string host_;
  std::uint16_t port_ = 9000;
  int fd_ = -1;
  bool alive_ = false;
  bool hello_done_ = false;
  std::vector<std::uint8_t> read_buf_;
  std::vector<InboundEvent> inbox_;
};

}  // namespace tp
