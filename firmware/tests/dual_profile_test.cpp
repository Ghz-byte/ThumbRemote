#include "../dual_remote/dual_profile.h"
#include <cstdio>
using namespace dual_remote;
int checks=0, failures=0;
#define CHECK(c) do {++checks; if(!(c)){++failures; std::printf("FAIL %d: %s\n",__LINE__,#c);}} while(0)
int main(){
  PeerIdentity peer; peer.type=0; peer.address={{1,2,3,4,5,6}};
  CHECK(validIdentity(peer));
  const auto bytes=encodeProfile(peer);
  CHECK(bytes.size()==16); CHECK(bytes[0]=='T' && bytes[3]=='P' && bytes[4]==1);
  CHECK(decodeProfile(bytes.data(),bytes.size()).status==ProfileStatus::Valid);
  CHECK(decodeProfile(bytes.data(),bytes.size()).appPeer==peer);
  CHECK(decodeProfile(nullptr,0).status==ProfileStatus::Fault);
  CHECK(decodeProfile(bytes.data(),15).status==ProfileStatus::Fault);
  for(size_t i=0;i<bytes.size();++i){auto b=bytes; b[i]^=1; CHECK(decodeProfile(b.data(),b.size()).status==ProfileStatus::Fault);}
  auto random=peer; random.type=1; CHECK(!validIdentity(random));
  random.address[5]=0xc1; CHECK(validIdentity(random));
  auto randomBytes=encodeProfile(random); CHECK(decodeProfile(randomBytes.data(),16).appPeer==random);
  auto invalid=peer; invalid.type=2; CHECK(!validIdentity(invalid));
  invalid=peer; invalid.address.fill(0); CHECK(!validIdentity(invalid));
  invalid.address.fill(255); CHECK(!validIdentity(invalid));
  const uint8_t checkText[]={'1','2','3','4','5','6','7','8','9'};
  CHECK(crc32(checkText,sizeof(checkText))==0xcbf43926u);
  Profile missing; CHECK(missing.status==ProfileStatus::Missing);
  CHECK(enrollmentAllowed(missing,peer));
  Profile known; known.status=ProfileStatus::Valid; known.appPeer=peer;
  CHECK(enrollmentAllowed(known,peer)); peer.address[0]=9; CHECK(!enrollmentAllowed(known,peer));
  missing.status=ProfileStatus::Fault; CHECK(!enrollmentAllowed(missing,peer));
  std::printf("Dual profile: %d checks, %d failures\n",checks,failures); return failures?1:0;
}
