#include "../dual_remote/dual_bond_callbacks.h"
#include <cstdio>
struct Event{int code;};
struct SdkBase {
  int deletions=0;
  virtual int onStoreStatus(Event*,void*){++deletions;return 0;}
};
int main(){
  dual_remote::BondCallbacks<SdkBase,Event,6> callback;SdkBase& sdk=callback;
  Event overflow{1},full{2};
  const bool ok=sdk.onStoreStatus(&overflow,nullptr)==6 && sdk.onStoreStatus(&full,nullptr)==6 && sdk.deletions==0;
  std::printf("Dual bond callbacks: 1 check, %d failures\n",ok?0:1);return ok?0:1;
}
