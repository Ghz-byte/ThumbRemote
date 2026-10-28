#pragma once
#include <stddef.h>
#include <stdint.h>

namespace touch {
enum class QueryState { Idle, Holding, Packet };
enum class Event { None, Down, Move, Up };
struct Point { uint16_t x = 0; uint16_t y = 0; };
struct Report { uint8_t mask = 0; Point points[3]; };
inline QueryState classifyQuery(uint8_t query) {
  if (query & 0x80) return QueryState::Packet;
  return (query & 0x40) ? QueryState::Holding : QueryState::Idle;
}
inline bool decodeReport(const uint8_t* bytes, size_t length, Report& output) {
  if (!bytes || length != 14 || (bytes[0] & 0xF0) != 0) return false;
  Report parsed;
  parsed.mask = bytes[0] & 0x07;
  for (uint8_t i = 0; i < 3; ++i) {
    const size_t offset = 2 + i * 4;
    parsed.points[i].x = bytes[offset] | ((bytes[offset + 1] & 0x0F) << 8);
    parsed.points[i].y = bytes[offset + 2] | ((bytes[offset + 1] & 0xF0) << 4);
  }
  output = parsed;
  return true;
}
class TouchState {
 public:
  Event apply(QueryState query, const Report* report = nullptr) {
    if (query == QueryState::Holding) return Event::None;
    if (query == QueryState::Packet && !report) return Event::None;
    if (query == QueryState::Idle || report->mask == 0) {
      const bool wasActive = active();
      previous_ = Report{};
      return wasActive ? Event::Up : Event::None;
    }
    bool changed = previous_.mask != report->mask;
    for (uint8_t i = 0; i < 3; ++i) {
      if ((report->mask & (1 << i)) &&
          (previous_.points[i].x != report->points[i].x ||
           previous_.points[i].y != report->points[i].y)) changed = true;
    }
    const bool wasActive = active();
    previous_ = *report;
    return !wasActive ? Event::Down : (changed ? Event::Move : Event::None);
  }
  bool active() const { return previous_.mask != 0; }
  const char* description(QueryState query, bool healthy) const {
    if (!healthy) return "unknown";
    if (active()) return "active";
    return query == QueryState::Holding ? "holding_without_coordinates" : "idle";
  }
 private:
  Report previous_;
};
}  // namespace touch
