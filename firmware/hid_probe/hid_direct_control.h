#pragma once
#include "hid_keyboard_gate.h"
namespace hid_probe {
class DirectControl {
 public:
  explicit DirectControl(Mode mode = Mode::Arrow) { gate_.mode(mode); }
  void update(bool ready, uint32_t generation, bool healthy, uint32_t now) {
    if (ready==ready_ && generation==generation_ && healthy==healthy_) return;
    ready_=ready; generation_=generation; healthy_=healthy;
    gate_.update(ready,generation,now);
    cancel(now);
  }
  bool observe(touch::QueryState query, const touch::Report* report, uint32_t now, Report& output) {
    if (!ready_ || !healthy_) return false;
    if (!gate_.armed()) {
      // A release can enable future input, never complete a pre-connection swipe.
      recognizer_.cancel();
      const bool released=query==touch::QueryState::Idle ||
        (query==touch::QueryState::Packet && report && report->mask==0);
      if (released && gate_.armPersistent(now)) recognizer_.observe(query,report);
      return false;
    }
    const auto gesture=recognizer_.observe(query,report);
    return gesture.outcome==touch::GestureOutcome::Accepted &&
      gate_.press(gesture.direction,generation_,now,output);
  }
  bool tick(uint32_t now, Report& output) { return gate_.tick(now,output); }
  void cancel(uint32_t now) { gate_.disarm(now); recognizer_.cancel(); }
  bool enabled() const { return gate_.armed(); }
  Mode mode() const { return gate_.mode(); }
 private:
  Gate gate_;
  touch::GestureRecognizer recognizer_;
  uint32_t generation_=0;
  bool ready_=false, healthy_=false;
};
}
