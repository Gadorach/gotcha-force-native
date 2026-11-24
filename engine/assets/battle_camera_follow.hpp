#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//The first ordinary0C918/CB8C/CDC0 update after camera initialization.
class BattleCameraFollowProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="camera follow: exact first ordinary source recipe required";return false;};
        if(!b.contains(0,160)||b.size!=160||b.be32(0)!=0x47465646||b.be32(4)!=1||b.be32(8)!=160||b.be32(12))return fail();
        const std::array<u32,14> keys{0x8000bda4,0x8000c918,0x8000cb8c,0x8000cdc0,0x8000fc2c,0x800101c8,0x8001063c,0x80010664,0x8003dd34,0x8000fffc,0x8000c314,0x800452e4,0x800452a0,0x8004526c};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        const std::array<u32,16> operands{0,0x3f800000,0x3f000000,0x3a83126f,0x38c90fdb,0x4622f983,0x40800000,0x40a00000,0x41100000,0x41200000,0x3fa66666,0x3f4ccccd,0x3fc00000,0x3e99999a,0x40400000,0x3e800000};
        for(unsigned i=0;i<operands.size();++i)if(b.be32(72+4*i)!=operands[i])return fail();
        const std::array<u32,6> tail{1536,12288,0,1,2,0};
        for(unsigned i=0;i<tail.size();++i)if(b.be32(136+4*i)!=tail[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
