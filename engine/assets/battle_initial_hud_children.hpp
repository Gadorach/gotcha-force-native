#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Sealed fresh split-cockpit initializer recipe; not HUD update/draw authority.
class BattleInitialHudChildrenProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial HUD: exact fresh cockpit/source initializer recipe required";return false;};
        if(!b.contains(0,128)||b.size!=128)return fail();
        const std::array<u32,32> words{
            0x47464849,1,128,0,0x800813a8,0x80081438,0x80081478,0x800818cc,
            0x800807e4,0x80080874,0x800808b4,0x80080f88,0x80089100,0x80006fb4,
            0x800086b8,0x80007d40,0x80007c30,0x8024ef10,0x8024ebc8,0x80007cd0,
            2708,0x2fbc80c9,118,56,81,50,0x21,5,1,6,0xff00,0xff0000};
        for(unsigned i=0;i<words.size();++i)if(b.be32(4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
