#include "../dual_remote/dual_advertising.h"
#include <cstdio>
using namespace dual_remote;
int main(){
  const auto a=advertisingPayload();const auto s=scanResponsePayload();
  const bool ok=a.size()==29 && a[0]==2 && a[1]==1 && a[2]==6 &&
    a[3]==17 && a[4]==7 && a[5]==0xd0 && a[20]==0x50 &&
    a[21]==3 && a[22]==3 && a[23]==0x12 && a[24]==0x18 &&
    a[25]==3 && a[26]==0x19 && a[27]==0xc1 && a[28]==0x03 &&
    s.size()==13 && s[0]==12 && s[1]==9 && s[2]=='T' && s[12]=='e';
  std::printf("Dual advertising: 1 check, %d failures\n",ok?0:1);return ok?0:1;
}
