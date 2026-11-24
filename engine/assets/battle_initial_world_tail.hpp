#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Only the initial inactive auxiliary branch and end of the first89554 traversal.
//Does not authorize active auxiliary transforms, UI89658 or a render frame.
class BattleInitialWorldTailProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial world tail: exact inactive auxiliary/first89554 end recipe required";return false;};
        const std::array<u32,24> words{0x47465754,1,96,0,0x800c79f8,0x800c7a0c,0x800c7a1c,0x800c7a24,0x800c7c6c,0x800537a8,0x8005685c,0x80089554,0x80089658,0x8008972c,0xa8,6,3,0x615,41,2,40,0,0x144,2};
        if(!b.contains(0,96)||b.size!=96)return fail();
        for(unsigned i=0;i<words.size();++i)if(b.be32(4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
