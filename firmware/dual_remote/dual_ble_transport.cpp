#include "dual_ble_transport.h"
#include "dual_advertising.h"
#include "dual_service_setup.h"
#include "dual_bond_callbacks.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>
#include <host/ble_gap.h>
#include <host/ble_gatt.h>
#include <host/ble_hs_mbuf.h>
#include <host/ble_store.h>
#include <services/gatt/ble_svc_gatt.h>
#include <nimble/nimble_port.h>
#include <freertos/semphr.h>

namespace dual_remote {
namespace {
constexpr const char* kService="50101af6-adf1-4884-b512-754d9f980ed0";
constexpr const char* kEvent="d71a4ee9-9da8-4672-9e8f-e6478b2ca2b0";
constexpr const char* kStart="b5fb20eb-73b5-4cfc-ba46-d787489c1b80";
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
struct DispatchMutex {
  SemaphoreHandle_t handle=nullptr;
  bool begin(){handle=xSemaphoreCreateMutex();return handle!=nullptr;}
  void lock(){xSemaphoreTake(handle,portMAX_DELAY);}
  void unlock(){xSemaphoreGive(handle);}
} dispatchMutex;
TransportSnapshot exported;
// Only NimBLE host callbacks/events mutate these authoritative values after begin.
LinkSnapshot link;
Profile currentProfile;
RoutePolicy policy;
ProfileStore profileStore;
bool hidSubscribed=false,digitizerSubscribed=false,terminating=false,keyDown=false,digitizerDown=false;
HidModes hidModes;
bool cancelRequested=false,enrollPending=false,enrollInFlight=false,enrollComplete=false;
bool saveSucceeded=false,initialized=false,advertisingConfigured=false;
bool advFailed=false;
uint32_t lastAdvFailure=0;
LinkSnapshot enrollmentToken;
Profile savedProfile;
uint32_t sequence=0,readyAppEpoch=0,readyConnectionEpoch=0;
BLECharacteristic* hidInput=nullptr;
BLECharacteristic* digitizerInput=nullptr;
BLECharacteristic* appEvent=nullptr;
hid_probe::Delivery hidDelivery;
TxRequest hidRequest;
TxRequest digitizerRequest;
Digitizer digitizer;
AppDelivery appDelivery;
ble_npl_event dispatchEvent,stateEvent;
ble_gap_event_fn* originalGap=nullptr;
void* originalGapArg=nullptr;

void queue(ble_npl_event& event){ble_npl_eventq_put(nimble_port_get_dflt_eventq(),&event);}
void exportState(){
  const bool advertising=advertisingConfigured && BLEDevice::getAdvertising()->isAdvertising();
  portENTER_CRITICAL(&mux);
  exported.link=link;exported.profile=currentProfile;exported.route=policy.route();exported.generation=policy.generation();
  exported.advertising=advertising;
  portEXIT_CRITICAL(&mux);
}
void cancelSlots(){
  portENTER_CRITICAL(&mux);hidDelivery.cancel();digitizer.cancel();appDelivery.cancel();portEXIT_CRITICAL(&mux);
}
void terminate(const char* reason){
  if(terminating || !link.connected)return;
  terminating=true;link.appSession=0;++link.appEpoch;link.hidReady=false;++link.hidEpoch;link.digitizerReady=false;++link.digitizerEpoch;
  cancelSlots();policy.update(link,currentProfile);exportState();
  Serial.printf("DUAL_STOP reason=%s; no replay\n",reason);
  ble_gap_terminate(link.handle,BLE_ERR_REM_USER_CONN_TERM);
}
bool confirmedIdentity(const ble_gap_conn_desc& desc,PeerIdentity& peer){
  if(!desc.sec_state.encrypted || !desc.sec_state.bonded)return false;
  peer.type=desc.peer_id_addr.type&1;
  for(size_t i=0;i<6;++i)peer.address[i]=desc.peer_id_addr.val[i];
  if(!validIdentity(peer))return false;
  // Do not trust an unresolved OTA/RPA merely because it occupies peer_id_addr.
  ble_addr_t peers[CONFIG_BT_NIMBLE_MAX_BONDS];int count=0;
  if(ble_store_util_bonded_peers(peers,&count,CONFIG_BT_NIMBLE_MAX_BONDS)!=0)return false;
  for(int i=0;i<count;++i){
    if((peers[i].type&1)!=peer.type)continue;
    bool equal=true;for(size_t j=0;j<6;++j)equal=equal && peers[i].val[j]==peer.address[j];
    if(equal)return true;
  }
  return false;
}
void readSecurity(){
  if(!link.connected)return;
  ble_gap_conn_desc desc{};
  if(ble_gap_conn_find(link.handle,&desc)!=0){link.identityReady=false;link.encrypted=false;link.bonded=false;return;}
  const bool encrypted=desc.sec_state.encrypted,bonded=desc.sec_state.bonded;
  PeerIdentity peer;const bool identity=confirmedIdentity(desc,peer);
  if(link.encrypted && !encrypted){link.appSession=0;++link.appEpoch;}
  if(link.encrypted!=encrypted || link.bonded!=bonded || link.identityReady!=identity || (identity && link.peer!=peer)){++link.hidEpoch;++link.digitizerEpoch;}
  link.encrypted=encrypted;link.bonded=bonded;link.identityReady=identity;
  if(identity)link.peer=peer;
}
TxRequest requestFor(Route route){
  TxRequest r;r.route=route;r.handle=link.handle;r.connectionEpoch=link.connectionEpoch;
  r.routeGeneration=policy.generation();r.channelEpoch=route==Route::HidReady?link.hidEpoch:link.appEpoch;return r;
}
bool notify(const TxRequest& r){
  if(terminating || !DispatchGate::accepts(r,link,policy.route(),policy.generation()))return false;
  const bool hid=r.route==Route::HidReady;
  auto characteristic=hid?(r.digitizer?digitizerInput:hidInput):appEvent;
  if(!characteristic)return false;
  const uint8_t* bytes=hid?(r.digitizer?r.pen.data():r.hid.data()):r.app.data();
  const size_t length=hid?(r.digitizer?r.pen.size():r.hid.size()):r.app.size();
  auto packet=ble_hs_mbuf_from_flat(bytes,length);
  const int rc=packet?ble_gatts_notify_custom(link.handle,characteristic->getHandle(),packet):BLE_HS_ENOMEM;
  Serial.printf("DUAL_NOTIFY route=%s generation=%lu code=%u rc=%d\n",hid?(r.digitizer?"digitizer":"hid"):"app",static_cast<unsigned long>(policy.generation()),hid?(r.digitizer?r.pen[0]:r.hid[2]):r.app[9],rc);
  if(rc){terminate("SDK notify failed");return false;}
  if(hid){if(r.digitizer)digitizerDown=r.pen==digitizerPress();else keyDown=r.hid!=hid_probe::Report{};}
  return true;
}
bool neutral(){
  if(digitizerDown){auto p=requestFor(Route::HidReady);p.digitizer=true;p.channelEpoch=link.digitizerEpoch;p.pen=digitizerRelease();
    if(!notify(p)){terminate("digitizer neutral unavailable");return false;}}
  if(keyDown){auto r=requestFor(Route::HidReady);
    if(!notify(r)){terminate("neutral unavailable");return false;}}
  return true;
}
void recompute(){
  const auto oldGeneration=policy.generation();
  const bool base=!terminating && link.connected && link.encrypted && link.bonded && link.identityReady;
  link.hidReady=hidModes.keyboardReady(base,hidSubscribed);
  link.digitizerReady=hidModes.digitizerReady(base,digitizerSubscribed);
  policy.update(link,currentProfile);
  if(oldGeneration!=policy.generation()){
    cancelSlots();
    if((keyDown || digitizerDown) && !neutral())return;
    Serial.printf("DUAL_ROUTE generation=%lu route=%s enrolled=%d encrypted=%d bonded=%d identity=%d\n",
      static_cast<unsigned long>(policy.generation()),routeName(policy.route()),currentProfile.status==ProfileStatus::Valid,link.encrypted,link.bonded,link.identityReady);
  }
  exportState();
  if(terminating)return;
  if(policy.needsEnrollment()){
    portENTER_CRITICAL(&mux);
    if(!enrollInFlight){enrollmentToken=link;enrollPending=true;enrollInFlight=true;}
    portEXIT_CRITICAL(&mux);
  }
  if(policy.route()==Route::AppReady && (readyAppEpoch!=link.appEpoch || readyConnectionEpoch!=link.connectionEpoch)){
    auto r=requestFor(Route::AppReady);r.readyFrame=true;r.app=remote_ble::encodeReady(link.appSession);
    portENTER_CRITICAL(&mux);const bool accepted=appDelivery.offer(r,millis());portEXIT_CRITICAL(&mux);
    if(accepted){readyAppEpoch=link.appEpoch;readyConnectionEpoch=link.connectionEpoch;queue(dispatchEvent);}
  }
}
void dispatchLocked(){
  readSecurity();recompute();
  TxRequest app,hid,pen;hid_probe::Report report;DigitizerReport digitizerReport;
  portENTER_CRITICAL(&mux);
  const bool appPending=appDelivery.take(millis(),app);
  const bool hidPending=hidDelivery.take(true,link.connectionEpoch,millis(),report);
  if(hidPending){hid=hidRequest;hid.hid=report;}
  const bool penPending=digitizer.next(millis(),digitizerReport);
  if(penPending){pen=digitizerRequest;pen.pen=digitizerReport;}
  portEXIT_CRITICAL(&mux);
  if(hidPending && DispatchGate::accepts(hid,link,policy.route(),policy.generation()))notify(hid);
  if(penPending){
    if(DispatchGate::accepts(pen,link,policy.route(),policy.generation()))notify(pen);
    else {portENTER_CRITICAL(&mux);digitizer.cancel();portEXIT_CRITICAL(&mux);if(digitizerDown)neutral();}
  }
  if(appPending && DispatchGate::accepts(app,link,policy.route(),policy.generation())){
    if(!app.readyFrame)remote_ble::put32(app.app,5,++sequence);
    notify(app);
  }
  portENTER_CRITICAL(&mux);const bool moreApp=appDelivery.hasPending();portEXIT_CRITICAL(&mux);
  if(moreApp)queue(dispatchEvent);
}
void dispatchOnHost(ble_npl_event*){
  const bool ran=runSerializedDispatch(dispatchMutex,[](){
    portENTER_CRITICAL(&mux);const bool pending=cancelRequested;portEXIT_CRITICAL(&mux);return pending;
  },[](){dispatchLocked();});
  if(!ran)queue(stateEvent);
}
void stateOnHost(ble_npl_event*){
  bool cancel=false,complete=false,success=false;Profile result;LinkSnapshot token;
  portENTER_CRITICAL(&mux);
  cancel=cancelRequested;cancelRequested=false;
  complete=enrollComplete;
  if(complete){enrollComplete=false;enrollInFlight=false;success=saveSucceeded;result=savedProfile;token=enrollmentToken;}
  portEXIT_CRITICAL(&mux);
  if(complete){
    // Persisted identity survives even if the original request disconnected; READY never does.
    currentProfile=result;
    if(!success)currentProfile.status=ProfileStatus::Fault;
    Serial.printf("DUAL_ENROLL saved=%d request_current=%d\n",success,link.connected && link.connectionEpoch==token.connectionEpoch && link.appEpoch==token.appEpoch && link.peer==token.peer);
  }
  readSecurity();recompute();
  if(cancel){
    portENTER_CRITICAL(&mux);hidDelivery.cancel();digitizer.cancel();appDelivery.cancelDirections();portEXIT_CRITICAL(&mux);
    neutral();
  }
  if(advertisingConfigured){
    auto advertising=BLEDevice::getAdvertising();
    const bool wanted=!terminating && shouldAdvertise(link,currentProfile);
    const bool active=advertising->isAdvertising();
    const uint32_t now=millis();
    if(active!=wanted && (!advFailed || uint32_t(now-lastAdvFailure)>=1000)){
      const bool changed=wanted?advertising->start():advertising->stop();
      advFailed=!changed;if(advFailed)lastAdvFailure=now;
      Serial.printf("DUAL_ADV wanted=%d active=%d success=%d connected=%d\n",wanted,advertising->isAdvertising(),changed,link.connected);
    }
  }
  exportState();
}
void rejectPairing(){terminate("unsupported or repeat pairing; bonds preserved");}
int guardedGap(ble_gap_event* event,void*){
  if(event->type==BLE_GAP_EVENT_REPEAT_PAIRING){
    if(link.connected && link.handle==event->repeat_pairing.conn_handle)rejectPairing();
    else ble_gap_terminate(event->repeat_pairing.conn_handle,BLE_ERR_REM_USER_CONN_TERM);
    return BLE_GAP_REPEAT_PAIRING_IGNORE;
  }
  const int rc=originalGap?originalGap(event,originalGapArg):0;
  if(event->type==BLE_GAP_EVENT_IDENTITY_RESOLVED){readSecurity();recompute();}
  return rc;
}
BondCallbacks<BLEDeviceCallbacks,ble_store_status_event,BLE_HS_ENOMEM> deviceCallbacks;
class ServerCallbacks final:public BLEServerCallbacks {
  void onConnect(BLEServer* server,ble_gap_conn_desc* desc) override {
    if(link.connected){server->disconnect(desc->conn_handle);queue(stateEvent);return;}
    const uint32_t next=link.connectionEpoch+1;link=LinkSnapshot{};link.connectionEpoch=next;link.handle=desc->conn_handle;link.connected=true;
    ++link.appEpoch;++link.hidEpoch;++link.digitizerEpoch;hidSubscribed=false;digitizerSubscribed=false;hidModes=HidModes{};terminating=false;keyDown=false;digitizerDown=false;sequence=0;
    readyConnectionEpoch=0;readyAppEpoch=0;
    BLEDevice::getAdvertising()->stop();readSecurity();recompute();
    int rc=0;const bool started=BLESecurity::startSecurity(link.handle,&rc);
    Serial.printf("DUAL_SECURITY attempted=%d rc=%d\n",started,rc);
  }
  void onDisconnect(BLEServer*,ble_gap_conn_desc* desc) override {
    if(!link.connected || link.handle!=desc->conn_handle)return;
    const uint32_t next=link.connectionEpoch+1;link=LinkSnapshot{};link.connectionEpoch=next;
    keyDown=false;digitizerDown=false;terminating=false;hidSubscribed=false;digitizerSubscribed=false;sequence=0;
    recompute();queue(stateEvent);
  }
} serverCallbacks;
class SecurityCallbacks final:public BLESecurityCallbacks {
  bool onSecurityRequest() override{return link.connected && !terminating;}
  uint32_t onPassKeyRequest() override{rejectPairing();return 0;}
  void onPassKeyNotify(uint32_t) override{rejectPairing();}
  bool onConfirmPIN(uint32_t) override{rejectPairing();return false;}
  bool onAuthorizationRequest(uint16_t,uint16_t,bool) override{return false;}
  void onAuthenticationComplete(ble_gap_conn_desc* desc) override{
    if(desc && link.connected && desc->conn_handle==link.handle){readSecurity();recompute();}
  }
} securityCallbacks;
class AppSubscription final:public BLECharacteristicCallbacks {
  void onSubscribe(BLECharacteristic*,ble_gap_conn_desc* desc,uint16_t value) override{
    if(!link.connected || desc->conn_handle!=link.handle)return;
    const bool enabled=value==1;
    if(enabled)link.appClaimed=true;
    if(enabled!=link.appSubscribed){link.appSubscribed=enabled;link.appSession=0;++link.appEpoch;sequence=0;}
    readSecurity();recompute();
  }
} appSubscription;
class StartCallbacks final:public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c,ble_gap_conn_desc* desc) override{
    if(!link.connected || desc->conn_handle!=link.handle)return;
    const String value=c->getValue();uint32_t session=0;
    if(!remote_ble::decodeStart(reinterpret_cast<const uint8_t*>(value.c_str()),value.length(),session))return;
    link.appClaimed=true;
    if(link.appSubscribed && session!=link.appSession){link.appSession=session;++link.appEpoch;sequence=0;}
    readSecurity();recompute();
  }
} startCallbacks;
class HidInputCallbacks final:public BLECharacteristicCallbacks {
  void onSubscribe(BLECharacteristic*,ble_gap_conn_desc* desc,uint16_t value) override{
    if(!link.connected || desc->conn_handle!=link.handle)return;
    if(hidSubscribed!=(value==1)){hidSubscribed=value==1;++link.hidEpoch;}
    readSecurity();recompute();
  }
} hidInputCallbacks;
class DigitizerCallbacks final:public BLECharacteristicCallbacks {
  void onSubscribe(BLECharacteristic*,ble_gap_conn_desc* desc,uint16_t value) override{
    if(!link.connected || desc->conn_handle!=link.handle)return;
    const bool enabled=value==1;
    if(enabled!=digitizerSubscribed){
      digitizerSubscribed=enabled;++link.digitizerEpoch;
      portENTER_CRITICAL(&mux);digitizer.cancel();portEXIT_CRITICAL(&mux);
      if(!enabled && digitizerDown){terminate("digitizer unsubscribed while down");return;}
    }
    readSecurity();recompute();
  }
} digitizerCallbacks;
class ModeCallbacks final:public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c,ble_gap_conn_desc* desc) override{
    if(!link.connected || desc->conn_handle!=link.handle)return;
    const String value=c->getValue();const bool enabled=value.length()==1 && uint8_t(value[0])==1;
    if(hidModes.keyboardMode!=enabled){hidModes.keyboardMode=enabled;++link.hidEpoch;}
    recompute();
  }
} modeCallbacks;
class ControlCallbacks final:public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c,ble_gap_conn_desc* desc) override{
    if(!link.connected || desc->conn_handle!=link.handle)return;
    const String value=c->getValue();const bool sleep=!(value.length()==1 && uint8_t(value[0])==1);
    if(hidModes.keyboardSuspended!=sleep){hidModes.keyboardSuspended=sleep;++link.hidEpoch;}
    recompute();
  }
} controlCallbacks;
class DigitizerModeCallbacks final:public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c,ble_gap_conn_desc* desc) override{
    if(!link.connected || desc->conn_handle!=link.handle)return;
    const String value=c->getValue();const bool enabled=value.length()==1 && uint8_t(value[0])==1;
    if(hidModes.digitizerMode!=enabled){hidModes.digitizerMode=enabled;++link.digitizerEpoch;}
    recompute();
  }
} digitizerModeCallbacks;
class DigitizerControlCallbacks final:public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c,ble_gap_conn_desc* desc) override{
    if(!link.connected || desc->conn_handle!=link.handle)return;
    const String value=c->getValue();const bool sleep=!(value.length()==1 && uint8_t(value[0])==1);
    if(hidModes.digitizerSuspended!=sleep){hidModes.digitizerSuspended=sleep;++link.digitizerEpoch;}
    recompute();
  }
} digitizerControlCallbacks;
uint8_t reportMap[]={
  0x05,0x01,0x09,0x06,0xa1,0x01,0x85,0x01,
  0x05,0x07,0x19,0xe0,0x29,0xe7,0x15,0x00,0x25,0x01,
  0x75,0x01,0x95,0x08,0x81,0x02,0x95,0x01,0x75,0x08,0x81,0x01,
  0x95,0x05,0x75,0x01,0x05,0x08,0x19,0x01,0x29,0x05,0x91,0x02,
  0x95,0x01,0x75,0x03,0x91,0x01,0x95,0x06,0x75,0x08,0x15,0x00,0x25,0x65,
  0x05,0x07,0x19,0x00,0x29,0x65,0x81,0x00,0xc0
};
}
const char* routeName(Route route){
  switch(route){case Route::AppWaiting:return "app_waiting";case Route::AppReady:return "app";case Route::HidReady:return "hid_arrow";case Route::Blocked:return "blocked";default:return "none";}
}
void DualBleTransport::begin(){
  currentProfile=profileStore.load();
  if(!dispatchMutex.begin()){Serial.println("DUAL_DISPATCH_MUTEX_FAILED");return;}
  if(!initializeDualRuntime([](){return BLEDevice::init("ThumbRemote");},[](){
    ble_npl_event_init(&dispatchEvent,dispatchOnHost,nullptr);ble_npl_event_init(&stateEvent,stateOnHost,nullptr);
  })){Serial.println("DUAL_BLE_INIT_FAILED");return;}
  BLEDevice::setDeviceCallbacks(&deviceCallbacks);
  BLESecurity::setCapability(ESP_IO_CAP_NONE);BLESecurity::setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  BLESecurity::setInitEncryptionKey();BLESecurity::setRespEncryptionKey();BLESecurity::setForceAuthentication(false);
  BLEDevice::setSecurityCallbacks(&securityCallbacks);
  auto server=BLEDevice::createServer();server->setCallbacks(&serverCallbacks);
  auto hid=new BLEHIDDevice(server);hidInput=hid->inputReport(1);hidInput->setCallbacks(&hidInputCallbacks);
  const hid_probe::Report zero{};hidInput->setValue(zero.data(),zero.size());
  uint8_t led=0;hid->outputReport(1)->setValue(&led,1);
  // The fixed SDK deduplicates same-UUID characteristics inside one service.
  // A second standard HID service keeps each 0x2a4d in its own service.
  auto penService=server->createService(BLEUUID(uint16_t(0x1812)),40,1);
  auto penInfo=penService->createCharacteristic(uint16_t(0x2a4a),BLECharacteristic::PROPERTY_READ);
  const uint8_t infoValue[]={0x11,0x01,0x00,0x02};penInfo->setValue(infoValue,sizeof(infoValue));
  auto penMap=penService->createCharacteristic(uint16_t(0x2a4b),BLECharacteristic::PROPERTY_READ);
  penMap->setValue(digitizerReportMap,digitizerReportMapSize);
  auto penControl=penService->createCharacteristic(uint16_t(0x2a4c),BLECharacteristic::PROPERTY_WRITE_NR);
  penControl->setCallbacks(&digitizerControlCallbacks);
  auto penMode=penService->createCharacteristic(uint16_t(0x2a4e),BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE_NR);
  const uint8_t modeValue=1;penMode->setValue(&modeValue,1);penMode->setCallbacks(&digitizerModeCallbacks);
  digitizerInput=penService->createCharacteristic(uint16_t(0x2a4d),BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  auto reference=new BLEDescriptor(BLEUUID(uint16_t(0x2908)));const uint8_t referenceValue[]={2,1};
  reference->setValue(referenceValue,sizeof(referenceValue));digitizerInput->addDescriptor(reference);
  digitizerInput->setCallbacks(&digitizerCallbacks);
  const auto penZero=digitizerRelease();digitizerInput->setValue(penZero.data(),penZero.size());
  hid->protocolMode()->setCallbacks(&modeCallbacks);hid->hidControl()->setCallbacks(&controlCallbacks);
  hid->manufacturer()->setValue("ThumbRemote prototype");
  const uint8_t pnp[]={0x02,0xff,0xff,0x01,0x00,0x01,0x00};
  hid->deviceInfo()->getCharacteristic(BLEUUID(uint16_t(0x2a50)))->setValue(pnp,sizeof(pnp));
  hid->hidInfo(0,0x02);hid->reportMap(reportMap,sizeof(reportMap));
  auto service=server->createService(kService);
  appEvent=service->createCharacteristic(kEvent,BLECharacteristic::PROPERTY_NOTIFY);appEvent->setCallbacks(&appSubscription);
  auto start=service->createCharacteristic(kStart,BLECharacteristic::PROPERTY_WRITE);start->setCallbacks(&startCallbacks);
  if(!startCompositeServices(*hid->deviceInfo(),*hid->hidService(),*penService,*hid->batteryService(),*service,*server)){
    Serial.println("DUAL_SERVICE_REGISTER_FAILED");return;
  }
  BLEUUID appId(kService),eventId(kEvent),startId(kStart),hidId(uint16_t(0x1812));
  uint16_t appHandle=0,startHandle=0,hidHandle=0;
  const int eventRc=ble_gatts_find_chr(&appId.getNative()->u,&eventId.getNative()->u,nullptr,&appHandle);
  const int startRc=ble_gatts_find_chr(&appId.getNative()->u,&startId.getNative()->u,nullptr,&startHandle);
  const int hidRc=ble_gatts_find_svc(&hidId.getNative()->u,&hidHandle);
  const uint16_t values[]={hidInput->getHandle(),digitizerInput->getHandle(),penInfo->getHandle(),penMap->getHandle(),penControl->getHandle(),penMode->getHandle(),appEvent->getHandle(),start->getHandle()};
  const bool tableReady=eventRc==0 && startRc==0 && hidRc==0 && validDistinctHandles(values) &&
    appHandle==appEvent->getHandle() && startHandle==start->getHandle();
  if(!tableReady){
    Serial.printf("DUAL_GATT_TABLE_FAILED event_rc=%d start_rc=%d hid_rc=%d app=%u/%u start=%u/%u keyboard=%u pen=%u hid=%u; no advertising\n",
      eventRc,startRc,hidRc,appHandle,appEvent->getHandle(),startHandle,start->getHandle(),hidInput->getHandle(),digitizerInput->getHandle(),hidHandle);return;
  }
  Serial.printf("DUAL_GATT_READY app_event=%u app_start=%u hid_input=%u digitizer_input=%u\n",appHandle,startHandle,hidInput->getHandle(),digitizerInput->getHandle());
  ble_svc_gatt_changed(1,0xffff);
  auto advBytes=advertisingPayload();auto scanBytes=scanResponsePayload();
  BLEAdvertisementData adv,response;adv.addData(reinterpret_cast<char*>(advBytes.data()),advBytes.size());response.addData(reinterpret_cast<char*>(scanBytes.data()),scanBytes.size());
  auto advertising=BLEDevice::getAdvertising();
  advertisingConfigured=advertising->setAdvertisementData(adv) && advertising->setScanResponseData(response);
  advertising->setScanResponse(true);policy.update(link,currentProfile);initialized=true;
  if(!advertisingConfigured || !advertising->start())Serial.println("DUAL_ADV_FAILED");
  exportState();
  Serial.printf("DUAL_STARTED profile=%s adv_bytes=%u; first use enroll OPPO App; no historical input\n",currentProfile.status==ProfileStatus::Valid?"registered":currentProfile.status==ProfileStatus::Missing?"missing":"fault",unsigned(advBytes.size()));
}
TransportSnapshot DualBleTransport::snapshot() const{
  portENTER_CRITICAL(&mux);const auto result=exported;portEXIT_CRITICAL(&mux);return result;
}
void DualBleTransport::poll(){
  if(!initialized)return;
  bool save=false;LinkSnapshot token;
  portENTER_CRITICAL(&mux);save=enrollPending;if(save){enrollPending=false;token=enrollmentToken;}portEXIT_CRITICAL(&mux);
  if(save){
    const bool success=profileStore.saveFirst(token.peer);const auto result=profileStore.load();
    portENTER_CRITICAL(&mux);saveSucceeded=success;savedProfile=result;enrollComplete=true;portEXIT_CRITICAL(&mux);
    queue(stateEvent);
  }
  static uint32_t last=0;const auto now=millis();if(uint32_t(now-last)>=100){last=now;queue(stateEvent);}
  static uint32_t lastPen=0;
  if(uint32_t(now-lastPen)>=10){lastPen=now;portENTER_CRITICAL(&mux);const bool active=digitizer.active();portEXIT_CRITICAL(&mux);if(active)queue(dispatchEvent);}
}
bool DualBleTransport::send(const Action& action){
  if(!initialized)return false;
  const auto s=snapshot();TxRequest r;r.route=action.route;r.handle=s.link.handle;r.connectionEpoch=s.link.connectionEpoch;
  r.routeGeneration=action.generation;r.channelEpoch=action.route==Route::HidReady?s.link.hidEpoch:s.link.appEpoch;
  r.hid=action.report;r.app=encodeAppAction(s.link.appSession,action.direction,action.tap);
  if(action.route==Route::HidReady && action.tap){r.digitizer=true;r.channelEpoch=s.link.digitizerEpoch;r.pen=digitizerPress();}
  if(!DispatchGate::accepts(r,s.link,s.route,s.generation))return false;
  portENTER_CRITICAL(&mux);
  const bool accepted=action.route==Route::HidReady?(r.digitizer?digitizer.offer(millis()):hidDelivery.offer(r.hid,r.connectionEpoch,millis())):appDelivery.offer(r,millis());
  if(accepted && action.route==Route::HidReady){if(r.digitizer)digitizerRequest=r;else hidRequest=r;}
  portEXIT_CRITICAL(&mux);
  if(!accepted && r.digitizer){Serial.println("DUAL_DROP digitizer queue full");return true;}
  if(accepted)queue(dispatchEvent);return accepted;
}
void DualBleTransport::cancelInput(){
  if(!initialized)return;
  runSerializedCancel(dispatchMutex,[](){
    portENTER_CRITICAL(&mux);cancelRequested=true;hidDelivery.cancel();digitizer.cancel();appDelivery.cancelDirections();portEXIT_CRITICAL(&mux);
  });
  queue(stateEvent);
}
}
extern "C" int __real_ble_gap_adv_start(uint8_t,const ble_addr_t*,int32_t,const ble_gap_adv_params*,ble_gap_event_fn*,void*);
extern "C" int __wrap_ble_gap_adv_start(uint8_t ownType,const ble_addr_t* direct,int32_t duration,const ble_gap_adv_params* params,ble_gap_event_fn* callback,void* arg){
  using namespace dual_remote;
  if(callback==guardedGap)return BLE_HS_EINVAL;
  originalGap=callback;originalGapArg=arg;
  return __real_ble_gap_adv_start(ownType,direct,duration,params,guardedGap,nullptr);
}
