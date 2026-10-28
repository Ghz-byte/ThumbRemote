#pragma once
#include "hid_keyboard_gate.h"
namespace dual_remote {
// Keyboard gate for dual only: reliable release without the historic 500 ms cooldown.
class KeyboardGate {
 public:
  void update(bool ready,uint32_t generation,uint32_t){
    if(ready==ready_ && generation==generation_)return;
    ready_=ready;generation_=generation;armed_=false;pressed_=false;releasePending_=ready;
  }
  bool armPersistent(uint32_t){
    if(!ready_ || armed_ || pressed_ || releasePending_)return false;
    armed_=true;return true;
  }
  bool armed() const{return armed_;}
  void disarm(uint32_t){armed_=false;pressed_=false;releasePending_=ready_;}
  bool press(touch::Direction direction,uint32_t generation,uint32_t now,hid_probe::Report& out){
    if(!ready_ || !armed_ || generation!=generation_ || pressed_ || releasePending_)return false;
    if(direction!=touch::Direction::Up && direction!=touch::Direction::Down)return false;
    out={};out[2]=direction==touch::Direction::Up?0x51:0x52;pressed_=true;pressedAt_=now;return true;
  }
  bool tick(uint32_t now,hid_probe::Report& out){
    if(!ready_ || (!releasePending_ && (!pressed_ || uint32_t(now-pressedAt_)<40)))return false;
    pressed_=false;releasePending_=false;out={};return true;
  }
 private:
  bool ready_=false,armed_=false,pressed_=false,releasePending_=false;
  uint32_t generation_=0,pressedAt_=0;
};
}
