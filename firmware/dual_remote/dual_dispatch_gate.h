#pragma once
#include "dual_route_policy.h"
#include "ble_event_protocol.h"
#include "hid_keyboard_gate.h"
#include "dual_digitizer.h"
namespace dual_remote {
template<class Mutex> class DispatchLockGuard {
 public:
  explicit DispatchLockGuard(Mutex& mutex):mutex_(mutex){mutex_.lock();}
  ~DispatchLockGuard(){mutex_.unlock();}
 private:
  Mutex& mutex_;
};
template<class Mutex,class Pending,class Dispatch>
bool runSerializedDispatch(Mutex& mutex,Pending pending,Dispatch dispatch){
  DispatchLockGuard<Mutex> guard(mutex);
  if(pending())return false;
  dispatch();return true;
}
template<class Mutex,class Cancel>
void runSerializedCancel(Mutex& mutex,Cancel cancel){DispatchLockGuard<Mutex> guard(mutex);cancel();}
inline remote_ble::Packet encodeAppAction(uint32_t session,touch::Direction direction,bool tap) {
  auto packet=remote_ble::encodeDirection(session,0,direction);
  if(tap)packet[9]=5;
  return packet;
}
struct TxRequest {
  Route route=Route::None;
  uint16_t handle=0xffff;
  uint32_t connectionEpoch=0,routeGeneration=0,channelEpoch=0;
  bool readyFrame=false;
  bool digitizer=false;
  remote_ble::Packet app{};
  hid_probe::Report hid{};
  DigitizerReport pen{};
};
struct DispatchGate {
  static bool accepts(const TxRequest& r,const LinkSnapshot& s,Route route,uint32_t generation) {
    if(!s.connected || !s.encrypted || !s.bonded || !s.identityReady ||
       r.handle!=s.handle || r.connectionEpoch!=s.connectionEpoch)return false;
    if(r.route==Route::HidReady) {
      if(r.digitizer) {
        if(!s.digitizerReady || r.channelEpoch!=s.digitizerEpoch)return false;
        if(r.pen==digitizerRelease())return true;
        return r.pen==digitizerPress() && !s.appClaimed && route==Route::HidReady && r.routeGeneration==generation;
      }
      if(!s.hidReady || r.channelEpoch!=s.hidEpoch)return false;
      if(r.hid==hid_probe::Report{})return true; // Neutral release belongs to this physical connection.
      return !s.appClaimed && route==Route::HidReady && r.routeGeneration==generation;
    }
    if(r.route!=Route::AppReady || route!=Route::AppReady || r.routeGeneration!=generation ||
       !s.appSubscribed || !s.appSession || r.channelEpoch!=s.appEpoch)return false;
    uint32_t session=0;for(unsigned i=0;i<4;++i)session|=uint32_t(r.app[1+i])<<(8*i);
    return session==s.appSession;
  }
};
// Bounded short-lived input frames. READY supersedes all input and stays first.
class AppDelivery {
 public:
  bool offer(const TxRequest& request,uint32_t now) {
    if(request.readyFrame){count_=0;head_=0;}
    else if((count_ && requests_[head_].readyFrame) || count_==kCapacity)return false;
    const size_t index=(head_+count_)%kCapacity;
    requests_[index]=request;offeredAt_[index]=now;++count_;return true;
  }
  bool take(uint32_t now,TxRequest& out) {
    while(count_){
      const auto request=requests_[head_];const auto offered=offeredAt_[head_];
      head_=(head_+1)%kCapacity;--count_;
      if(!request.readyFrame && uint32_t(now-offered)>100)continue;
      out=request;return true;
    }
    return false;
  }
  bool hasPending() const{return count_!=0;}
  void cancel(){count_=0;head_=0;}
  void cancelDirections(){if(count_ && !requests_[head_].readyFrame)cancel();}
 private:
  static constexpr size_t kCapacity=4;
  TxRequest requests_[kCapacity]{};
  uint32_t offeredAt_[kCapacity]{};
  size_t head_=0,count_=0;
};
}
