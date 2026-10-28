#pragma once
#include "dual_profile.h"
namespace dual_remote {
enum class Route { None, AppWaiting, AppReady, HidReady, Blocked };
struct HidModes {
  bool keyboardMode=true,keyboardSuspended=false,digitizerMode=true,digitizerSuspended=false;
  bool keyboardReady(bool base,bool subscribed) const{return base && subscribed && keyboardMode && !keyboardSuspended;}
  bool digitizerReady(bool base,bool subscribed) const{return base && subscribed && digitizerMode && !digitizerSuspended;}
};
struct LinkSnapshot {
  uint16_t handle=0xffff;
  uint32_t connectionEpoch=0,appEpoch=0,hidEpoch=0,digitizerEpoch=0;
  bool connected=false,identityReady=false,encrypted=false,bonded=false;
  bool appSubscribed=false,appClaimed=false,hidReady=false,digitizerReady=false;
  PeerIdentity peer;
  uint32_t appSession=0;
};
inline bool shouldAdvertise(const LinkSnapshot& s,const Profile& p){
  // App and system HID can share the same physical link. The App still scans
  // before attaching its own GATT client, including after an App-only stop.
  return !s.connected || (p.status==ProfileStatus::Valid && s.identityReady &&
    s.encrypted && s.bonded && validIdentity(s.peer) && s.peer==p.appPeer);
}
class RoutePolicy {
 public:
  void update(const LinkSnapshot& s,const Profile& p) {
    if(s.connectionEpoch!=last_.connectionEpoch || !s.connected)claimed_=false;
    claimed_=claimed_ || s.appClaimed || s.appSubscribed || s.appSession!=0;
    enrollment_=false;
    Route next=Route::None;
    if(p.status==ProfileStatus::Fault)next=Route::Blocked;
    else if(s.connected && s.identityReady && s.encrypted && s.bonded && validIdentity(s.peer)) {
      const bool appRequest=s.appSubscribed && s.appSession!=0;
      if(p.status==ProfileStatus::Missing) {
        next=Route::AppWaiting;enrollment_=appRequest;
      } else if(s.peer==p.appPeer || claimed_) {
        next=s.peer==p.appPeer && appRequest?Route::AppReady:Route::AppWaiting;
      } else if(s.hidReady)next=Route::HidReady;
    }
    if(next!=route_ || s.connectionEpoch!=last_.connectionEpoch || s.peer!=last_.peer ||
       s.identityReady!=last_.identityReady || s.encrypted!=last_.encrypted || s.bonded!=last_.bonded ||
       (next==Route::AppReady && (s.appEpoch!=last_.appEpoch || s.appSession!=last_.appSession)) ||
       (next==Route::HidReady && (s.hidEpoch!=last_.hidEpoch || s.digitizerEpoch!=last_.digitizerEpoch || s.digitizerReady!=last_.digitizerReady)))++generation_;
    route_=next;last_=s;
  }
  Route route() const{return route_;}
  uint32_t generation() const{return generation_;}
  bool needsEnrollment() const{return enrollment_;}
 private:
  LinkSnapshot last_;
  Route route_=Route::None;
  uint32_t generation_=0;
  bool claimed_=false,enrollment_=false;
};
}
