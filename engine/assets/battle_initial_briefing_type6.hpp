#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleInitialBriefingType6Profile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="briefing type6: exact original argument0/final-widget recipe required";return false;};
        const std::array<u32,32> words{0x47464236,1,128,0,0x80029434,0x80089898,0x80199c0c,0x80199c74,0x8019b8a4,0x80006fb4,0x80089100,0x80047aa4,0x80199d30,0x8019bb34,0x80005630,0x80370fb8,0x22e678b4,27,6,0,1,21,0x20,4,0x19,0x12,8,0x444,0x3f4ccccd,0xc3c80000,0x3f800000,0x38};
        if(!b.contains(0,128)||b.size!=128)return fail();
        for(unsigned i=0;i<words.size();++i)if(b.be32(4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
