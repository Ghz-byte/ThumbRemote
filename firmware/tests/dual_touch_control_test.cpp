#include "../dual_remote/dual_touch_control.h"
#include <cstdio>
using namespace dual_remote;
int checks=0,failures=0;
#define CHECK(c) do {++checks;if(!(c)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#c);}} while(0)
struct Finger {
  TouchControl c;Action out;touch::Report r;
  bool point(uint32_t now,uint16_t x=200,uint16_t y=100){r.mask=1;r.points[0].x=x;r.points[0].y=y;return c.observe(touch::QueryState::Packet,&r,now,out);}
  bool idle(uint32_t now){return c.observe(touch::QueryState::Idle,nullptr,now,out);}
};
int main(){
  Finger taps;taps.c.update(Route::AppReady,1,true,0);CHECK(!taps.idle(0));
  CHECK(!taps.point(10));CHECK(taps.idle(100));CHECK(taps.out.tap);
  CHECK(!taps.point(120));CHECK(taps.idle(200));CHECK(taps.out.tap);
  CHECK(!taps.point(300));CHECK(!taps.idle(700));
  CHECK(!taps.point(800));CHECK(!taps.point(820,250));CHECK(!taps.point(840));CHECK(!taps.idle(860));
  taps.c.update(Route::HidReady,2,true,1000);CHECK(taps.c.tick(1000,taps.out));CHECK(!taps.idle(1001));
  CHECK(!taps.point(1010));CHECK(taps.idle(1100));CHECK(taps.out.tap && taps.out.route==Route::HidReady);
  taps.c.update(Route::AppReady,3,true,1200);CHECK(!taps.idle(1201));
  CHECK(!taps.point(1210));taps.c.cancel(1220);CHECK(!taps.idle(1230));
  CHECK(!taps.point(1240));CHECK(taps.idle(1300));CHECK(taps.out.tap);
  Finger transition;transition.c.update(Route::HidReady,10,true,0);CHECK(transition.c.tick(0,transition.out));
  CHECK(!transition.idle(1));CHECK(!transition.point(10));
  transition.c.update(Route::HidReady,11,true,20);CHECK(transition.c.tick(20,transition.out));
  CHECK(!transition.idle(30));CHECK(!transition.point(40));CHECK(transition.idle(50));CHECK(transition.out.tap);
  Finger f;f.c.update(Route::AppReady,1,true,0);
  f.point(1);f.point(2,80);CHECK(!f.idle(3));
  f.point(10);f.point(20,80);CHECK(f.idle(30));CHECK(f.out.route==Route::AppReady && f.out.direction==touch::Direction::Up);
  f.point(40,80);f.point(50,200);CHECK(f.idle(60));CHECK(f.out.direction==touch::Direction::Down);
  f.point(70);f.point(80,200,220);CHECK(f.idle(90));CHECK(f.out.direction==touch::Direction::Left);
  f.point(100,200,220);f.point(110);CHECK(f.idle(120));CHECK(f.out.direction==touch::Direction::Right);
  f.point(130);f.c.update(Route::HidReady,2,true,140);CHECK(f.c.tick(140,f.out));CHECK(f.out.report==hid_probe::Report{});
  f.point(150,80);CHECK(!f.idle(160));
  f.point(170);CHECK(!f.c.observe(touch::QueryState::Holding,nullptr,175,f.out));f.point(180,80);CHECK(f.idle(190));CHECK(f.out.report[2]==0x51);
  CHECK(!f.c.tick(229,f.out));CHECK(f.c.tick(230,f.out));CHECK(f.out.report==hid_probe::Report{});
  f.point(240,80);f.point(250);CHECK(f.idle(260));CHECK(f.out.report[2]==0x52);CHECK(f.c.tick(300,f.out));
  f.point(800,80);f.point(810);CHECK(f.idle(820));CHECK(f.out.report[2]==0x52);CHECK(f.c.tick(860,f.out));
  f.point(1400);f.point(1410,200,220);CHECK(!f.idle(1420));
  f.point(90000);f.point(90010,80);CHECK(f.idle(90020));CHECK(f.c.tick(90060,f.out));
  f.point(91000);f.c.update(Route::HidReady,2,false,91001);CHECK(f.c.tick(91001,f.out));
  CHECK(!f.idle(91002));f.c.update(Route::HidReady,2,true,91003);CHECK(f.c.tick(91003,f.out));
  f.point(91004,80);CHECK(!f.idle(91005));
  f.c.update(Route::AppReady,3,true,92000);f.point(92001);f.point(92002,80);CHECK(!f.idle(92003));
  f.point(93000);f.point(93001,80);CHECK(f.idle(93002));
  f.c.cancel(93003);f.point(94000);f.point(94001,80);CHECK(!f.idle(94002));
  std::printf("Dual touch: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
