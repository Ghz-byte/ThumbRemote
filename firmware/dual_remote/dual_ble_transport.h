#pragma once
#include "dual_touch_control.h"
#include "dual_dispatch_gate.h"
#include "dual_profile_store.h"
namespace dual_remote {
struct TransportSnapshot {
  LinkSnapshot link;
  Profile profile;
  Route route=Route::None;
  uint32_t generation=0;
  bool advertising=false;
};
const char* routeName(Route route);
class DualBleTransport {
 public:
  void begin();
  void poll();
  TransportSnapshot snapshot() const;
  LinkSnapshot state() const{return snapshot().link;}
  Profile profile() const{return snapshot().profile;}
  Route route() const{return snapshot().route;}
  uint32_t generation() const{return snapshot().generation;}
  bool send(const Action& action);
  void cancelInput();
};
}
