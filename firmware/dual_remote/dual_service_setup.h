#pragma once
#include <cstdint>
#include <cstddef>
namespace dual_remote {
template<size_t N> bool validDistinctHandles(const uint16_t (&handles)[N]){
  for(size_t i=0;i<N;++i){
    if(handles[i]==0 || handles[i]==0xffff)return false;
    for(size_t j=0;j<i;++j)if(handles[i]==handles[j])return false;
  }
  return true;
}
template<class InitBle,class InitEvents>
bool initializeDualRuntime(InitBle initBle,InitEvents initEvents){
  if(!initBle())return false;
  initEvents();return true;
}
template<class Service,class Server>
bool startDualServices(Service& info,Service& hid,Service& battery,Service& app,Server& server){
  // BLEHIDDevice::startServices also starts the server. Register its three
  // public services explicitly, append App, then freeze the entire table once.
  if(!info.start() || !hid.start() || !battery.start() || !app.start())return false;
  server.start();return true;
}
template<class Service,class Server>
bool startCompositeServices(Service& info,Service& keyboard,Service& digitizer,Service& battery,Service& app,Server& server){
  if(!info.start() || !keyboard.start() || !digitizer.start() || !battery.start() || !app.start())return false;
  server.start();return true;
}
}
