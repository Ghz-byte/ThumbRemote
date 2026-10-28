#include "../dual_remote/dual_dispatch_gate.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <cstdio>
using namespace dual_remote;
int checks=0,failures=0;
#define CHECK(c) do {++checks;if(!(c)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#c);}} while(0)
int main(){
  std::mutex lock;bool cancelled=true;bool sent=false;
  CHECK(!runSerializedDispatch(lock,[&]{return cancelled;},[&]{sent=true;}));CHECK(!sent);
  cancelled=false;
  std::atomic<bool> entered(false),allow(false),cancelStarted(false),cancelDone(false);
  std::thread sender([&]{runSerializedDispatch(lock,[&]{return cancelled;},[&]{entered=true;while(!allow)std::this_thread::yield();sent=true;});});
  while(!entered)std::this_thread::yield();
  std::thread canceller([&]{cancelStarted=true;runSerializedCancel(lock,[&]{cancelled=true;cancelDone=true;});});
  while(!cancelStarted)std::this_thread::yield();
  CHECK(!cancelDone);allow=true;sender.join();canceller.join();
  CHECK(sent && cancelDone && cancelled);
  sent=false;CHECK(!runSerializedDispatch(lock,[&]{return cancelled;},[&]{sent=true;}));CHECK(!sent);
  std::printf("Dual dispatch serial: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
