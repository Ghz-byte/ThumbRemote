#include "../hid_probe/hid_touch_control.h"
#include "../hid_probe/hid_keyboard_gate.h"
#include <cstdio>
int checks=0, failures=0;
#define CHECK(c) do { ++checks; if (!(c)) {++failures; std::printf("FAIL %d: %s\n",__LINE__,#c);} } while(0)
using namespace hid_probe;
using touch::QueryState;
struct Finger {
  TouchControl input;
  touch::Report report;
  Finger() { idle(0); }
  InputResult at(uint32_t now, uint16_t x=200, uint16_t y=100, uint8_t mask=1) {
    report.mask=mask;
    for(auto& p : report.points) { p.x=x; p.y=y; }
    return input.observe(QueryState::Packet,&report,now);
  }
  InputResult idle(uint32_t now) { return input.observe(QueryState::Idle,nullptr,now); }
  InputResult hold(uint32_t now) { return input.observe(QueryState::Holding,nullptr,now); }
};
int main() {
  // Removing the duration boundary or emitting before release must fail.
  Finger shortHold;
  CHECK(!shortHold.at(10).toggle); CHECK(!shortHold.hold(3009).toggle);
  CHECK(!shortHold.idle(3009).toggle);
  Finger exact;
  CHECK(!exact.at(10).toggle); CHECK(!exact.hold(3010).toggle);
  auto result=exact.idle(3010);
  CHECK(result.toggle); CHECK(result.gesture.outcome==touch::GestureOutcome::None);
  CHECK(!exact.idle(3011).toggle);
  // Each new contact can toggle once; packet mask=0 is also a release.
  exact.at(4000); exact.hold(8000);
  CHECK(exact.at(8001,200,100,0).toggle); CHECK(!exact.idle(8002).toggle);
  // A wrong drift threshold, resetting the origin or forgetting peak drift fails these.
  Finger jitter;
  jitter.at(0); jitter.at(1000,212,88); jitter.at(2000,188,112);
  CHECK(jitter.idle(3000).toggle);
  Finger moved;
  moved.at(0); moved.at(1000,213,100); moved.at(2000,200,100);
  CHECK(!moved.idle(3000).toggle);
  Finger movedY;
  movedY.at(0); movedY.at(1000,200,113); CHECK(!movedY.idle(3000).toggle);
  Finger lateMove;
  lateMove.at(0); lateMove.hold(3100); lateMove.at(3200,100,100);
  result=lateMove.idle(3300);
  CHECK(!result.toggle); CHECK(result.gesture.outcome==touch::GestureOutcome::None);
  // Existing short UP/DOWN mapping still works; a >=3s slow swipe must be consumed.
  Finger swipe;
  swipe.at(100); swipe.at(300,80,100); result=swipe.idle(500);
  CHECK(!result.toggle); CHECK(result.gesture.direction==touch::Direction::Up);
  CHECK(result.gesture.outcome==touch::GestureOutcome::Accepted);
  swipe.at(600,80,100); swipe.at(700,200,100); result=swipe.idle(800);
  CHECK(result.gesture.direction==touch::Direction::Down);
  Finger slow;
  slow.at(100); slow.at(1000,80,100); result=slow.idle(3100);
  CHECK(!result.toggle); CHECK(result.gesture.outcome==touch::GestureOutcome::None);
  // Reject half contacts, fault-spanning holds, multi-touch, changed finger and unknown packets.
  TouchControl startup;
  touch::Report p; p.mask=1; p.points[0].x=200; p.points[0].y=100;
  startup.observe(QueryState::Packet,&p,0);
  CHECK(!startup.observe(QueryState::Idle,nullptr,4000).toggle);
  Finger holding;
  holding.hold(10); holding.at(100); CHECK(!holding.idle(4000).toggle);
  Finger cancelled;
  cancelled.at(0); cancelled.input.cancel(); cancelled.at(1000);
  CHECK(!cancelled.idle(4000).toggle);
  cancelled.at(5000); CHECK(cancelled.idle(8000).toggle);
  for(uint8_t mask : {uint8_t(0),uint8_t(3),uint8_t(5),uint8_t(6),uint8_t(7)}) {
    if (!mask) continue;
    Finger multi;
    multi.at(0); multi.at(1000,200,100,mask); multi.at(2000);
    CHECK(!multi.idle(4000).toggle);
  }
  Finger changed;
  changed.at(0); changed.at(1000,200,100,2); CHECK(!changed.idle(4000).toggle);
  for(uint8_t mask : {uint8_t(2),uint8_t(4)}) {
    Finger alternate;
    alternate.at(0,200,100,mask); CHECK(alternate.idle(3000).toggle);
  }
  Finger unknown;
  unknown.at(0); unknown.input.observe(QueryState::Packet,nullptr,1000);
  unknown.at(2000); CHECK(!unknown.idle(4000).toggle);
  Finger wrapped;
  wrapped.at(0xfffffff0u); CHECK(wrapped.idle(0xba8u).toggle);
  // A physical session must survive >60s/wrap, but disconnect and serial arm still stop it.
  Gate gate; Report report{};
  CHECK(!gate.armPersistent(0));
  gate.update(true,1,0); CHECK(gate.tick(0,report));
  CHECK(gate.armPersistent(1)); CHECK(!gate.armPersistent(2));
  gate.tick(60001,report); CHECK(gate.armed());
  CHECK(gate.press(touch::Direction::Up,1,90001,report)); CHECK(report[2]==0x51);
  CHECK(gate.tick(90041,report)); CHECK(report==Report{});
  gate.disarm(90042); CHECK(!gate.armed()); CHECK(gate.tick(90042,report));
  CHECK(report==Report{}); CHECK(gate.arm(91000));
  gate.tick(151000,report); CHECK(!gate.armed());
  gate.tick(151001,report); CHECK(gate.armPersistent(151002));
  gate.update(false,2,151003); CHECK(!gate.armed()); CHECK(!gate.armPersistent(151004));
  gate.update(true,3,151005); CHECK(gate.tick(151005,report)); CHECK(!gate.armed());
  CHECK(gate.armPersistent(0xfffffff0u));
  gate.tick(0x10000u,report); CHECK(gate.armed());
  CHECK(gate.press(touch::Direction::Down,3,0x10001u,report)); CHECK(report[2]==0x52);
  gate.disarm(0x10002u); CHECK(gate.tick(0x10002u,report)); CHECK(report==Report{});
  std::printf("HID touch control: %d checks, %d failures\n",checks,failures);
  return failures?1:0;
}
