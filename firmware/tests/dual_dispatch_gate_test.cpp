#include "../dual_remote/dual_dispatch_gate.h"
#include <cstdio>
using namespace dual_remote;
int checks=0,failures=0;
#define CHECK(c) do {++checks;if(!(c)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#c);}} while(0)
int main(){
  CHECK(encodeAppAction(9,touch::Direction::None,true)[9]==5);
  CHECK(encodeAppAction(9,touch::Direction::Up,false)[9]==1);
  LinkSnapshot s;s.connected=s.encrypted=s.bonded=s.identityReady=s.hidReady=true;s.handle=1;s.connectionEpoch=1;s.hidEpoch=2;s.appEpoch=3;
  s.digitizerReady=true;s.digitizerEpoch=4;
  TxRequest pen;pen.route=Route::HidReady;pen.digitizer=true;pen.handle=1;pen.connectionEpoch=1;pen.channelEpoch=4;pen.routeGeneration=4;pen.pen=digitizerPress();
  CHECK(DispatchGate::accepts(pen,s,Route::HidReady,4));
  s.digitizerReady=false;CHECK(!DispatchGate::accepts(pen,s,Route::HidReady,4));s.digitizerReady=true;
  s.appClaimed=true;CHECK(!DispatchGate::accepts(pen,s,Route::HidReady,4));
  pen.pen=digitizerRelease();CHECK(DispatchGate::accepts(pen,s,Route::AppWaiting,5));
  pen.channelEpoch=3;CHECK(!DispatchGate::accepts(pen,s,Route::AppWaiting,5));pen.channelEpoch=4;
  pen.connectionEpoch=0;CHECK(!DispatchGate::accepts(pen,s,Route::AppWaiting,5));pen.connectionEpoch=1;
  s.appClaimed=false;
  TxRequest r;r.route=Route::HidReady;r.handle=1;r.connectionEpoch=1;r.routeGeneration=4;r.channelEpoch=2;r.hid[2]=0x51;
  CHECK(DispatchGate::accepts(r,s,Route::HidReady,4));
  s.appClaimed=true;CHECK(!DispatchGate::accepts(r,s,Route::HidReady,4));
  CHECK(!DispatchGate::accepts(r,s,Route::AppWaiting,5));
  r.hid={};CHECK(DispatchGate::accepts(r,s,Route::AppWaiting,5));
  r.connectionEpoch=0;CHECK(!DispatchGate::accepts(r,s,Route::AppWaiting,5));r.connectionEpoch=1;
  r.handle=2;CHECK(!DispatchGate::accepts(r,s,Route::AppWaiting,5));r.handle=1;
  s.encrypted=false;CHECK(!DispatchGate::accepts(r,s,Route::AppWaiting,5));s.encrypted=true;
  r.route=Route::AppReady;r.channelEpoch=3;r.routeGeneration=5;r.app=remote_ble::encodeDirection(9,1,touch::Direction::Up);
  s.appSession=9;s.appSubscribed=true;
  CHECK(DispatchGate::accepts(r,s,Route::AppReady,5));
  s.appSession=10;CHECK(!DispatchGate::accepts(r,s,Route::AppReady,5));s.appSession=9;
  s.appEpoch++;CHECK(!DispatchGate::accepts(r,s,Route::AppReady,5));s.appEpoch--;
  s.appSubscribed=false;CHECK(!DispatchGate::accepts(r,s,Route::AppReady,5));s.appSubscribed=true;
  CHECK(!DispatchGate::accepts(r,s,Route::AppWaiting,5));
  AppDelivery q;auto ready=r;ready.readyFrame=true;ready.app=remote_ble::encodeReady(9);
  CHECK(q.offer(ready,100));CHECK(!q.offer(r,101));TxRequest out;
  CHECK(q.take(2000,out));CHECK(out.readyFrame);
  CHECK(q.offer(r,2001));CHECK(!q.take(2102,out));
  CHECK(q.offer(r,2200));CHECK(q.offer(ready,2201));CHECK(q.take(2202,out));CHECK(out.readyFrame);
  CHECK(q.offer(r,2300));q.cancel();CHECK(!q.take(2301,out));
  CHECK(q.offer(ready,2400));q.cancelDirections();CHECK(q.take(2401,out));CHECK(out.readyFrame);
  CHECK(q.offer(r,2500));q.cancelDirections();CHECK(!q.take(2501,out));
  CHECK(q.offer(r,2600));auto tap=r;tap.app=encodeAppAction(9,touch::Direction::None,true);
  CHECK(q.offer(tap,2601));CHECK(q.take(2602,out));CHECK(out.app[9]==1);
  CHECK(q.take(2603,out));CHECK(out.app[9]==5);CHECK(!q.take(2604,out));
  CHECK(q.offer(r,2700));CHECK(q.offer(tap,2701));CHECK(q.offer(ready,2702));
  CHECK(q.take(2703,out));CHECK(out.readyFrame);CHECK(!q.take(2704,out));
  CHECK(q.offer(r,2800));CHECK(q.offer(tap,2801));CHECK(q.offer(tap,2802));CHECK(q.offer(tap,2803));
  CHECK(!q.offer(tap,2804));CHECK(!q.take(2904,out));CHECK(!q.hasPending());
  std::printf("Dual dispatch: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
