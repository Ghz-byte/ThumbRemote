#include <Arduino.h>
#include <Wire.h>
#include "touch_protocol.h"
#include "dual_ble_transport.h"

using namespace dual_remote;
constexpr int kSda=4,kScl=5;
constexpr uint8_t kAddress=0x46;
DualBleTransport transport;
TouchControl control;
touch::TouchState touchState;
hid_probe::CommandReader commands;
bool busReady=false,found=false,healthy=false;
uint32_t lastPoll=0,lastProbe=0,lastStatus=0,errors=0,unknown=0,gestures=0;
uint8_t lastQuery=0;

void status(){
  const auto s=transport.snapshot();
  Serial.printf("DUAL_STATUS generation=%lu route=%s profile=%u connected=%d advertising=%d encrypted=%d bonded=%d identity=%d app_sub=%d app_claim=%d app_session=%d hid_ready=%d digitizer_ready=%d touch=%s errors=%lu gestures=%lu unknown=%lu\n",
    static_cast<unsigned long>(s.generation),routeName(s.route),unsigned(s.profile.status),s.link.connected,s.advertising,s.link.encrypted,s.link.bonded,s.link.identityReady,
    s.link.appSubscribed,s.link.appClaimed,s.link.appSession!=0,s.link.hidReady,s.link.digitizerReady,touchState.description(touch::classifyQuery(lastQuery),healthy),
    static_cast<unsigned long>(errors),static_cast<unsigned long>(gestures),static_cast<unsigned long>(unknown));
}
void cancelInput(const char* reason){
  control.cancel(millis());transport.cancelInput();
  Serial.printf("DUAL_INPUT_CANCEL reason=%s; fresh release required\n",reason);
}
void fail(const char* operation,int code){
  ++errors;healthy=false;cancelInput("touch communication error");
  Serial.printf("ERROR op=%s code=%d count=%lu\n",operation,code,static_cast<unsigned long>(errors));
}
bool readBuffer(uint8_t selector,uint8_t* output,size_t length){
  if(!digitalRead(kSda) || !digitalRead(kScl)){fail("bus_line_low",-1);return false;}
  Wire.beginTransmission(kAddress);
  if(Wire.write(selector)!=1){Wire.endTransmission(true);fail("selector_write",-1);return false;}
  const auto rc=Wire.endTransmission(true);
  if(rc){fail("selector_ack",rc);return false;}
  const auto count=Wire.requestFrom(kAddress,length,true);
  if(count!=length){while(Wire.available())Wire.read();fail("short_read",int(count));return false;}
  for(size_t i=0;i<length;++i){const auto value=Wire.read();if(value<0){fail("read_empty",-1);return false;}output[i]=uint8_t(value);}
  return true;
}
bool probe(){
  Wire.beginTransmission(kAddress);const auto rc=Wire.endTransmission(true);
  if(rc){fail("address_0x46",rc);return false;}
  Serial.println("ACK address=0x46");return true;
}
void serviceControl(){
  const auto s=transport.snapshot();control.update(s.route,s.generation,healthy,millis());
  Action action;
  if(control.tick(millis(),action) && !transport.send(action))cancelInput("neutral submission failed");
}
void observe(touch::QueryState query,const touch::Report* report=nullptr){
  serviceControl();Action action;
  if(!control.observe(query,report,millis(),action))return;
  ++gestures;
  Serial.printf("GESTURE route=%s direction=%u tap=%d count=%lu\n",routeName(action.route),unsigned(action.direction),action.tap,static_cast<unsigned long>(gestures));
  if(!transport.send(action))cancelInput("direction dropped; busy or state changed");
}
void setup(){
  Serial.begin(115200);delay(300);
  Serial.println("\nThumbRemote dual_remote v1; OPPO App + MatePad Arrow HID; one peer, no replay");
  Serial.println("First use: enroll OPPO with App START; HID disabled before enrollment. Keep bonds; user handles pairing.");
  pinMode(kSda,INPUT_PULLUP);pinMode(kScl,INPUT_PULLUP);
  busReady=Wire.begin(kSda,kScl,100000);Wire.setTimeOut(50);
  if(busReady)found=probe();else fail("wire_begin",-1);
  transport.begin();
}
void loop(){
  transport.poll();serviceControl();
  for(unsigned i=0;i<32 && Serial.available();++i){
    const auto command=commands.push(char(Serial.read()));
    if(command==hid_probe::Command::Status)status();
    else if(command!=hid_probe::Command::None)Serial.println("DUAL: status only; switch devices by disconnecting current Bluetooth peer");
  }
  serviceControl();const auto now=millis();
  if(uint32_t(now-lastStatus)>=2000){lastStatus=now;status();}
  if(!busReady){delay(1);return;}
  if(!found){if(uint32_t(now-lastProbe)>=2000){lastProbe=now;found=probe();}delay(1);return;}
  if(uint32_t(now-lastPoll)<20){delay(1);return;}lastPoll=now;
  uint8_t query=0;if(!readBuffer(0x80,&query,1))return;lastQuery=query;
  const auto state=touch::classifyQuery(query);
  if(state!=touch::QueryState::Packet){healthy=true;touchState.apply(state);observe(state);return;}
  uint8_t bytes[14];if(!readBuffer(0xe0,bytes,sizeof(bytes)))return;
  touch::Report report;
  if(!touch::decodeReport(bytes,sizeof(bytes),report)){++unknown;return;} // Normal non-coordinate Flick: neither UP nor an error.
  healthy=true;touchState.apply(state,&report);observe(state,&report);
}
