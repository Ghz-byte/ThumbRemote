#include "../touch_probe/touch_protocol.h"
#include <cstdio>
#include <cstring>

int failures = 0;
int checks = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
  std::printf("FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

int main() {
  using namespace touch;
  // Removing bit7 precedence must fail: 0xC0 is a packet, not holding.
  CHECK(classifyQuery(0x00) == QueryState::Idle);
  CHECK(classifyQuery(0x40) == QueryState::Holding);
  CHECK(classifyQuery(0x80) == QueryState::Packet);
  CHECK(classifyQuery(0xC0) == QueryState::Packet);
  CHECK(classifyQuery(0x03) == QueryState::Idle);

  // Hand-derived 12-bit positions: ABC/DEF, 123/456, FFF/000.
  const uint8_t bytes[14] = {0x07, 0, 0xBC, 0xDA, 0xEF, 0,
                            0x23, 0x41, 0x56, 0, 0xFF, 0x0F, 0, 0};
  Report report;
  CHECK(decodeReport(bytes, sizeof(bytes), report));
  CHECK(report.mask == 7);
  CHECK(report.points[0].x == 0xABC && report.points[0].y == 0xDEF);
  CHECK(report.points[1].x == 0x123 && report.points[1].y == 0x456);
  CHECK(report.points[2].x == 0xFFF && report.points[2].y == 0);
  report.mask = 5;
  CHECK(!decodeReport(bytes, 13, report));
  CHECK(report.mask == 5);  // Rejected input must leave caller state intact.
  CHECK(!decodeReport(nullptr, 14, report));
  uint8_t other[14] = {0x80};
  CHECK(!decodeReport(other, 14, report));
  CHECK(report.mask == 5);
  uint8_t release[14] = {};
  CHECK(decodeReport(release, 14, report));
  CHECK(report.mask == 0);

  TouchState state;
  Report point;
  point.mask = 2;  // Point1 alone is still an active touch.
  point.points[1].x = 123;
  CHECK(state.apply(QueryState::Idle) == Event::None);
  CHECK(state.apply(QueryState::Packet, &point) == Event::Down);
  CHECK(state.active());
  CHECK(state.apply(QueryState::Holding) == Event::None);
  CHECK(state.active());
  CHECK(state.apply(QueryState::Packet, nullptr) == Event::None);
  CHECK(state.active());  // Missing/failed packet is not a release.
  CHECK(state.apply(QueryState::Packet, &point) == Event::None);
  point.points[1].x = 124;
  CHECK(state.apply(QueryState::Packet, &point) == Event::Move);
  point.mask = 1;
  CHECK(state.apply(QueryState::Packet, &point) == Event::Move);
  CHECK(state.apply(QueryState::Idle) == Event::Up);
  CHECK(!state.active());
  CHECK(state.apply(QueryState::Idle) == Event::None);
  CHECK(state.apply(QueryState::Holding) == Event::None);
  CHECK(!state.active());
  CHECK(std::strcmp(state.description(QueryState::Holding, true), "holding_without_coordinates") == 0);
  CHECK(std::strcmp(state.description(QueryState::Idle, false), "unknown") == 0);
  CHECK(std::strcmp(state.description(QueryState::Idle, true), "idle") == 0);
  CHECK(state.apply(QueryState::Packet, &point) == Event::Down);
  CHECK(std::strcmp(state.description(QueryState::Holding, true), "active") == 0);
  CHECK(state.apply(QueryState::Packet, &report) == Event::Up);
  CHECK(!state.active());
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
