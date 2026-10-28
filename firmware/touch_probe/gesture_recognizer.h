#pragma once
#include "touch_protocol.h"

namespace touch {
enum class GestureOutcome { None, Accepted, Rejected };
enum class Direction { None, Up, Down, Left, Right };
enum class RejectReason { None, TooShort, Ambiguous, Communication, MultiplePoints, PointChanged };
struct GestureResult {
  GestureOutcome outcome = GestureOutcome::None;
  Direction direction = Direction::None;
  RejectReason reason = RejectReason::None;
  int32_t dx = 0;
  int32_t dy = 0;
};
class GestureRecognizer {
 public:
  static constexpr int32_t kMinDistance = 40;
  static constexpr int32_t kAxisRatio = 2;

  GestureResult observe(QueryState query, const Report* report = nullptr) {
    if (query == QueryState::Idle ||
        (query == QueryState::Packet && report && report->mask == 0)) return release();
    if (query == QueryState::Holding) {
      // Without an observed origin, do not start halfway through a held contact.
      if (!tracking_) blocked_ = true;
      return GestureResult{};
    }
    if (!report) { cancel(); return GestureResult{}; }
    const uint8_t mask = report->mask;
    if (mask != 1 && mask != 2 && mask != 4) {
      rejectContact(RejectReason::MultiplePoints);
      return GestureResult{};
    }
    if (blocked_) return GestureResult{};
    if (tracking_ && mask != pointMask_) {
      rejectContact(RejectReason::PointChanged);
      return GestureResult{};
    }
    const uint8_t index = mask == 1 ? 0 : (mask == 2 ? 1 : 2);
    last_ = report->points[index];
    if (!tracking_) {
      start_ = last_;
      pointMask_ = mask;
      tracking_ = true;
    }
    return GestureResult{};
  }

  void cancel() {
    if (tracking_ || pendingReason_ != RejectReason::None) {
      pendingReason_ = RejectReason::Communication;
    }
    tracking_ = false;
    blocked_ = true;
  }

 private:
  bool tracking_ = false;
  bool blocked_ = true;  // Require a confirmed release after startup/fault.
  uint8_t pointMask_ = 0;
  Point start_;
  Point last_;
  RejectReason pendingReason_ = RejectReason::None;

  void rejectContact(RejectReason reason) {
    pendingReason_ = reason;
    tracking_ = false;
    blocked_ = true;
  }

  GestureResult release() {
    GestureResult result;
    if (pendingReason_ != RejectReason::None) {
      result.outcome = GestureOutcome::Rejected;
      result.reason = pendingReason_;
    } else if (!blocked_ && tracking_) {
      result.dx = static_cast<int32_t>(last_.x) - start_.x;
      result.dy = static_cast<int32_t>(last_.y) - start_.y;
      const int32_t ax = result.dx < 0 ? -result.dx : result.dx;
      const int32_t ay = result.dy < 0 ? -result.dy : result.dy;
      const int32_t major = ax > ay ? ax : ay;
      const int32_t minor = ax > ay ? ay : ax;
      result.outcome = GestureOutcome::Rejected;
      if (major < kMinDistance) {
        result.reason = RejectReason::TooShort;
      } else if (major < kAxisRatio * minor) {
        result.reason = RejectReason::Ambiguous;
      } else {
        result.outcome = GestureOutcome::Accepted;
        result.direction = ax > ay ? (result.dx < 0 ? Direction::Up : Direction::Down)
                                   : (result.dy < 0 ? Direction::Right : Direction::Left);
      }
    }
    tracking_ = false;
    blocked_ = false;
    pendingReason_ = RejectReason::None;
    return result;
  }
};
}  // namespace touch
