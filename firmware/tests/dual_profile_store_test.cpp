#include "../dual_remote/dual_profile_store.h"
#include <cstdio>
using namespace dual_remote;
int checks=0,failures=0;
#define CHECK(c) do {++checks;if(!(c)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#c);}} while(0)
struct MemoryIo:ProfileIo {
  Profile stored;bool failWrite=false,corruptReadback=false;int writes=0;
  Profile read() override{return stored;}
  bool write(const ProfileBytes& bytes) override{
    ++writes;if(failWrite)return false;
    stored=decodeProfile(bytes.data(),bytes.size());
    if(corruptReadback)stored.status=ProfileStatus::Fault;
    return true;
  }
};
int main(){
  PeerIdentity peer;peer.address={{1,2,3,4,5,6}};
  MemoryIo io;ProfileStore store(io);CHECK(store.load().status==ProfileStatus::Missing);
  CHECK(store.saveFirst(peer));CHECK(io.writes==1);CHECK(store.load().appPeer==peer);
  CHECK(store.saveFirst(peer));CHECK(io.writes==1);
  auto other=peer;other.address[0]=9;CHECK(!store.saveFirst(other));CHECK(io.writes==1);CHECK(store.load().appPeer==peer);
  io.stored.status=ProfileStatus::Fault;CHECK(!store.saveFirst(peer));CHECK(io.writes==1);
  io.stored.status=ProfileStatus::Missing;io.failWrite=true;CHECK(!store.saveFirst(peer));CHECK(store.load().status==ProfileStatus::Missing);
  io.failWrite=false;io.corruptReadback=true;CHECK(!store.saveFirst(peer));CHECK(store.load().status==ProfileStatus::Fault);
  std::printf("Dual store: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
