#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Fresh normal Challenge mode1/first-wave0 branch only; neither Story's phase2
//refusal branch nor later waves gain authority from this recipe.
class BattleInitialBriefingType1Profile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="briefing type1: exact Challenge mode1/wave0 digit initializer required";return false;};
        if(!b.contains(0,128)||b.size!=128)return fail();
        const std::array<u32,32> words{
            0x47464231,1,128,0,0x8002821c,0x80195f2c,0x80195fbc,0x801962c4,
            0x80199c0c,0x80199c74,0x8019a108,0x80006fb4,0x800086b8,0x80007c30,
            0x80089100,0x80047aa4,0x22e678b4,27,1,0,1,0x20,4,0x11,15,1,0,
            0xc2ffb22d,0x42b40ee6,0xc3390000,0xc3960000,0xc0f55555};
        for(unsigned i=0;i<words.size();++i)if(b.be32(4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
