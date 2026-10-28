#include "ble_transport.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>

namespace {
constexpr const char* kService="50101af6-adf1-4884-b512-754d9f980ed0";
constexpr const char* kEvent="d71a4ee9-9da8-4672-9e8f-e6478b2ca2b0";
constexpr const char* kStart="b5fb20eb-73b5-4cfc-ba46-d787489c1b80";
struct Shared {
  bool connected=false, subscribed=false, advertise=false;
  uint16_t connection=0xffff;
  uint32_t session=0, revision=0;
} shared;
portMUX_TYPE stateMux=portMUX_INITIALIZER_UNLOCKED;
BLECharacteristic* eventCharacteristic=nullptr;
Shared snapshot() {
  portENTER_CRITICAL(&stateMux); const Shared state=shared; portEXIT_CRITICAL(&stateMux);
  return state;
}
class ServerCallbacks final : public BLEServerCallbacks {
  void onConnect(BLEServer* server, ble_gap_conn_desc* desc) override {
    const uint16_t id=desc->conn_handle;
    portENTER_CRITICAL(&stateMux);
    const bool extra=shared.connected;
    if (!extra) {
      shared.connected=true; shared.subscribed=false; shared.session=0;
      shared.connection=id; shared.advertise=false; ++shared.revision;
    }
    portEXIT_CRITICAL(&stateMux);
    if (extra) server->disconnect(id);
    BLEDevice::getAdvertising()->stop();
  }
  void onDisconnect(BLEServer*, ble_gap_conn_desc* desc) override {
    portENTER_CRITICAL(&stateMux);
    if (shared.connection==desc->conn_handle) {
      shared.connected=false; shared.subscribed=false; shared.session=0;
      shared.connection=0xffff; shared.advertise=true; ++shared.revision;
    }
    portEXIT_CRITICAL(&stateMux);
  }
} serverCallbacks;
class SubscriptionCallbacks final : public BLECharacteristicCallbacks {
  void onSubscribe(BLECharacteristic*, ble_gap_conn_desc* desc, uint16_t subValue) override {
    const bool enabled=subValue==1;
    portENTER_CRITICAL(&stateMux);
    if (shared.connected && shared.connection==desc->conn_handle) {
      shared.subscribed=enabled; shared.session=0; ++shared.revision;
    }
    portEXIT_CRITICAL(&stateMux);
  }
} subscriptionCallbacks;
class StartCallbacks final : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic, ble_gap_conn_desc* desc) override {
    const String value=characteristic->getValue(); uint32_t session=0;
    if (!remote_ble::decodeStart(reinterpret_cast<const uint8_t*>(value.c_str()),value.length(),session)) return;
    portENTER_CRITICAL(&stateMux);
    if (shared.connected && shared.subscribed && shared.connection==desc->conn_handle) {
      shared.session=session; ++shared.revision;
    }
    portEXIT_CRITICAL(&stateMux);
  }
} startCallbacks;
bool notifyPacket(remote_ble::Packet packet, uint32_t generation) {
  const Shared state=snapshot();
  if (!eventCharacteristic || !state.connected || !state.subscribed || !state.session || state.revision!=generation) return false;
  eventCharacteristic->setValue(packet.data(),packet.size());
  eventCharacteristic->notify();
  return true;  // SDK notification attempt, not a phone delivery acknowledgement.
}
}

void BleTransport::begin() {
  BLEDevice::init("ThumbRemote");
  BLEServer* server=BLEDevice::createServer(); server->setCallbacks(&serverCallbacks);
  BLEService* service=server->createService(kService);
  eventCharacteristic=service->createCharacteristic(kEvent,BLECharacteristic::PROPERTY_NOTIFY);
  // The installed NimBLE backend generates the standard 0x2902 automatically.
  eventCharacteristic->setCallbacks(&subscriptionCallbacks);
  BLECharacteristic* start=service->createCharacteristic(kStart,BLECharacteristic::PROPERTY_WRITE);
  start->setCallbacks(&startCallbacks);
  service->start();
  BLEAdvertisementData advertisement; advertisement.setFlags(0x06); advertisement.setCompleteServices(BLEUUID(kService));
  BLEAdvertisementData response; response.setName("ThumbRemote");
  BLEAdvertising* advertising=BLEDevice::getAdvertising();
  advertising->setAdvertisementData(advertisement); advertising->setScanResponseData(response);
  advertising->setScanResponse(true); advertising->start();
  Serial.println("BLE advertising ThumbRemote; receiver only, no historical queue");
}

uint32_t BleTransport::generation() const { return snapshot().revision; }

void BleTransport::poll() {
  const Shared state=snapshot();
  if (state.revision!=gate_.generation()) {
    gate_.update(state.connected,state.subscribed,state.session,state.revision);
    Serial.printf("BLE_STATE generation=%lu connected=%d subscribed=%d ready=%d\n",
      static_cast<unsigned long>(state.revision),state.connected,state.subscribed,gate_.ready());
    if (gate_.ready()) {
      const bool attempted=notifyPacket(remote_ble::encodeReady(state.session),state.revision);
      Serial.printf("BLE_READY_ATTEMPT session=%lu attempted=%d\n",static_cast<unsigned long>(state.session),attempted);
    }
  }
  if (state.advertise && !state.connected) {
    portENTER_CRITICAL(&stateMux);
    const bool restart=shared.advertise && !shared.connected;
    if (restart) shared.advertise=false;
    portEXIT_CRITICAL(&stateMux);
    if (restart) BLEDevice::startAdvertising();
  }
}

bool BleTransport::sendDirection(touch::Direction direction, uint32_t capturedGeneration) {
  remote_ble::Packet packet;
  const bool prepared=gate_.prepare(direction,capturedGeneration,packet);
  const bool attempted=prepared && notifyPacket(packet,capturedGeneration);
  if (attempted) {
    Serial.printf("BLE_NOTIFY_ATTEMPT session=%lu seq=%lu code=%u\n",
      static_cast<unsigned long>(static_cast<uint32_t>(packet[1]) | (static_cast<uint32_t>(packet[2])<<8) |
        (static_cast<uint32_t>(packet[3])<<16) | (static_cast<uint32_t>(packet[4])<<24)),
      static_cast<unsigned long>(static_cast<uint32_t>(packet[5]) | (static_cast<uint32_t>(packet[6])<<8) |
        (static_cast<uint32_t>(packet[7])<<16) | (static_cast<uint32_t>(packet[8])<<24)),packet[9]);
  } else Serial.println("BLE_DROP no current ready session; not queued");
  return attempted;
}
