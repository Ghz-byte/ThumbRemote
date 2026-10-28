#include "../dual_remote/dual_digitizer.h"
#include <cstdio>
using namespace dual_remote;
int checks=0,failures=0;
#define CHECK(c) do {++checks;if(!(c)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#c);}} while(0)
int main(){
  Digitizer d;DigitizerReport report{};
  CHECK(!d.next(0,report));CHECK(d.offer(10));CHECK(d.next(10,report));
  CHECK(report==digitizerPress());CHECK(report[0]==5 && report[1]==0 && report[2]==0x40 && report[3]==0 && report[4]==0x40);
  CHECK(d.offer(30));CHECK(!d.offer(31));CHECK(!d.next(69,report));
  CHECK(d.next(70,report));CHECK(report==digitizerRelease());
  CHECK(d.next(70,report));CHECK(report==digitizerPress());
  CHECK(d.next(130,report));CHECK(report==digitizerRelease());CHECK(!d.next(130,report));
  CHECK(d.offer(200));CHECK(d.next(200,report));CHECK(d.offer(210));
  CHECK(d.next(600,report));CHECK(report==digitizerRelease());CHECK(!d.next(600,report));
  CHECK(d.offer(700));CHECK(d.next(700,report));CHECK(d.offer(710));d.cancel();CHECK(!d.next(800,report));
  CHECK(d.offer(900));CHECK(d.offer(901));CHECK(!d.offer(902));
  CHECK(d.next(902,report));CHECK(report==digitizerPress());
  CHECK(d.next(962,report));CHECK(report==digitizerRelease());
  CHECK(d.next(962,report));CHECK(report==digitizerPress());
  CHECK(d.next(1022,report));CHECK(report==digitizerRelease());
  CHECK(digitizerReportMapSize>20);CHECK(digitizerReportMap[0]==0x05 && digitizerReportMap[1]==0x0d);
  std::printf("Dual digitizer: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
