#pragma once
#include "dual_profile.h"
namespace dual_remote {
class ProfileIo {
 public:
  virtual ~ProfileIo(){}
  virtual Profile read()=0;
  virtual bool write(const ProfileBytes& bytes)=0;
};
class ProfileStore {
 public:
  ProfileStore();
  explicit ProfileStore(ProfileIo& io):io_(io){}
  Profile load(){return io_.read();}
  bool saveFirst(const PeerIdentity& peer){
    const auto old=load();
    if(!enrollmentAllowed(old,peer))return false;
    if(old.status==ProfileStatus::Valid)return true;
    const auto bytes=encodeProfile(peer);
    if(!io_.write(bytes))return false;
    const auto saved=load();
    return saved.status==ProfileStatus::Valid && saved.appPeer==peer;
  }
 private:
  ProfileIo& io_;
};
}
