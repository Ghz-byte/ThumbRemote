#include "hid_keyboard_transport.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>
#include <host/ble_gap.h>
#include <host/ble_gatt.h>
#include <host/ble_hs_mbuf.h>
#include <nimble/nimble_port.h>

namespace {
portMUX_TYPE stateMux=portMUX_INITIALIZER_UNLOCKED;
HidLink sharedLink;
uint16_t connection=0xffff;
bool advertiseAgain=false;
BLECharacteristic* input=nullptr;
hid_probe::Delivery delivery;
ble_npl_event dispatchEvent;
ble_gap_event_fn* originalGap=nullptr;
void* originalGapArg=nullptr;
HidLink snapshot() {
  portENTER_CRITICAL(&stateMux); auto s=sharedLink; portEXIT_CRITICAL(&stateMux); return s;
}
// NimBLE processes GAP callbacks and this event on the same host event queue.
// No BLE subscription vector is read by the Arduino loop, and a report is bound
// to one explicit connection at the actual SDK notification boundary.
void dispatchOnHost(ble_npl_event*) {
  hid_probe::Report report;
  uint16_t id;
  uint32_t generation;
  portENTER_CRITICAL(&stateMux);
  const bool accepted=delivery.take(sharedLink.ready(),sharedLink.generation,millis(),report);
  id=connection; generation=sharedLink.generation;
  portEXIT_CRITICAL(&stateMux);
  if (!accepted || !input) return;
  ble_gap_conn_desc desc{};
  if (ble_gap_conn_find(id,&desc)!=0 || !desc.sec_state.encrypted) return;
  auto packet=ble_hs_mbuf_from_flat(report.data(),report.size());
  const int rc=packet ? ble_gatts_notify_custom(id,input->getHandle(),packet) : BLE_HS_ENOMEM;
  Serial.printf("HID_NOTIFY_ATTEMPT generation=%lu key=0x%02X rc=%d\n",static_cast<unsigned long>(generation),report[2],rc);
  if (rc) {
    // Failed dispatches are not retried; disconnect to clear any host key state.
    portENTER_CRITICAL(&stateMux); sharedLink.encrypted=false; ++sharedLink.generation; delivery.cancel(); portEXIT_CRITICAL(&stateMux);
    ble_gap_terminate(id,BLE_ERR_REM_USER_CONN_TERM);
  }
}
void rejectUnexpectedPairing() {
  uint16_t id;
  portENTER_CRITICAL(&stateMux); id=connection; sharedLink.encrypted=false; ++sharedLink.generation; portEXIT_CRITICAL(&stateMux);
  if (id!=0xffff) ble_gap_terminate(id,BLE_ERR_REM_USER_CONN_TERM);
  Serial.println("HID_PAIRING_REJECT unsupported confirmation/input; no passkey supplied");
}
// Interpose only this public SDK entry point, retaining the original server callback.
// The installed BLEServer otherwise deletes a peer bond on REPEAT_PAIRING.
int guardedGap(ble_gap_event* event, void*) {
  if (event->type==BLE_GAP_EVENT_REPEAT_PAIRING) {
    rejectUnexpectedPairing();
    Serial.println("HID_PAIRING_REJECT repeat pairing; existing bond preserved");
    return BLE_GAP_REPEAT_PAIRING_IGNORE;
  }
  return originalGap ? originalGap(event,originalGapArg) : 0;
}
class ServerCallbacks final : public BLEServerCallbacks {
  void onConnect(BLEServer* server, ble_gap_conn_desc* desc) override {
    portENTER_CRITICAL(&stateMux);
    const bool extra=sharedLink.connected;
    if (!extra) {
      connection=desc->conn_handle; sharedLink.connected=true; sharedLink.subscribed=false;
      sharedLink.encrypted=desc->sec_state.encrypted; sharedLink.reportMode=true; sharedLink.suspended=false;
      advertiseAgain=false; ++sharedLink.generation;
    }
    portEXIT_CRITICAL(&stateMux);
    if (extra) { server->disconnect(desc->conn_handle); return; }
    BLEDevice::getAdvertising()->stop();
    int rc=0;
    const bool started=BLESecurity::startSecurity(desc->conn_handle,&rc);
    Serial.printf("HID_SECURITY_START attempted=%d rc=%d\n",started,rc);
  }
  void onDisconnect(BLEServer*, ble_gap_conn_desc* desc) override {
    portENTER_CRITICAL(&stateMux);
    if (connection==desc->conn_handle) {
      connection=0xffff; sharedLink.connected=false; sharedLink.subscribed=false; sharedLink.encrypted=false;
      advertiseAgain=true; ++sharedLink.generation;
    }
    portEXIT_CRITICAL(&stateMux);
  }
} serverCallbacks;
class SecurityCallbacks final : public BLESecurityCallbacks {
  bool onSecurityRequest() override { return snapshot().connected; }
  uint32_t onPassKeyRequest() override { rejectUnexpectedPairing(); return 0; }
  void onPassKeyNotify(uint32_t) override { rejectUnexpectedPairing(); }
  bool onConfirmPIN(uint32_t) override { rejectUnexpectedPairing(); return false; }
  bool onAuthorizationRequest(uint16_t, uint16_t, bool) override { return false; }
  void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
    if (!desc) return;
    portENTER_CRITICAL(&stateMux);
    if (connection==desc->conn_handle && sharedLink.encrypted!=bool(desc->sec_state.encrypted)) {
      sharedLink.encrypted=desc->sec_state.encrypted; ++sharedLink.generation;
    }
    portEXIT_CRITICAL(&stateMux);
  }
} securityCallbacks;
class InputCallbacks final : public BLECharacteristicCallbacks {
  void onSubscribe(BLECharacteristic*, ble_gap_conn_desc* desc, uint16_t value) override {
    portENTER_CRITICAL(&stateMux);
    if (connection==desc->conn_handle) { sharedLink.subscribed=(value==1); ++sharedLink.generation; }
    portEXIT_CRITICAL(&stateMux);
  }
  void onStatus(BLECharacteristic*, Status status, uint32_t code) override {
    Serial.printf("HID_TX_STATUS status=%u rc=%lu; not app acknowledgement\n",unsigned(status),static_cast<unsigned long>(code));
  }
} inputCallbacks;
class ModeCallbacks final : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c, ble_gap_conn_desc* desc) override {
    const String value=c->getValue();
    portENTER_CRITICAL(&stateMux);
    if (connection==desc->conn_handle) {
      sharedLink.reportMode=value.length()==1 && uint8_t(value[0])==1; ++sharedLink.generation;
    }
    portEXIT_CRITICAL(&stateMux);
  }
} modeCallbacks;
class ControlCallbacks final : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c, ble_gap_conn_desc* desc) override {
    const String value=c->getValue();
    portENTER_CRITICAL(&stateMux);
    if (connection==desc->conn_handle) {
      sharedLink.suspended=!(value.length()==1 && uint8_t(value[0])==1); ++sharedLink.generation;
    }
    portEXIT_CRITICAL(&stateMux);
  }
} controlCallbacks;
// Standard keyboard report ID 1: modifiers, reserved, six key usages; LED output.
uint8_t reportMap[]={
  0x05,0x01,0x09,0x06,0xa1,0x01,0x85,0x01,
  0x05,0x07,0x19,0xe0,0x29,0xe7,0x15,0x00,0x25,0x01,
  0x75,0x01,0x95,0x08,0x81,0x02,
  0x95,0x01,0x75,0x08,0x81,0x01,
  0x95,0x05,0x75,0x01,0x05,0x08,0x19,0x01,0x29,0x05,0x91,0x02,
  0x95,0x01,0x75,0x03,0x91,0x01,
  0x95,0x06,0x75,0x08,0x15,0x00,0x25,0x65,
  0x05,0x07,0x19,0x00,0x29,0x65,0x81,0x00,0xc0
};
}

