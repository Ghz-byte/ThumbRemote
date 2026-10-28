#include <Arduino.h>
#include <Wire.h>
#include "touch_protocol.h"
#include "gesture_recognizer.h"
#include "ble_transport.h"

constexpr int kSda = 4;
constexpr int kScl = 5;
constexpr uint8_t kAddress = 0x46;
constexpr uint32_t kPollMs = 20;
constexpr uint32_t kHeartbeatMs = 2000;

touch::TouchState touchState;
touch::GestureRecognizer gestureRecognizer;
BleTransport bleTransport;
uint32_t gestureGeneration = 0;
uint32_t gestureCount = 0;
uint32_t gestureRejectCount = 0;
bool busReady = false;
bool deviceFound = false;
bool linkHealthy = false;
uint32_t lastProbeMs = 0;
uint32_t lastPollMs = 0;
uint32_t lastHeartbeatMs = 0;
uint32_t lastErrorMs = 0;
uint32_t errorCount = 0;
uint32_t queryCount = 0;
uint32_t packetCount = 0;
uint32_t unknownCount = 0;
uint8_t lastQuery = 0;

void reportError(const char* operation, int code) {
  gestureRecognizer.cancel();
  ++errorCount;
  linkHealthy = false;
  const uint32_t now = millis();
  if (errorCount == 1 || now - lastErrorMs >= 1000) {
    lastErrorMs = now;
    Serial.printf("ERROR op=%s code=%d count=%lu; touch state UNKNOWN, not UP\n",
                  operation, code, static_cast<unsigned long>(errorCount));
  }
}

bool linesHigh() {
  return digitalRead(kSda) == HIGH && digitalRead(kScl) == HIGH;
}

bool probeDevice() {
  if (!linesHigh()) {
    reportError("bus_line_low", -1);
    return false;
  }
  Wire.beginTransmission(kAddress);
  const uint8_t result = Wire.endTransmission(true);
  if (result != 0) {
    reportError("address_0x46", result);
    return false;
  }
  Serial.println("ACK address=0x46; protocol and touch still need verification");
  return true;
}

bool readBuffer(uint8_t selector, uint8_t* output, size_t length) {
  if (!linesHigh()) {
    reportError("bus_line_low", -1);
    return false;
  }
  Wire.beginTransmission(kAddress);
  if (Wire.write(selector) != 1) {
    Wire.endTransmission(true);
    reportError("selector_write", -1);
    return false;
  }
  // STOP-separated DMA read is documented in the seller guide, page 10.
  const uint8_t result = Wire.endTransmission(true);
  if (result != 0) {
    reportError("selector_ack", result);
    return false;
  }
  const size_t received = Wire.requestFrom(kAddress, length, true);
  if (received != length) {
    while (Wire.available()) Wire.read();
    reportError("short_read", static_cast<int>(received));
    return false;
  }
  for (size_t i = 0; i < length; ++i) {
    const int value = Wire.read();
    if (value < 0) {
      reportError("read_empty", -1);
      return false;
    }
    output[i] = static_cast<uint8_t>(value);
  }
  return true;
}

void emitEvent(touch::Event event, const touch::Report* report) {
  if (event == touch::Event::None) return;
  if (event == touch::Event::Up) {
    Serial.printf("UP t=%lu source=%s\n", static_cast<unsigned long>(millis()),
                  report ? "point_packet" : "query_idle");
    return;
  }
  Serial.printf("%s t=%lu mask=0x%X", event == touch::Event::Down ? "DOWN" : "MOVE",
                static_cast<unsigned long>(millis()), report->mask);
  for (uint8_t i = 0; i < 3; ++i) {
    if (report->mask & (1 << i)) {
      Serial.printf(" p%u=(%u,%u)", i, report->points[i].x, report->points[i].y);
    }
  }
  Serial.println();
}

