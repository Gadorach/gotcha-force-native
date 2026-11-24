#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//53B30 on the genuine six-cell factory state, before any541AC consumer.
//Subsequent frames and nonzero attachment/combined-mode dependencies stay closed.
class BattleInitialActorOrderProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial actor order: exact fresh source recipe required";return false;};
        if(!b.contains(0,64)||b.size!=64||b.be32(0)!=0x4746414f||b.be32(4)!=1||b.be32(8)!=64||b.be32(12))return fail();
        const std::array<u32,12> words{0x80053b30,0x80053a2c,0x80054070,0x800055fc,0x803c4e6c,0x80436260,0x803c6dec,0x80436264,6,0,0x47435000,0x3f800000};
        for(unsigned i=0;i<words.size();++i)if(b.be32(16+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
