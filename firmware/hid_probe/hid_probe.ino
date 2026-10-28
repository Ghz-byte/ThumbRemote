#include <Arduino.h>
#include <Wire.h>
#include "touch_protocol.h"
#include "gesture_recognizer.h"
#include "hid_keyboard_transport.h"
#include "hid_direct_control.h"

constexpr int kSda=4, kScl=5;
constexpr uint8_t kAddress=0x46;
HidKeyboardTransport hid;
hid_probe::DirectControl control(hid_probe::Mode::Page);
hid_probe::CommandReader commands;
touch::TouchState touchState;
uint32_t linkGeneration=0, lastPoll=0, lastStatus=0, lastProbe=0;
uint32_t errors=0, unknown=0, gestures=0;
bool busReady=false, found=false, healthy=false;
uint8_t lastQuery=0;

void status() {
  const auto link=hid.state();
  Serial.printf("HID_STATUS generation=%lu connected=%d subscribed=%d encrypted=%d ready=%d enabled=%d mode=%s touch=%s errors=%lu gestures=%lu unknown=%lu\n",
    static_cast<unsigned long>(link.generation),link.connected,link.subscribed,link.encrypted,link.ready(),control.enabled(),
    control.mode()==hid_probe::Mode::Page?"direct_page":"direct_arrow",touchState.description(touch::classifyQuery(lastQuery),healthy),
    static_cast<unsigned long>(errors),static_cast<unsigned long>(gestures),static_cast<unsigned long>(unknown));
}
void cancelInput(const char* reason) {
  hid.cancelPending(); control.cancel(millis());
  Serial.printf("HID_INPUT_CANCEL reason=%s; fresh release required\n",reason);
}
void fail(const char* operation, int code) {
  ++errors; healthy=false; cancelInput("touch communication error");
  Serial.printf("ERROR op=%s code=%d count=%lu\n",operation,code,static_cast<unsigned long>(errors));
}
bool readBuffer(uint8_t selector, uint8_t* output, size_t length) {
  if (!digitalRead(kSda) || !digitalRead(kScl)) { fail("bus_line_low",-1); return false; }
  Wire.beginTransmission(kAddress);
  if (Wire.write(selector)!=1) { Wire.endTransmission(true); fail("selector_write",-1); return false; }
  const uint8_t result=Wire.endTransmission(true);
  if (result) { fail("selector_ack",result); return false; }
  const size_t received=Wire.requestFrom(kAddress,length,true);
  if (received!=length) {
    while(Wire.available()) Wire.read();
    fail("short_read",static_cast<int>(received)); return false;
  }
  for(size_t i=0;i<length;i++) {
    const int value=Wire.read();
    if(value<0) {fail("read_empty",-1); return false;}
    output[i]=static_cast<uint8_t>(value);
  }
  return true;
}
bool probe() {
  Wire.beginTransmission(kAddress); const int rc=Wire.endTransmission(true);
  if (rc) {fail("address_0x46",rc); return false;}
  Serial.println("ACK address=0x46"); return true;
}
void serviceControl() {
  const auto link=hid.state();
  if(link.generation!=linkGeneration) {
    hid.cancelPending(); linkGeneration=link.generation;
    Serial.println("HID link changed; fresh release required; no historical input");
  }
  control.update(link.ready(),linkGeneration,healthy,millis());
  hid_probe::Report report;
  if (control.tick(millis(),report) && !hid.send(report,linkGeneration)) cancelInput("release/neutral could not be attempted");
}
void command(hid_probe::Command c) {
  if(c==hid_probe::Command::None) return;
  if(c==hid_probe::Command::Status) status();
  else Serial.println("HID_DIRECT_MODE; only status command is supported; disconnect Bluetooth or power to stop");
}
void observe(touch::QueryState query, const touch::Report* report=nullptr) {
  serviceControl();
  hid_probe::Report packet;
  if(!control.observe(query,report,millis(),packet)) return;
  ++gestures;
  Serial.printf("GESTURE direction=%s mode=%s count=%lu\n",
    (packet[2]==0x51 || packet[2]==0x4e)?"UP":"DOWN",
    control.mode()==hid_probe::Mode::Page?"direct_page":"direct_arrow",static_cast<unsigned long>(gestures));
  if(!hid.send(packet,linkGeneration)) cancelInput("press could not be attempted");
}
void setup() {
  Serial.begin(115200); delay(300);
  Serial.println("\nThumbRemote hid_probe v3; KEYBOARD ONLY; direct after ready link and fresh release");
  Serial.println(control.mode()==hid_probe::Mode::Page ? "UP -> PageDown; DOWN -> PageUp; keyboard compatibility trial" : "UP -> DownArrow; DOWN -> UpArrow");
  Serial.println("No long-press toggle or time limit; disconnect Bluetooth or power to stop");
  pinMode(kSda,INPUT_PULLUP); pinMode(kScl,INPUT_PULLUP);
  busReady=Wire.begin(kSda,kScl,100000); Wire.setTimeOut(50);
  if(busReady) found=probe(); else fail("wire_begin",-1);
  hid.begin();
}
void loop() {
  hid.poll(); serviceControl();
  // Bound serial work so an input stream cannot starve key release handling.
  for(unsigned i=0;i<32 && Serial.available();i++) command(commands.push(char(Serial.read())));
  serviceControl();
  const uint32_t now=millis();
  if(uint32_t(now-lastStatus)>=2000) {lastStatus=now; status();}
  if(!busReady) {delay(1); return;}
  if(!found) {
    if(uint32_t(now-lastProbe)>=2000) {lastProbe=now; found=probe();}
    delay(1); return;
  }
  if(uint32_t(now-lastPoll)<20) {delay(1); return;}
  lastPoll=now;
  uint8_t query=0;
  if(!readBuffer(0x80,&query,1)) return;
  lastQuery=query;
  const auto state=touch::classifyQuery(query);
  if(state!=touch::QueryState::Packet) {
    healthy=true; touchState.apply(state); observe(state); return;
  }
  uint8_t bytes[14]; if(!readBuffer(0xe0,bytes,sizeof(bytes))) return;
  touch::Report report;
  if(!touch::decodeReport(bytes,sizeof(bytes),report)) {
    ++unknown;
    // Seller's non-coordinate gesture packets are ignored, never interpreted as UP.
    return;
  }
  healthy=true; touchState.apply(state,&report); observe(state,&report);
}
