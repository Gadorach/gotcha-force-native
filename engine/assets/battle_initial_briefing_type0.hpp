#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Fresh group39/type0 initializer only. Phase1/19EA8 and its later sound/counter
//work are deliberately not admitted by this recipe.
class BattleInitialBriefingType0Profile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="briefing type0: exact fresh initializer/default-alpha recipe required";return false;};
        if(!b.contains(0,128)||b.size!=128)return fail();
        const std::array<u32,32> words{
            0x47464230,1,128,0,0x80029434,0x80089898,0x80199c0c,0x80199c74,
            0x80199dc0,0x80199ea8,0x80006fb4,0x80007cac,0x800091e4,0x802437c8,
            0x80250298,0x80199d30,0x22e678b4,27,0,0,2,2,3,0x11,10,0,
            0xc2960000,0x3f800000,0xc3968000,0x3f000000,0xc3960000,0x3e4ccccd};
        for(unsigned i=0;i<words.size();++i)if(b.be32(4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
