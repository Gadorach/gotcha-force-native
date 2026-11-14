#pragma once
#include "engine/core/types.hpp"
#include <string>
namespace gf {
//40FF4→ARAM→4104C slot22 byte recipe; NOT a pool1 allocation/current-DMA ACK.
class BattlePreparationModelProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};if(!b.data||b.size!=64||b.be32(0)!=0x47465042||b.be32(4)!=1||b.be32(8)!=64||b.be32(12)<32||b.be32(12)>(4u<<20)||b.be16(20)!=0xaa0||b.data[22]!=22||b.data[23]!=1||b.be16(24)!=32||b.be16(26)!=32||b.be32(28)!=0x1ee800||b.be32(32)!=0x400||b.be32(36)!=0x80006e3c||b.be32(40)!=0x80006e8c||b.be32(44)!=0xa36800||b.be32(48)!=0x803bb384||b.be32(52)!=68||b.be32(56)||b.be32(60)){if(error)*error="battle preparation models: byte/slot/archive recipe";return false;}
        bytes_=b.be32(12);hash_=b.be32(16);ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}u32 bytes()const{return bytes_;}u32 hash()const{return hash_;}
    bool matches(ByteView b)const{if(!ready_||!b.data||b.size!=bytes_)return false;u32 h=2166136261u;for(std::size_t i=0;i<b.size;++i)h=(h^b.data[i])*16777619u;return h==hash_;}
private:u32 bytes_=0,hash_=0;bool ready_=false;
};
} // namespace gf
