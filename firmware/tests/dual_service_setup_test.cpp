#include "../dual_remote/dual_service_setup.h"
#include <cstdio>
#include <cstdint>
// Fixed SDK semantics: HID startServices starts the entire server once. Later additions
// do not enter its active service/notification map until a disconnect rebuild.
struct Server {
  unsigned registered=0,active=0,order=0;int starts=0;
  void start(){if(starts)return;++starts;active=registered;}
};
struct Service {
  Server& server;unsigned id;bool fail=false;
  Service(Server& s,unsigned i):server(s),id(i){}
  bool start(){if(fail)return false;server.registered|=id;server.order=(server.order<<4)|id;return true;}
};
int main(){
  Server server;Service info(server,1),hid(server,2),battery(server,4),app(server,8);
  const bool started=dual_remote::startDualServices(info,hid,battery,app,server);
  const bool ok=started && server.active==15 && server.starts==1 && server.order==0x1248;
  Server bad;Service a(bad,1),b(bad,2),c(bad,4),d(bad,8);b.fail=true;
  const bool stopped=!dual_remote::startDualServices(a,b,c,d,bad) && bad.starts==0;
  bool bleReady=false,eventsReady=false,nullRuntimeAccess=false;
  const bool runtime=dual_remote::initializeDualRuntime([&](){bleReady=true;return true;},[&](){if(!bleReady)nullRuntimeAccess=true;else eventsReady=true;});
  const bool ordered=runtime && eventsReady && !nullRuntimeAccess;
  bool called=false;
  const bool initFailed=!dual_remote::initializeDualRuntime([](){return false;},[&](){called=true;}) && !called;
  Server composite;Service ci(composite,1),ck(composite,2),cp(composite,4),cb(composite,8),ca(composite,16);
  const bool both=dual_remote::startCompositeServices(ci,ck,cp,cb,ca,composite) && composite.active==31 && composite.starts==1;
  Server failed;Service fi(failed,1),fk(failed,2),fp(failed,4),fb(failed,8),fa(failed,16);fp.fail=true;
  const bool penFailed=!dual_remote::startCompositeServices(fi,fk,fp,fb,fa,failed) && failed.starts==0;
  const uint16_t valid[]={1,2,3,4,5},zero[]={1,2,0,4,5},invalid[]={1,2,0xffff,4,5},same[]={1,2,2,4,5};
  const bool handles=dual_remote::validDistinctHandles(valid) && !dual_remote::validDistinctHandles(zero) &&
    !dual_remote::validDistinctHandles(invalid) && !dual_remote::validDistinctHandles(same);
  const int failures=(ok?0:1)+(stopped?0:1)+(ordered?0:1)+(initFailed?0:1)+(both?0:1)+(penFailed?0:1)+(handles?0:1);
  std::printf("Dual service startup: 7 checks, %d failures\n",failures);return failures?1:0;
}
