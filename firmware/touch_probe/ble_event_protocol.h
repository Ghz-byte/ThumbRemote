#pragma once
#include <array>
#include <stddef.h>
#include <stdint.h>
#include "gesture_recognizer.h"
namespace remote_ble {
using Packet = std::array<uint8_t, 10>;
inline void put32(Packet& packet, size_t offset, uint32_t value) {
  for (size_t i=0;i<4;++i) packet[offset+i]=static_cast<uint8_t>(value>>(8*i));
}
inline Packet encodeReady(uint32_t session) {
  Packet packet{}; packet[0]=1; put32(packet,1,session); return packet;
}
inline Packet encodeDirection(uint32_t session, uint32_t sequence, touch::Direction direction) {
  Packet packet=encodeReady(session); put32(packet,5,sequence);
  switch(direction) {
    case touch::Direction::Up: packet[9]=1; break;
    case touch::Direction::Down: packet[9]=2; break;
    case touch::Direction::Left: packet[9]=3; break;
    case touch::Direction::Right: packet[9]=4; break;
    default: break;
  }
  return packet;
}
inline bool decodeStart(const uint8_t* bytes, size_t length, uint32_t& session) {
  if (!bytes || length!=5 || bytes[0]!=1) return false;
  uint32_t value=0;
  for (size_t i=0;i<4;++i) value|=static_cast<uint32_t>(bytes[1+i])<<(8*i);
  if (!value) return false;
  session=value; return true;
}
class SendGate {
 public:
  void update(bool connected, bool subscribed, uint32_t session, uint32_t generation) {
    if (generation!=generation_) sequence_=0;
    generation_=generation; session_=session; ready_=connected && subscribed && session!=0;
  }
  bool ready() const { return ready_; }
  uint32_t generation() const { return generation_; }
  bool prepare(touch::Direction direction, uint32_t capturedGeneration, Packet& packet) {
    if (!ready_ || capturedGeneration!=generation_ || direction==touch::Direction::None) return false;
    packet=encodeDirection(session_,++sequence_,direction); return true;
  }
 private:
  bool ready_=false;
  uint32_t generation_=0, session_=0, sequence_=0;
};
}
