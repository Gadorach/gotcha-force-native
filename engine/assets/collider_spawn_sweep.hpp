#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class ColliderSpawnSweepProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="spawn collider sweep: initial owned source continuation required";return false;};
        if(!b.contains(0,80)||b.size!=80||b.be32(0)!=0x47464355||b.be32(4)!=1||b.be32(8)!=80||b.be32(12))return fail();
        const std::array<u32,16> words{0x8008a28c,0x80089aec,0x8008a65c,0x80088e50,0x80089814,0x8003a5e4,0x8003b304,0x8003c614,0x8008aa40,0x8008a2bc,48,55,0,1,2,0};
        for(unsigned i=0;i<16;++i)if(b.be32(16+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
