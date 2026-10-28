#include "../hid_probe/hid_direct_control.h"
#include <cstdio>
int checks=0, failures=0;
#define CHECK(c) do { ++checks; if (!(c)) {++failures; std::printf("FAIL %d: %s\n",__LINE__,#c);} } while(0)
using namespace hid_probe;
using touch::QueryState;
struct Finger {
  DirectControl control;
  Report out{};
  touch::Report point;
  bool at(uint32_t now,uint16_t x=200,uint16_t y=100,uint8_t mask=1) {
    point.mask=mask;
    for(auto& p : point.points) {p.x=x; p.y=y;}
    return control.observe(QueryState::Packet,&point,now,out);
  }
  bool idle(uint32_t now) {return control.observe(QueryState::Idle,nullptr,now,out);}
  bool hold(uint32_t now) {return control.observe(QueryState::Holding,nullptr,now,out);}
  void connect(uint32_t now=0,uint32_t generation=1) {
    control.update(true,generation,true,now);
    CHECK(control.tick(now,out)); CHECK(out==Report{});
  }
};
int main() {
  Finger f;
  CHECK(!f.control.enabled()); CHECK(!f.idle(0)); CHECK(!f.control.enabled());
  f.connect(); CHECK(!f.control.enabled());
  f.at(10); f.at(100,80); CHECK(!f.idle(200));
  CHECK(f.control.enabled()); // The connection-spanning contact only enables; never sends.
  f.at(300); f.at(400,80); CHECK(f.idle(500)); CHECK(f.out[2]==0x51);
  CHECK(!f.control.tick(539,f.out)); CHECK(f.control.tick(540,f.out)); CHECK(f.out==Report{});
  f.at(550,80); f.at(600,200); CHECK(!f.idle(700)); // Cooldown drops, not queued.
  f.at(1100,80); f.at(1200,200); CHECK(f.idle(1300)); CHECK(f.out[2]==0x52);
  CHECK(f.control.tick(1340,f.out)); CHECK(f.out==Report{});
  f.control.tick(90000,f.out); CHECK(f.control.enabled());
  f.at(90010); f.at(90100,80); CHECK(f.idle(90200)); CHECK(f.out[2]==0x51);
  CHECK(f.control.tick(90240,f.out));
  // A hold is no longer an on/off switch, nor a source of keyboard input.
  f.at(91000); f.hold(95000); CHECK(!f.idle(95100)); CHECK(f.control.enabled());
  f.at(96000); f.at(96100,80); CHECK(f.idle(96200));
  CHECK(f.control.tick(96240,f.out));
  // Both physical disconnect and generation-only readiness changes discard half gestures.
  f.at(97000); f.control.update(false,2,true,97100); CHECK(!f.control.enabled());
  f.at(97200,80); CHECK(!f.idle(97300));
  f.control.update(true,3,true,97400); CHECK(f.control.tick(97400,f.out));
  f.at(97500); f.at(97600,80); CHECK(!f.idle(97700)); CHECK(f.control.enabled());
  f.at(98000); f.control.update(true,4,true,98100); CHECK(!f.control.enabled());
  CHECK(f.control.tick(98100,f.out)); CHECK(f.out==Report{});
  f.at(98200,80); CHECK(!f.idle(98300)); CHECK(f.control.enabled());
  f.at(98400); f.at(98500,80); CHECK(f.idle(98600));
  // A health fault while a key is held rebuilds neutral and requires release on recovery.
  f.control.update(true,4,false,98610); CHECK(!f.control.enabled());
  CHECK(f.control.tick(98610,f.out)); CHECK(f.out==Report{});
  CHECK(!f.idle(99000)); CHECK(!f.control.enabled());
  f.control.update(true,4,true,99100); CHECK(f.control.tick(99100,f.out));
  f.hold(99110); f.at(99200); f.at(99300,80);
  CHECK(!f.idle(99400)); CHECK(f.control.enabled());
  f.at(100000); f.at(100100,80); CHECK(f.idle(100200));
  f.control.cancel(100201); CHECK(!f.control.enabled());
  CHECK(f.control.tick(100201,f.out)); CHECK(f.out==Report{});
  f.at(100300,80); f.at(100400,200); CHECK(!f.idle(100500));
  CHECK(f.control.enabled());
  // Unchanged short/ambiguous/multiple/changed-point filters cannot emit navigation.
  f.at(101000); f.at(101100,180); CHECK(!f.idle(101200));
  f.at(102000); f.at(102100,100,200); CHECK(!f.idle(102200));
  f.at(103000); f.at(103100,200,200); CHECK(!f.idle(103200)); // Left/right ignored.
  f.at(104000); f.at(104100,80,100,3); CHECK(!f.idle(104200));
  f.at(105000); f.at(105100,80,100,2); CHECK(!f.idle(105200));
  Finger wrap;
  wrap.connect(0xfffffff0u); CHECK(!wrap.idle(0xfffffff1u)); CHECK(wrap.control.enabled());
  wrap.at(0xfffffffeu); wrap.at(1,80); CHECK(wrap.idle(10)); CHECK(wrap.out[2]==0x51);
  CHECK(wrap.control.tick(50,wrap.out)); CHECK(wrap.out==Report{});
  wrap.control.tick(0x10000u,wrap.out); CHECK(wrap.control.enabled());
  // Page fallback must map actual swipes, keep neutral release and survive reconnect.
  DirectControl page(Mode::Page); Report output{};
  touch::Report p; p.mask=1; p.points[0].x=200; p.points[0].y=100;
  page.update(true,1,true,0); CHECK(page.tick(0,output)); CHECK(output==Report{});
  CHECK(!page.observe(QueryState::Idle,nullptr,1,output)); CHECK(page.enabled());
  CHECK(!page.observe(QueryState::Packet,&p,10,output));
  p.points[0].x=80; CHECK(!page.observe(QueryState::Packet,&p,100,output));
  CHECK(page.observe(QueryState::Idle,nullptr,200,output)); CHECK(output[2]==0x4e);
  CHECK(page.tick(240,output)); CHECK(output==Report{});
  page.update(false,2,true,300); page.update(true,3,true,400);
  CHECK(page.tick(400,output)); CHECK(!page.observe(QueryState::Idle,nullptr,401,output));
  CHECK(!page.observe(QueryState::Packet,&p,500,output));
  p.points[0].x=200; CHECK(!page.observe(QueryState::Packet,&p,600,output));
  CHECK(page.observe(QueryState::Idle,nullptr,700,output)); CHECK(output[2]==0x4b);
  page.cancel(701); CHECK(page.tick(701,output)); CHECK(output==Report{});
  std::printf("HID direct: %d checks, %d failures\n",checks,failures);
  return failures?1:0;
}
