#pragma once
// Offline-exported difficulty values/counters, not a DOL reader or battle setup.
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct ChallengeDifficultyProfile {
    std::array<s32,3> energy{};
    u16 open_updates=0,close_updates=0;
    bool valid()const{
        return energy[0]>0&&energy[0]<energy[1]&&energy[1]<energy[2]&&
            open_updates&&open_updates<=1024&&close_updates&&close_updates<=1024;
    }
};
inline bool challenge_difficulty_profile_load(ByteView bytes,ChallengeDifficultyProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("Challenge difficulty: ")+why;return false;};
    if(!out||!bytes.data||bytes.size!=40||bytes.be32(0)!=0x47464344||bytes.be32(4)!=1)return fail("profile header/extent");
    ChallengeDifficultyProfile candidate;
    for(unsigned i=0;i<3;++i){const u32 value=bytes.be32(8+i*4);if(value>0x7fffffffu)return fail("signed energy range");candidate.energy[i]=s32(value);}
    candidate.open_updates=bytes.be16(20);candidate.close_updates=bytes.be16(22);
    // This controller implements the normalized source input/mask domain only.
    if(bytes.be16(24)!=1||bytes.be16(26)!=2||bytes.be16(28)!=0x180||bytes.be16(30)!=0x200||bytes.data[32]!=8)return fail("unsupported input/display contract");
    for(unsigned i=33;i<40;++i)if(bytes.data[i])return fail("reserved bytes");
    if(!candidate.valid())return fail("energy order/counter bounds");
    *out=candidate;if(error)error->clear();return true;
}
} // namespace gf
