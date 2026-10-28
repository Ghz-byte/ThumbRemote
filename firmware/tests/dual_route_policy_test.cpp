#include "../dual_remote/dual_route_policy.h"
#include <cstdio>
using namespace dual_remote;
int checks=0,failures=0;
#define CHECK(c) do {++checks;if(!(c)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#c);}} while(0)
int main(){
  HidModes modes;CHECK(modes.keyboardReady(true,true));CHECK(modes.digitizerReady(true,true));
  modes.digitizerMode=false;CHECK(modes.keyboardReady(true,true));CHECK(!modes.digitizerReady(true,true));
  modes.digitizerMode=true;modes.digitizerSuspended=true;CHECK(modes.keyboardReady(true,true));CHECK(!modes.digitizerReady(true,true));
  modes.digitizerSuspended=false;modes.keyboardMode=false;CHECK(!modes.keyboardReady(true,true));CHECK(modes.digitizerReady(true,true));
  modes.keyboardMode=true;modes.keyboardSuspended=true;CHECK(!modes.keyboardReady(true,true));CHECK(modes.digitizerReady(true,true));
  Profile profile; PeerIdentity app; app.type=0; app.address={{1,2,3,4,5,6}};
  // Reconnect regression: OS HID may own the ACL before the App starts scanning.
  LinkSnapshot discovery;
  CHECK(shouldAdvertise(discovery,profile));
  discovery.connected=true;discovery.peer=app;discovery.hidReady=true;
  CHECK(!shouldAdvertise(discovery,profile));
  Profile enrolled;enrolled.status=ProfileStatus::Valid;enrolled.appPeer=app;
  CHECK(!shouldAdvertise(discovery,enrolled));
  discovery.identityReady=discovery.encrypted=discovery.bonded=true;
  CHECK(shouldAdvertise(discovery,enrolled));
  discovery.appSubscribed=discovery.appClaimed=true;discovery.appSession=55;
  CHECK(shouldAdvertise(discovery,enrolled)); // Do not depend on receiving unsubscribe on App close.
  discovery.appSession=0;CHECK(shouldAdvertise(discovery,enrolled));
  discovery.peer.address[0]=9;CHECK(!shouldAdvertise(discovery,enrolled));
  discovery.peer=app;discovery.encrypted=false;CHECK(!shouldAdvertise(discovery,enrolled));
  discovery.encrypted=true;discovery.bonded=false;CHECK(!shouldAdvertise(discovery,enrolled));
  discovery.bonded=true;discovery.identityReady=false;CHECK(!shouldAdvertise(discovery,enrolled));
  discovery.identityReady=true;enrolled.status=ProfileStatus::Fault;CHECK(!shouldAdvertise(discovery,enrolled));
  discovery.connected=false;CHECK(shouldAdvertise(discovery,enrolled));
  LinkSnapshot s; s.handle=1;s.connectionEpoch=1;s.connected=true;s.identityReady=true;s.encrypted=true;s.bonded=true;s.peer=app;s.hidReady=true;
  RoutePolicy p; p.update(s,profile); CHECK(p.route()==Route::AppWaiting); CHECK(!p.needsEnrollment());
  s.appClaimed=true;s.appSubscribed=true;s.appSession=42;s.appEpoch=1;
  p.update(s,profile);CHECK(p.needsEnrollment());CHECK(p.route()!=Route::AppReady);
  profile.status=ProfileStatus::Valid;profile.appPeer=app;p.update(s,profile);
  CHECK(p.route()==Route::AppReady);auto gen=p.generation();
  s.hidEpoch++;s.hidReady=false;p.update(s,profile);CHECK(p.generation()==gen);
  s.appEpoch++;p.update(s,profile);CHECK(p.generation()!=gen);
  s.appSubscribed=false;s.appSession=0;p.update(s,profile);CHECK(p.route()==Route::AppWaiting);
  s.appClaimed=false; s.hidReady=true;p.update(s,profile);CHECK(p.route()==Route::AppWaiting);
  s.connected=false;s.connectionEpoch++;p.update(s,profile);CHECK(p.route()==Route::None);
  s.connected=true;s.connectionEpoch++;s.peer.address[0]=9;p.update(s,profile);CHECK(p.route()==Route::HidReady);
  gen=p.generation();s.digitizerReady=true;++s.digitizerEpoch;p.update(s,profile);
  CHECK(p.route()==Route::HidReady);CHECK(p.generation()!=gen);
  gen=p.generation();s.digitizerReady=false;++s.digitizerEpoch;p.update(s,profile);
  CHECK(p.generation()!=gen);
  s.appClaimed=true;s.appSubscribed=false;p.update(s,profile);CHECK(p.route()==Route::AppWaiting);
  // A fast subscribe/unsubscribe edge remains latched by the host in the snapshot.
  s.appSubscribed=true;p.update(s,profile);s.appSubscribed=false;p.update(s,profile);CHECK(p.route()!=Route::HidReady);
  s.appSubscribed=true;s.appSession=43;p.update(s,profile);CHECK(p.route()==Route::AppWaiting);CHECK(!p.needsEnrollment());
  s.encrypted=false;p.update(s,profile);CHECK(p.route()==Route::None);
  s.encrypted=true;s.bonded=false;p.update(s,profile);CHECK(p.route()==Route::None);
  s.bonded=true;s.identityReady=false;p.update(s,profile);CHECK(p.route()==Route::None);
  s.identityReady=true;profile.status=ProfileStatus::Fault;p.update(s,profile);CHECK(p.route()==Route::Blocked);
  profile.status=ProfileStatus::Valid; s.peer=app;s.appSubscribed=true;s.appSession=44;p.update(s,profile);CHECK(p.route()==Route::AppReady);
  RoutePolicy reboot;reboot.update(s,profile);CHECK(reboot.route()==Route::AppReady);
  s.appSubscribed=false;s.appSession=0;reboot.update(s,profile);CHECK(reboot.route()==Route::AppWaiting);
  s.connectionEpoch++;s.peer.address[0]=8;s.appClaimed=false;reboot.update(s,profile);CHECK(reboot.route()==Route::HidReady);
  std::printf("Dual route: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
