#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleSidePublicationProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="side publication: first camera-invariant source prefix required";return false;};
        if(!b.contains(0,80)||b.size!=80||b.be32(0)!=0x47465350||b.be32(4)!=1||b.be32(8)!=80||b.be32(12))return fail();
        const std::array<u32,16> words{0x80089eb4,0x800820c8,0x800076d0,0x80007cac,0x800097b4,0x8008b290,0x3f800000,0x3f000000,0x3e800000,0x3bf5c28f,7,0x0f,0x47,1,0xff,0};
        for(unsigned i=0;i<words.size();++i)if(b.be32(16+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