void observeGesture(touch::QueryState query, const touch::Report* report = nullptr) {
  const uint32_t generation = bleTransport.generation();
  if (generation != gestureGeneration) {
    gestureRecognizer.cancel();
    gestureGeneration = generation;
  }
  const touch::GestureResult result = gestureRecognizer.observe(query, report);
  if (result.outcome == touch::GestureOutcome::None) return;
  if (result.outcome == touch::GestureOutcome::Accepted) {
    ++gestureCount;
    const char* direction = "NONE";
    switch (result.direction) {
      case touch::Direction::Up: direction = "UP"; break;
      case touch::Direction::Down: direction = "DOWN"; break;
      case touch::Direction::Left: direction = "LEFT"; break;
      case touch::Direction::Right: direction = "RIGHT"; break;
      default: break;
    }
    Serial.printf("GESTURE direction=%s dx=%ld dy=%ld count=%lu\n", direction,
                  static_cast<long>(result.dx), static_cast<long>(result.dy),
                  static_cast<unsigned long>(gestureCount));
    bleTransport.sendDirection(result.direction, gestureGeneration);
    return;
  }
  ++gestureRejectCount;
  const char* reason = "none";
  switch (result.reason) {
    case touch::RejectReason::TooShort: reason = "too_short"; break;
    case touch::RejectReason::Ambiguous: reason = "ambiguous"; break;
    case touch::RejectReason::Communication: reason = "communication"; break;
    case touch::RejectReason::MultiplePoints: reason = "multiple_points"; break;
    case touch::RejectReason::PointChanged: reason = "point_changed"; break;
    default: break;
  }
  Serial.printf("GESTURE_REJECT reason=%s dx=%ld dy=%ld count=%lu\n", reason,
                static_cast<long>(result.dx), static_cast<long>(result.dy),
                static_cast<unsigned long>(gestureRejectCount));
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nThumbRemote touch_probe v3 BLE receiver; no display, no reset/config commands");
  bleTransport.begin();
  Serial.printf("Gesture min_distance=%ld axis_ratio=%ld; X-=UP X+=DOWN Y-=RIGHT Y+=LEFT; last tested orientation only\n",
                static_cast<long>(touch::GestureRecognizer::kMinDistance),
                static_cast<long>(touch::GestureRecognizer::kAxisRatio));
  Serial.println("SDA=GPIO4 SCL=GPIO5 I2C=100000 address=0x46 timeout=50ms");
  Serial.println("External pull-ups unverified; internal weak pull-ups are not electrical validation");
  pinMode(kSda, INPUT_PULLUP);
  pinMode(kScl, INPUT_PULLUP);
  delay(2);
  Serial.printf("Idle levels with weak pull-ups: SDA=%d SCL=%d\n",
                digitalRead(kSda), digitalRead(kScl));
  busReady = Wire.begin(kSda, kScl, 100000);
  Wire.setTimeOut(50);
  if (!busReady) {
    reportError("wire_begin", -1);
    return;
  }
  deviceFound = probeDevice();
  lastProbeMs = millis();
}

void loop() {
  bleTransport.poll();
  const uint32_t now = millis();
  if (now - lastHeartbeatMs >= kHeartbeatMs) {
    lastHeartbeatMs = now;
    Serial.printf("STATUS bus=%s device=%s link=%s touch=%s query=0x%02X queries=%lu packets=%lu unknown=%lu errors=%lu gestures=%lu rejected=%lu\n",
                  busReady ? "ready" : "failed", deviceFound ? "ack_seen" : "not_found",
                  linkHealthy ? "ok" : "unverified_or_error",
                  touchState.description(touch::classifyQuery(lastQuery), linkHealthy),
                  lastQuery, static_cast<unsigned long>(queryCount),
                  static_cast<unsigned long>(packetCount), static_cast<unsigned long>(unknownCount),
                  static_cast<unsigned long>(errorCount), static_cast<unsigned long>(gestureCount),
                  static_cast<unsigned long>(gestureRejectCount));
  }
  if (!busReady) { delay(10); return; }
  if (!deviceFound) {
    if (now - lastProbeMs >= kHeartbeatMs) {
      lastProbeMs = now;
      deviceFound = probeDevice();
    }
    delay(1);
    return;
  }
  if (now - lastPollMs < kPollMs) { delay(1); return; }
  lastPollMs = now;
  uint8_t query = 0;
  if (!readBuffer(0x80, &query, 1)) return;
  ++queryCount;
  lastQuery = query;
  const touch::QueryState queryState = touch::classifyQuery(query);
  if (queryState != touch::QueryState::Packet) {
    // A held finger without a new report is never converted to UP.
    linkHealthy = true;
    emitEvent(touchState.apply(queryState), nullptr);
    observeGesture(queryState);
    return;
  }
  uint8_t bytes[14];
  if (!readBuffer(0xE0, bytes, sizeof(bytes))) return;
  ++packetCount;
  touch::Report report;
  if (!touch::decodeReport(bytes, sizeof(bytes), report)) {
    ++unknownCount;
    linkHealthy = false;
    if (unknownCount == 1 || unknownCount % 50 == 0) {
      Serial.print("UNKNOWN_REPORT raw=");
      for (uint8_t value : bytes) Serial.printf("%02X ", value);
      Serial.println("; not interpreted as coordinates or UP");
    }
    return;
  }
  linkHealthy = true;
  emitEvent(touchState.apply(queryState, &report), &report);
  observeGesture(queryState, &report);
}
