#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleInitialBriefingType2Profile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="briefing type2: exact first-party mode1/source18-row recipe required";return false;};
        const std::array<u32,32> header{0x47464232,1,352,0,0x80029434,0x80089898,0x80199c0c,0x80199c74,0x8019a3f0,0x8019bf18,0x80006fb4,0x80005630,0x800086b8,0x80007c30,0x80007cd0,0x80089100,0x22e678b4,27,2,18,57,0x80047aa4,0x20,4,0x80370ff8,0,14,15,22,23,0xc3480000,0xc28c0000};
        const std::array<u32,54> targets{
            0xc2b8cfdf,0xc120bd3c,0xc3968000,0xc2758106,0xc2ac22f8,0xc3968000,0xc2b8cfdf,0xc2ac22f8,0xc3968000,
            0x42758106,0x4239020c,0xc3968000,0x42b8cfdf,0xc1ed999a,0xc3968000,0x42758106,0xc1ed999a,0xc3968000,
            0xc2fefb64,0x4092432d,0xc395e000,0xc2c0ec4a,0xc28ee711,0xc395e000,0xc2fefb64,0xc28ee711,0xc395e000,
            0x42c0ec4a,0x4273020c,0xc395e000,0x42fefb64,0xc1745254,0xc395e000,0x42c0ec4a,0xc1745254,0xc395e000,
            0xc2ddda93,0x3ff542c4,0xc39a0000,0xc29fcb78,0xc2943639,0xc39a0000,0xc2ddda93,0xc2943639,0xc39a0000,
            0x429fcb78,0x426863bd,0xc39a0000,0x42ddda93,0xc18f65c9,0xc39a0000,0x429fcb78,0xc18f65c9,0xc39a0000};
        if(!b.contains(0,352)||b.size!=352)return fail();
        for(unsigned i=0;i<header.size();++i)if(b.be32(4*i)!=header[i])return fail();
        for(unsigned i=0;i<targets.size();++i){if(b.be32(128+4*i)!=targets[i])return fail();targets_[i/3][i%3]=bits_to_f32(targets[i]);}
        if(b.be32(344)||b.be32(348))return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    const std::array<f32,3>* target(unsigned variant,unsigned team,unsigned ordinal)const{return ready_&&variant<3&&team<2&&ordinal<3?&targets_[variant*6+team*3+ordinal]:nullptr;}
private:bool ready_=false;std::array<std::array<f32,3>,18> targets_{};
};
}
