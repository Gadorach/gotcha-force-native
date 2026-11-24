#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleContactTailProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="battle initial contact tail: source initial non-response recipe required";return false;};
        if(!b.contains(0,112)||b.size!=112||b.be32(0)!=0x47464354||b.be32(4)!=1||b.be32(8)!=112||b.be32(12))return fail();
        const std::array<u32,24> words{0x8002bb14,0x8002d35c,0x8002d58c,0x8002d198,0x8002e1c8,0x8002cb20,0x8002ce44,0x8002d7c4,0x8002df08,0x8002db58,0x8005a580,0x80036e24,0x8008972c,0x80089658,0x8000bda4,0x8002ce90,0x80029b58,0x41a00000,0x800,0x17,13,12,6,0};
        for(unsigned i=0;i<words.size();++i)if(b.be32(16+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
