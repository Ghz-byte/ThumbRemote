#pragma once
#include "dual_route_policy.h"
#include "dual_keyboard_gate.h"
namespace dual_remote {
struct Action {
  Route route=Route::None;
  touch::Direction direction=touch::Direction::None;
  bool tap=false;
  hid_probe::Report report{};
  uint32_t generation=0;
};
class TouchControl {
 public:
  void update(Route route,uint32_t generation,bool healthy,uint32_t now) {
    if(route==route_ && generation==generation_ && healthy==healthy_)return;
    route_=route;generation_=generation;healthy_=healthy;
    gate_.update(route==Route::HidReady,generation,now);cancel(now);
  }
  bool observe(touch::QueryState query,const touch::Report* report,uint32_t now,Action& out) {
    if(!healthy_ || (route_!=Route::AppReady && route_!=Route::HidReady))return false;
    if(!released_ || (route_==Route::HidReady && !gate_.armed())) {
      recognizer_.cancel();
      const bool release=query==touch::QueryState::Idle || (query==touch::QueryState::Packet && report && report->mask==0);
      if(release){released_=true;tapTracking_=false;if(route_==Route::HidReady)gate_.armPersistent(now);recognizer_.observe(query,report);}
      return false;
    }
    const bool release=query==touch::QueryState::Idle || (query==touch::QueryState::Packet && report && report->mask==0);
    if(query==touch::QueryState::Packet && report && report->mask!=0) {
      const auto mask=report->mask;
      if(mask!=1 && mask!=2 && mask!=4){tapEligible_=false;}
      else {
        const auto point=report->points[mask==1?0:mask==2?1:2];
        if(!tapTracking_){tapTracking_=true;tapEligible_=true;tapStart_=point;tapStarted_=now;tapMask_=mask;}
        else {
          const int32_t dx=int32_t(point.x)-tapStart_.x,dy=int32_t(point.y)-tapStart_.y;
          if(mask!=tapMask_ || dx>=40 || dx<=-40 || dy>=40 || dy<=-40)tapEligible_=false;
        }
      }
    }
    const auto gesture=recognizer_.observe(query,report);
    const bool tap=release && tapTracking_ && tapEligible_ && uint32_t(now-tapStarted_)<=350 &&
      gesture.outcome==touch::GestureOutcome::Rejected && gesture.reason==touch::RejectReason::TooShort;
    if(release)tapTracking_=false;
    if(tap){out=Action{};out.route=route_;out.generation=generation_;out.tap=true;return true;}
    if(gesture.outcome!=touch::GestureOutcome::Accepted)return false;
    out=Action{};out.route=route_;out.generation=generation_;out.direction=gesture.direction;
    return route_==Route::AppReady || gate_.press(gesture.direction,generation_,now,out.report);
  }
  bool tick(uint32_t now,Action& out) {
    out=Action{};out.route=Route::HidReady;out.generation=generation_;
    return gate_.tick(now,out.report);
  }
  void cancel(uint32_t now){released_=false;tapTracking_=false;recognizer_.cancel();gate_.disarm(now);}
 private:
  Route route_=Route::None;
  uint32_t generation_=0;
  bool healthy_=false,released_=false;
  bool tapTracking_=false,tapEligible_=false;
  uint32_t tapStarted_=0;
  uint8_t tapMask_=0;
  touch::Point tapStart_{};
  touch::GestureRecognizer recognizer_;
  KeyboardGate gate_;
};
}