extern "C" int __real_ble_gap_adv_start(uint8_t, const ble_addr_t*, int32_t,
                                        const ble_gap_adv_params*, ble_gap_event_fn*, void*);
extern "C" int __wrap_ble_gap_adv_start(uint8_t ownType, const ble_addr_t* direct, int32_t duration,
                                        const ble_gap_adv_params* params, ble_gap_event_fn* callback, void* arg) {
  if (callback==guardedGap) return BLE_HS_EINVAL;
  originalGap=callback; originalGapArg=arg;
  return __real_ble_gap_adv_start(ownType,direct,duration,params,guardedGap,nullptr);
}

void HidKeyboardTransport::begin() {
  BLEDevice::init("ThumbRemote-HID-Test");
  BLESecurity::setCapability(ESP_IO_CAP_NONE);
  BLESecurity::setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  BLESecurity::setInitEncryptionKey(); BLESecurity::setRespEncryptionKey();
  BLESecurity::setForceAuthentication(false);
  BLEDevice::setSecurityCallbacks(&securityCallbacks);
  BLEServer* server=BLEDevice::createServer(); server->setCallbacks(&serverCallbacks);
  BLEHIDDevice* hid=new BLEHIDDevice(server);
  input=hid->inputReport(1); input->setCallbacks(&inputCallbacks);
  const auto zero=delivery.readable(); input->setValue(zero.data(),zero.size());
  ble_npl_event_init(&dispatchEvent,dispatchOnHost,nullptr);
  uint8_t led=0; hid->outputReport(1)->setValue(&led,1);
  hid->protocolMode()->setCallbacks(&modeCallbacks);
  hid->hidControl()->setCallbacks(&controlCallbacks);
  hid->manufacturer()->setValue("ThumbRemote prototype");
  // Prototype-local IDs, no claim of a registered vendor/product.
  const uint8_t pnp[]={0x02,0xff,0xff,0x01,0x00,0x01,0x00};
  hid->deviceInfo()->getCharacteristic(BLEUUID(uint16_t(0x2a50)))->setValue(pnp,sizeof(pnp));
  hid->hidInfo(0,0x02); hid->reportMap(reportMap,sizeof(reportMap));
  hid->startServices();
  // Library creates a mandatory battery service. USB supply has no measured percentage:
  // leave its value unset, do not publish a fictitious battery level.
  BLEAdvertisementData adv; adv.setFlags(0x06); adv.setAppearance(HID_KEYBOARD);
  adv.setCompleteServices(BLEUUID(uint16_t(0x1812)));
  BLEAdvertisementData response; response.setName("ThumbRemote-HID-Test");
  auto advertising=BLEDevice::getAdvertising();
  advertising->setAdvertisementData(adv); advertising->setScanResponseData(response);
  advertising->setScanResponse(true); advertising->start();
  Serial.println("HID advertising ThumbRemote-HID-Test; DISARMED; no automatic navigation keys");
}
HidLink HidKeyboardTransport::state() const { return snapshot(); }
void HidKeyboardTransport::poll() {
  static uint32_t lastGeneration=0;
  const auto s=snapshot();
  if (lastGeneration!=s.generation) {
    lastGeneration=s.generation;
    Serial.printf("HID_STATE generation=%lu connected=%d subscribed=%d encrypted=%d report=%d suspended=%d ready=%d\n",
      static_cast<unsigned long>(s.generation),s.connected,s.subscribed,s.encrypted,s.reportMode,s.suspended,s.ready());
  }
  portENTER_CRITICAL(&stateMux);
  const bool restart=advertiseAgain && !sharedLink.connected;
  if (restart) advertiseAgain=false;
  portEXIT_CRITICAL(&stateMux);
  if (restart) BLEDevice::startAdvertising();
}
bool HidKeyboardTransport::send(const hid_probe::Report& report, uint32_t generation) {
  portENTER_CRITICAL(&stateMux);
  const bool accepted=input && sharedLink.ready() && sharedLink.generation==generation && delivery.offer(report,generation,millis());
  portEXIT_CRITICAL(&stateMux);
  if (accepted) ble_npl_eventq_put(nimble_port_get_dflt_eventq(),&dispatchEvent);
  return accepted; // Submitted to the SDK host queue; not a delivery acknowledgement.
}
void HidKeyboardTransport::cancelPending() {
  portENTER_CRITICAL(&stateMux); delivery.cancel(); portEXIT_CRITICAL(&stateMux);
}
