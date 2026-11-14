#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
// Sparse, explicitly audited descriptor cells, NOT the extent of8031B694.
struct StartupSoundDescriptor {
    u16 id=0; s16 start=0,stop=0; u16 type=0,reserved=0; u32 interval=0; s32 next=0;
};
struct StartupSoundProfile {
    std::array<StartupSoundDescriptor,5> descriptors{}; f32 viewer_bias=0; bool loaded=false;
};
inline bool startup_sound_profile_valid(const StartupSoundProfile& p){
    constexpr std::array<u16,5> ids={0,1,2,4,10};
    constexpr std::array<s16,5> starts={40,126,200,204,204},stops={41,127,201,205,205};
    constexpr std::array<u16,5> types={1,1,0,0,0};
    if(!p.loaded||f32_to_bits(p.viewer_bias)!=0x3e4ccccd)return false;
    for(unsigned i=0;i<ids.size();++i){const auto& d=p.descriptors[i];
        if(d.id!=ids[i]||d.start!=starts[i]||d.stop!=stops[i]||d.type!=types[i]||d.reserved||d.interval!=10000||d.next)return false;
    }
    return true;
}
inline const StartupSoundDescriptor* startup_sound_descriptor(const StartupSoundProfile& p,u16 id){
    for(const auto& d:p.descriptors)if(d.id==id)return &d;
    return nullptr;
}
inline bool startup_sound_load(ByteView b,StartupSoundProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound profile: ")+why;return false;};
    if(!out||!b.contains(0,144)||b.size!=144||b.be32(0)!=0x4746534d||b.be16(4)!=1||b.be16(6)!=5||b.be32(8)!=144||b.be32(12)||b.be32(20))return fail("header/extent");
    StartupSoundProfile p;p.loaded=true;p.viewer_bias=b.bef32(16);
    for(unsigned i=0;i<5;++i){const auto at=24+i*24;auto& d=p.descriptors[i];
        if(b.be16(at+2)||b.be32(at+20))return fail("reserved record fields");
        d.id=b.be16(at);d.start=std::bit_cast<s16>(b.be16(at+4));d.stop=std::bit_cast<s16>(b.be16(at+6));
        d.type=b.be16(at+8);d.reserved=b.be16(at+10);d.interval=b.be32(at+12);d.next=std::bit_cast<s32>(b.be32(at+16));
    }
    if(!startup_sound_profile_valid(p))return fail("unreviewed sparse descriptor/operands");
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
