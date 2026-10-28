#pragma once
namespace dual_remote {
// Bind the same override to the real SDK base and to a host test SDK substitute.
// Never delegate to the SDK default, whose storage handler evicts old bonds.
template<class Base,class StoreEvent,int NoSpace>
class BondCallbacks:public Base {
 public:
  int onStoreStatus(StoreEvent*,void*) override{return NoSpace;}
};
}
