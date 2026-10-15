#pragma once

#include "battle/ProtocolEvents.hpp"

#include <memory>
#include <vector>

namespace tp {

class MediatorTransport {
 public:
  virtual ~MediatorTransport() = default;

  virtual bool connect() = 0;
  virtual void close() = 0;
  virtual void send(const OutboundEvent& ev) = 0;
  virtual void poll(std::vector<InboundEvent>& out) = 0;
  [[nodiscard]] virtual bool alive() const = 0;
  [[nodiscard]] virtual bool offline() const { return true; }

  // Hot-seat: advance in-process opponent simulation. Online: no-op.
  virtual void tick(int /*elapsed_ms*/) {}
};

}  // namespace tp
