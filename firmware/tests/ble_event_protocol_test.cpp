#include "../touch_probe/ble_event_protocol.h"
#include <cstdio>
int failures=0, checks=0;
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; std::printf("FAIL %d: %s\n",__LINE__,#c); } } while(0)
int main() {
  using namespace remote_ble;
  const Packet want = {{1,0x78,0x56,0x34,0x12,1,0,0,0,1}};
  CHECK(encodeDirection(0x12345678,1,touch::Direction::Up)==want);
  CHECK(encodeDirection(1,1,touch::Direction::Down)[9]==2);
  CHECK(encodeDirection(1,1,touch::Direction::Left)[9]==3);
  CHECK(encodeDirection(1,1,touch::Direction::Right)[9]==4);
  CHECK(encodeDirection(1,0xffffffff,touch::Direction::Right)[8]==0xff);
  CHECK(encodeReady(0x12345678)[1]==0x78);
  CHECK(encodeReady(1)[9]==0 && encodeReady(1)[5]==0);
  const uint8_t start[] = {1,0x78,0x56,0x34,0x12}; uint32_t session=0;
  CHECK(decodeStart(start,5,session) && session==0x12345678);
  CHECK(!decodeStart(start,4,session));
  CHECK(!decodeStart(start,6,session));
  CHECK(!decodeStart(nullptr,5,session));
  const uint8_t zero[]={1,0,0,0,0}, version[]={2,1,0,0,0};
  CHECK(!decodeStart(zero,5,session)); CHECK(!decodeStart(version,5,session));
  SendGate gate; Packet packet;
  CHECK(!gate.prepare(touch::Direction::Up,0,packet));
  gate.update(true,false,0,1); CHECK(!gate.ready());
  gate.update(true,true,0,2); CHECK(!gate.ready());
  gate.update(true,true,123,3); CHECK(gate.ready()); CHECK(gate.generation()==3);
  CHECK(!gate.prepare(touch::Direction::Up,2,packet));
  CHECK(gate.prepare(touch::Direction::Up,3,packet)); CHECK(packet[5]==1 && packet[9]==1);
  CHECK(gate.prepare(touch::Direction::Down,3,packet)); CHECK(packet[5]==2);
  gate.update(false,false,0,4); CHECK(!gate.prepare(touch::Direction::Up,3,packet));
  gate.update(true,true,456,7); CHECK(!gate.prepare(touch::Direction::Up,3,packet));
  CHECK(gate.prepare(touch::Direction::Left,7,packet)); CHECK(packet[5]==1 && packet[9]==3);
  CHECK(!gate.prepare(touch::Direction::None,7,packet));
  gate.update(true,false,0,8); CHECK(!gate.prepare(touch::Direction::Up,7,packet));
  std::printf("BLE protocol: %d checks, %d failures\n",checks,failures); return failures?1:0;
}
