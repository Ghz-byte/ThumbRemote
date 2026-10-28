#pragma once
#include "gesture_recognizer.h"

namespace hid_probe {
struct InputResult {
  bool toggle = false;
  touch::GestureResult gesture;
};
class TouchControl {
 public:
  InputResult observe(touch::QueryState query, const touch::Report* report, uint32_t now) {
    InputResult result;
    if (query==touch::QueryState::Idle ||
        (query==touch::QueryState::Packet && report && report->mask==0)) {
      const bool longContact=tracking_ && uint32_t(now-startedAt_)>=3000;
      result.toggle=longContact && eligible_ && !blocked_;
      const auto gesture=recognizer_.observe(query,report);
      // A long contact belongs exclusively to the switch, even if it moved.
      if (!longContact) result.gesture=gesture;
      tracking_=false; blocked_=false; eligible_=false;
      return result;
    }
    if (query==touch::QueryState::Holding) {
      if (!tracking_) blocked_=true;
      recognizer_.observe(query,report);
      return result;
    }
    if (!report || (report->mask!=1 && report->mask!=2 && report->mask!=4) ||
        (tracking_ && report->mask!=pointMask_)) {
      cancel(); return result;
    }
    if (blocked_) return result;
    const auto& point=report->points[report->mask==1?0:(report->mask==2?1:2)];
    if (!tracking_) {
      tracking_=true; eligible_=true; startedAt_=now;
      start_=point; pointMask_=report->mask;
    }
    const int32_t dx=static_cast<int32_t>(point.x)-start_.x;
    const int32_t dy=static_cast<int32_t>(point.y)-start_.y;
    if (dx < -12 || dx > 12 || dy < -12 || dy > 12) eligible_=false;
    recognizer_.observe(query,report);
    return result;
  }
  void cancel() {
    tracking_=false; eligible_=false; blocked_=true;
    recognizer_.cancel();
  }
 private:
  touch::GestureRecognizer recognizer_;
  touch::Point start_;
  uint32_t startedAt_=0;
  uint8_t pointMask_=0;
  bool tracking_=false, eligible_=false, blocked_=true;
};
}
