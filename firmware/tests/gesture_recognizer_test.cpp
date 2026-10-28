#include "../touch_probe/gesture_recognizer.h"
#include <cstdio>

int failures = 0;
int checks = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
  std::printf("FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)
using namespace touch;

Report single(uint16_t x, uint16_t y, uint8_t mask = 1) {
  Report report;
  report.mask = mask;
  for (auto& point : report.points) { point.x = x; point.y = y; }
  return report;
}

GestureResult stroke(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  GestureRecognizer recognizer;
  recognizer.observe(QueryState::Idle);
  Report report = single(x0, y0);
  CHECK(recognizer.observe(QueryState::Packet, &report).outcome == GestureOutcome::None);
  report = single(x1, y1);
  CHECK(recognizer.observe(QueryState::Packet, &report).outcome == GestureOutcome::None);
  CHECK(recognizer.observe(QueryState::Holding).outcome == GestureOutcome::None);
  const auto result = recognizer.observe(QueryState::Idle);
  CHECK(recognizer.observe(QueryState::Idle).outcome == GestureOutcome::None);
  return result;
}

int main() {
  // Twelve literal start/end pairs from the hardware direction logs.
  struct Fixture { uint16_t x0, y0, x1, y1; Direction direction; };
  const Fixture fixtures[] = {
    {208,132,60,136,Direction::Up}, {166,119,70,131,Direction::Up},
    {172,126,53,125,Direction::Up}, {87,114,182,105,Direction::Down},
    {52,114,174,125,Direction::Down}, {43,121,169,128,Direction::Down},
    {128,173,143,36,Direction::Right}, {158,226,152,35,Direction::Right},
    {123,214,148,36,Direction::Right}, {116,53,132,193,Direction::Left},
    {107,59,133,202,Direction::Left}, {81,51,125,204,Direction::Left}
  };
  for (const auto& fixture : fixtures) {
    const auto result = stroke(fixture.x0, fixture.y0, fixture.x1, fixture.y1);
    CHECK(result.outcome == GestureOutcome::Accepted);
    CHECK(result.direction == fixture.direction);
  }
  CHECK(stroke(100,100,139,100).reason == RejectReason::TooShort);
  CHECK(stroke(100,100,140,100).direction == Direction::Down);
  CHECK(stroke(100,100,180,140).direction == Direction::Down);
  CHECK(stroke(100,100,180,141).reason == RejectReason::Ambiguous);
  CHECK(stroke(100,100,100,100).reason == RejectReason::TooShort);
  CHECK(stroke(0,4095,4095,4095).direction == Direction::Down);
  CHECK(stroke(4095,4095,0,4095).dx == -4095);
  CHECK(stroke(4095,4095,4095,0).direction == Direction::Right);

  GestureRecognizer recognizer;
  Report report = single(200,100);
  // Booting in the middle of a touch must never recognize a partial stroke.
  recognizer.observe(QueryState::Packet, &report);
  report = single(50,100);
  recognizer.observe(QueryState::Packet, &report);
  CHECK(recognizer.observe(QueryState::Idle).outcome == GestureOutcome::None);
  // One no-new-packet query before the first coordinate also means no known origin.
  recognizer.observe(QueryState::Holding);
  recognizer.observe(QueryState::Packet, &report);
  report = single(200,100);
  recognizer.observe(QueryState::Packet, &report);
  CHECK(recognizer.observe(QueryState::Idle).outcome == GestureOutcome::None);

  recognizer.observe(QueryState::Packet, &report);
  recognizer.cancel();
  report = single(50,100);
  recognizer.observe(QueryState::Packet, &report);
  CHECK(recognizer.observe(QueryState::Holding).outcome == GestureOutcome::None);
  const auto cancelled = recognizer.observe(QueryState::Idle);
  CHECK(cancelled.outcome == GestureOutcome::Rejected);
  CHECK(cancelled.reason == RejectReason::Communication);
  CHECK(cancelled.direction == Direction::None);
  CHECK(recognizer.observe(QueryState::Idle).outcome == GestureOutcome::None);
  // A fresh contact after confirmed release works again, and packet-mask0 releases too.
  recognizer.observe(QueryState::Packet, &report);
  report = single(200,100);
  recognizer.observe(QueryState::Packet, &report);
  Report released;
  CHECK(recognizer.observe(QueryState::Packet, &released).direction == Direction::Down);
  CHECK(recognizer.observe(QueryState::Packet, &released).outcome == GestureOutcome::None);

  recognizer.observe(QueryState::Packet, &report);
  recognizer.observe(QueryState::Packet, nullptr);
  report = single(50,100);
  recognizer.observe(QueryState::Packet, &report);
  CHECK(recognizer.observe(QueryState::Idle).reason == RejectReason::Communication);

  report = single(50,100,3);
  recognizer.observe(QueryState::Packet, &report);
  report = single(200,100);
  recognizer.observe(QueryState::Packet, &report);
  CHECK(recognizer.observe(QueryState::Idle).reason == RejectReason::MultiplePoints);
  // A single point1 is valid, but swapping identities during a stroke is not.
  report = single(50,100,2);
  recognizer.observe(QueryState::Packet, &report);
  report = single(200,100,2);
  recognizer.observe(QueryState::Packet, &report);
  CHECK(recognizer.observe(QueryState::Idle).direction == Direction::Down);
  recognizer.observe(QueryState::Packet, &report);
  report = single(50,100,1);
  recognizer.observe(QueryState::Packet, &report);
  CHECK(recognizer.observe(QueryState::Idle).reason == RejectReason::PointChanged);

  recognizer.observe(QueryState::Packet, &report);
  report = single(200,100);
  recognizer.observe(QueryState::Packet, &report);
  report = single(50,100);
  recognizer.observe(QueryState::Packet, &report);
  CHECK(recognizer.observe(QueryState::Idle).reason == RejectReason::TooShort);
  // Missing coordinates from an unchanged held finger must not create an event.
  recognizer.cancel();
  CHECK(recognizer.observe(QueryState::Holding).outcome == GestureOutcome::None);
  CHECK(recognizer.observe(QueryState::Idle).outcome == GestureOutcome::None);
  std::printf("Gesture: %d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
