#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Initial070B slot0/type3 point follower only. No other child/update/frame ACK.
class BattleInitialPointChildProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial point child: exact first070B source recipe required";return false;};
        if(!b.contains(0,104)||b.size!=104||b.be32(0)!=0x47464950||b.be32(4)!=1||b.be32(8)!=104||b.be32(12))return fail();
        const std::array<u32,12> keys{0x80089a94,0x8008997c,0x801fcda8,0x800660e8,0x801fcdfc,0x801fcefc,0x80046588,0x80089100,0x80007030,0x80007c54,0x80007cac,0x80047aa4};
        const std::array<u32,10> words{0x803a2f88,0x01010308,0x3f800000,0x3ecccccd,0x3e800000,0x070b,0x54,0x31,1,0};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        for(unsigned i=0;i<words.size();++i)if(b.be32(64+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
