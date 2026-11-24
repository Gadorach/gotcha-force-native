#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleContactPrefixProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="battle contact prefix: exact first source reset/sphere pass required";return false;};
        if(!b.contains(0,80)||b.size!=80||b.be32(0)!=0x47464350||b.be32(4)!=1||b.be32(8)!=80||b.be32(12))return fail();
        const std::array<u32,16> words{0x8002bb14,0x8002cda4,0x8002cc78,0x8002cf80,0x8003a05c,0x80039f6c,0x3dcccccdu,0xc61c4000u,40,32,0,16,8,24,12,6};
        for(unsigned i=0;i<16;++i)if(b.be32(16+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
