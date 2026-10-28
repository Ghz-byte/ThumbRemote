#pragma once
#include "ble_event_protocol.h"
class BleTransport {
 public:
  void begin();
  void poll();
  uint32_t generation() const;
  bool sendDirection(touch::Direction direction, uint32_t capturedGeneration);
 private:
  remote_ble::SendGate gate_;
};
