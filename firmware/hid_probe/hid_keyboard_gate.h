#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include "gesture_recognizer.h"

namespace hid_probe {
using Report = std::array<uint8_t, 8>;
// One SDK dispatch slot, not an input history; a neutral release may replace a pending press.
class Delivery {
 public:
  bool offer(const Report& report, uint32_t generation, uint32_t now) {
    if (pending_ && report!=Report{}) return false;
    payload_=report; generation_=generation; offeredAt_=now; pending_=true; return true;
  }
  bool take(bool ready, uint32_t generation, uint32_t now, Report& report) {
    if (!pending_) return false;
    pending_=false;
    if (!ready || generation!=generation_) return false;
    if (payload_!=Report{} && uint32_t(now-offeredAt_)>100) return false;
    report=payload_; return true;
  }
  void cancel() { pending_=false; }
  Report readable() const { return Report{}; }
 private:
  Report payload_{};
  uint32_t generation_=0, offeredAt_=0;
  bool pending_=false;
};
enum class Mode { Arrow, Page };
enum class Command { None, Invalid, Arm, Disarm, Arrow, Page, Status };
class CommandReader {
 public:
  Command push(char c) {
    if (c=='\r' || c=='\n') {
      Command command=Command::Invalid;
      buffer_[size_]=0;
      if (!invalid_) {
        if (!size_) command=Command::None;
        else if (!std::strcmp(buffer_,"arm")) command=Command::Arm;
        else if (!std::strcmp(buffer_,"disarm")) command=Command::Disarm;
        else if (!std::strcmp(buffer_,"mode arrow")) command=Command::Arrow;
        else if (!std::strcmp(buffer_,"mode page")) command=Command::Page;
        else if (!std::strcmp(buffer_,"status")) command=Command::Status;
      }
      size_=0; invalid_=false; return command;
    }
    if (c<32 || c>126 || size_>=sizeof(buffer_)-1) invalid_=true;
    if (!invalid_) buffer_[size_++]=c;
    return Command::None;
  }
 private:
  char buffer_[32]{};
  size_t size_=0;
  bool invalid_=false;
};
class Gate {
 public:
  void update(bool ready, uint32_t generation, uint32_t) {
    if (ready == ready_ && generation == generation_) return;
    generation_=generation; ready_=ready; armed_=false; pressed_=false;
    releasePending_=ready; cooldown_=false;
  }
  bool arm(uint32_t now) {
    if (!ready_ || armed_ || pressed_ || releasePending_) return false;
    armed_=true; persistent_=false; armedAt_=now; return true;
  }
  bool armPersistent(uint32_t now) {
    if (!arm(now)) return false;
    persistent_=true; return true;
  }
  void disarm(uint32_t now) {
    armed_=false;
    // Rebuild a neutral release even when tick already handed the previous one
    // to a dispatch slot that a concurrent stop is about to invalidate.
    releasePending_=ready_;
    if (pressed_) { pressed_=false; cooldown_=true; releasedAt_=now; }
  }
  bool mode(Mode mode) {
    if (armed_ || pressed_ || releasePending_) return false;
    if (mode!=Mode::Arrow && mode!=Mode::Page) return false;
    mode_=mode; return true;
  }
  Mode mode() const { return mode_; }
  bool armed() const { return armed_; }
  bool persistent() const { return armed_ && persistent_; }
  bool press(touch::Direction direction, uint32_t generation, uint32_t now, Report& report) {
    expire(now);
    if (!ready_ || !armed_ || generation!=generation_ || pressed_ || releasePending_) return false;
    if (cooldown_ && uint32_t(now-releasedAt_)<500) return false;
    if (direction!=touch::Direction::Up && direction!=touch::Direction::Down) return false;
    report=Report{};
    report[2]= mode_==Mode::Arrow ? (direction==touch::Direction::Up ? 0x51 : 0x52)
                                    : (direction==touch::Direction::Up ? 0x4e : 0x4b);
    pressed_=true; pressedAt_=now; return true;
  }
  bool tick(uint32_t now, Report& report) {
    expire(now);
    if (!ready_) return false;
    if (!releasePending_ && (!pressed_ || uint32_t(now-pressedAt_)<40)) return false;
    const bool releasedKey=pressed_ || cooldown_;
    pressed_=false; releasePending_=false;
    if (releasedKey) { cooldown_=true; releasedAt_=now; }
    report=Report{}; return true;
  }
 private:
  void expire(uint32_t now) { if (armed_ && !persistent_ && uint32_t(now-armedAt_)>=60000) disarm(now); }
  bool ready_=false, armed_=false, persistent_=false, pressed_=false, releasePending_=false, cooldown_=false;
  uint32_t generation_=0, armedAt_=0, pressedAt_=0, releasedAt_=0;
  Mode mode_=Mode::Arrow;
};
}
