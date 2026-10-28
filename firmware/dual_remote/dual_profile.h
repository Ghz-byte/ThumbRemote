#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

namespace dual_remote {
struct PeerIdentity {
  uint8_t type=0;
  std::array<uint8_t,6> address{};
  bool operator==(const PeerIdentity& other) const { return type==other.type && address==other.address; }
  bool operator!=(const PeerIdentity& other) const { return !(*this==other); }
};
inline bool validIdentity(const PeerIdentity& peer) {
  if(peer.type>1 || (peer.type==1 && (peer.address[5]&0xc0)!=0xc0)) return false;
  bool zero=true, ones=true;
  for(auto b:peer.address){zero=zero && b==0;ones=ones && b==255;}
  return !zero && !ones;
}
enum class ProfileStatus { Missing, Valid, Fault };
struct Profile { ProfileStatus status=ProfileStatus::Missing; PeerIdentity appPeer; };
using ProfileBytes=std::array<uint8_t,16>;
inline uint32_t crc32(const uint8_t* bytes,size_t length) {
  uint32_t crc=0xffffffffu;
  for(size_t i=0;i<length;++i){crc^=bytes[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320u:0);}
  return ~crc;
}
inline ProfileBytes encodeProfile(const PeerIdentity& peer) {
  ProfileBytes b{};b[0]='T';b[1]='R';b[2]='A';b[3]='P';b[4]=1;b[5]=peer.type;
  for(size_t i=0;i<6;++i)b[6+i]=peer.address[i];
  const uint32_t crc=crc32(b.data(),12);
  for(size_t i=0;i<4;++i)b[12+i]=uint8_t(crc>>(8*i));
  return b;
}
inline Profile decodeProfile(const uint8_t* bytes,size_t length) {
  Profile p;p.status=ProfileStatus::Fault;
  if(!bytes || length!=16 || bytes[0]!='T' || bytes[1]!='R' || bytes[2]!='A' || bytes[3]!='P' || bytes[4]!=1)return p;
  uint32_t stored=0;for(size_t i=0;i<4;++i)stored|=uint32_t(bytes[12+i])<<(8*i);
  if(crc32(bytes,12)!=stored)return p;
  p.appPeer.type=bytes[5];for(size_t i=0;i<6;++i)p.appPeer.address[i]=bytes[6+i];
  if(validIdentity(p.appPeer))p.status=ProfileStatus::Valid;
  return p;
}
inline bool enrollmentAllowed(const Profile& p,const PeerIdentity& peer) {
  return validIdentity(peer) && (p.status==ProfileStatus::Missing || (p.status==ProfileStatus::Valid && p.appPeer==peer));
}
}
