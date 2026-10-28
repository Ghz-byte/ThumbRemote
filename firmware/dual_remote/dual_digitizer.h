#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace dual_remote {
using DigitizerReport=std::array<uint8_t,5>;
inline DigitizerReport digitizerPress(){return {{0x05,0x00,0x40,0x00,0x40}};}
inline DigitizerReport digitizerRelease(){return {{0x00,0x00,0x40,0x00,0x40}};}
// Generic pen, Report ID 2. Adapted from folgercn/ESP32-BLE-Mouse (MIT);
// copyright and license are preserved in docs/third-party-notices.md.
static const uint8_t digitizerReportMap[]={
  0x05,0x0d,0x09,0x02,0xa1,0x01,0x85,0x02,0x09,0x20,0xa1,0x00,
  0x09,0x42,0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x01,0x81,0x02,
  0x09,0x44,0x81,0x02,0x09,0x32,0x81,0x02,0x95,0x05,0x81,0x03,
  0x05,0x01,0x09,0x30,0x16,0x00,0x00,0x26,0xff,0x7f,0x75,0x10,
  0x95,0x01,0x81,0x02,0x09,0x31,0x81,0x02,0xc0,0xc0
};
static constexpr size_t digitizerReportMapSize=sizeof(digitizerReportMap);
// Host event loop calls next; timestamps use wrap-safe unsigned arithmetic.
class Digitizer {
 public:
  bool offer(uint32_t now){
    if(count_ >= (down_?1u:2u))return false;
    offeredAt_[(head_+count_)%2]=now;++count_;return true;
  }
  bool next(uint32_t now,DigitizerReport& out){
    if(down_){
      if(uint32_t(now-pressedAt_)<60)return false;
      down_=false;out=digitizerRelease();return true;
    }
    while(count_){
      const auto offered=offeredAt_[head_];head_=(head_+1)%2;--count_;
      if(uint32_t(now-offered)>300)continue;
      down_=true;pressedAt_=now;out=digitizerPress();return true;
    }
    return false;
  }
  void cancel(){count_=0;head_=0;down_=false;}
  bool active() const{return count_!=0 || down_;}
 private:
  bool down_=false;
  size_t head_=0,count_=0;
  uint32_t offeredAt_[2]{},pressedAt_=0;
};
}
