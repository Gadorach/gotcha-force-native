#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//First group39/type4 initializer THROUGH the pre-input part of19B094 only.
//The controller-word read, counter1653 change and completed dispatch are absent.
class BattleInitialBriefingWidgetProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial briefing widget: exact first type4/pre-input recipe required";return false;};
        if(!b.contains(0,128)||b.size!=128)return fail();
        const std::array<u32,32> words{
            0x47464249,1,128,0,0x80029434,0x80089898,0x80199c0c,0x80199c74,
            0x8019b030,0x8019b094,0x80006fb4,0x800086b8,0x80007d40,0x80007c30,
            0x8024ef10,0x8024ebc8,0x22e678b4,77059,27,4,3,18,16,17,
            57,58,0x12,180,179,0x3dcccccd,0x426c0000,0x803c7204};
        for(unsigned i=0;i<words.size();++i)if(b.be32(4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
