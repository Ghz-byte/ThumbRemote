#pragma once
#include "hid_keyboard_gate.h"

struct HidLink {
  uint32_t generation=0;
  bool connected=false, subscribed=false, encrypted=false, reportMode=true, suspended=false;
  bool ready() const { return connected && subscribed && encrypted && reportMode && !suspended; }
};
class HidKeyboardTransport {
 public:
  void begin();
  void poll();
  HidLink state() const;
  bool send(const hid_probe::Report& report, uint32_t generation);
  void cancelPending();
};
