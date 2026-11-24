#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class SourceHalfwordRngProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="halfword RNG: exact05630/C4 independent-stream recipe required";return false;};
        const std::array<u32,16> words{0x47465248,1,64,0,0x80005630,0x804360c4,0x800055fc,0x804360c6,0x804360c7,3,65536,255,16,1,0,0};
        if(!b.contains(0,64)||b.size!=64)return fail();
        for(unsigned i=0;i<words.size();++i)if(b.be32(4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
