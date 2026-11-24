#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleInitialPostWorldProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial post-world: exact29434 counter and normal66 caller recipe required";return false;};
        const std::array<u32,32> words{0x47465043,1,128,0,0x80029434,0x800294c8,0x80089554,0x800294cc,0x800294d0,0x800294d4,0x800294d8,0x800294e0,0xb0,0x66,0,1,0x8008a28c,0x80089aec,0x8002bb14,0x80036e24,0x8008972c,0x80089658,0x808d8394,0x806400b0,0x38030001,0x900400b0,0x88030066,0,2,3,4,5};
        if(!b.contains(0,128)||b.size!=128)return fail();
        for(unsigned i=0;i<words.size();++i)if(b.be32(4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
