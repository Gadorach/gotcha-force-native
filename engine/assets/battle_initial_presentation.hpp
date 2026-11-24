#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Only the initial horizontal-split graphics fragment of286D8/8CEC8/8CBD4.
//Not an acknowledgement of the enclosing preparation transition or game tick.
class BattleInitialPresentationProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial battle presentation: exact source split graphics recipe required";return false;};
        if(!b.contains(0,112)||b.size!=112||b.be32(0)!=0x47465052||b.be32(4)!=1||b.be32(8)!=112||b.be32(12))return fail();
        const std::array<u32,12> keys{0x800286d8,0x8002aaac,0x8008cec8,0x8008cbd4,0x8002acd0,0x8008c440,0x80005668,0x80009c4c,0x800058d0,0x80009cbc,0x8002ab70,0x8008c3f0};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        const std::array<u32,12> words{640,448,224,6,0,1,0,4,7,0,0x100000,0x8c000};
        for(unsigned i=0;i<words.size();++i)if(b.be32(64+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
