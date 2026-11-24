#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//05668/09C4C row4 and058D0/09CBC rows7/8. Both source video tables
//have identical operands for this horizontal split; other layouts stay separate.
class BattleSplitProjectionProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="battle split projection: exact source row4/viewport7-8 operands required";return false;};
        if(!b.data||b.size!=128||b.be32(0)!=0x47465650||b.be32(4)!=1||b.be32(8)!=128||b.be32(12))return fail();
        const std::array<u32,8> keys{0x8008cbd4,0x8008c440,0x80005668,0x80009c4c,0x800058d0,0x80009cbc,0x8020b8ec,0x8002aaac};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        const std::array<u32,4> projection{0x42240000,0x402aaaab,0x41200000,0x471c4000};
        for(unsigned i=0;i<4;++i)if(b.be32(48+4*i)!=projection[i])return fail();
        for(unsigned half=0;half<2;++half){
            const std::array<u32,6> viewport{0,half?0x43600000u:0u,0x44200000,0x43600000,0,0x3f800000};
            for(unsigned i=0;i<6;++i)if(b.be32(64+24*half+4*i)!=viewport[i])return fail();
        }
        const std::array<u32,4> branch{1,1,4,7};
        for(unsigned i=0;i<4;++i)if(b.be32(112+4*i)!=branch[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
