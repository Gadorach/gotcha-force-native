#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct StartupSoundSpatialProfile {
    f32 bias=0,invalid_distance=0,zero=0,attenuation_max=0,distance_unit=0,screen_max=0,pan_unit=0,degenerate_length=0;
    f32 perspective=0,one=0,half=0;bool loaded=false;
};
inline bool startup_sound_spatial_profile_valid(const StartupSoundSpatialProfile& p){
    const std::array<u32,11> bits={f32_to_bits(p.bias),f32_to_bits(p.invalid_distance),f32_to_bits(p.zero),f32_to_bits(p.attenuation_max),f32_to_bits(p.distance_unit),f32_to_bits(p.screen_max),f32_to_bits(p.pan_unit),f32_to_bits(p.degenerate_length),f32_to_bits(p.perspective),f32_to_bits(p.one),f32_to_bits(p.half)};
    return p.loaded&&bits==std::array<u32,11>{0x3e4ccccd,0xbf800000,0,0x42be0000,0x42200000,0x44200000,0x40a14285,0x3c23d70a,0,0x3f800000,0x3f000000};
}
inline bool startup_sound_spatial_load(ByteView b,StartupSoundSpatialProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound spatial profile: ")+why;return false;};
    if(!out||!b.contains(0,64)||b.size!=64||b.be32(0)!=0x47465350||b.be16(4)!=1||b.be16(6)!=11||b.be32(8)!=64||b.be32(12)||b.be32(60))return fail("header/extent/reserved");
    StartupSoundSpatialProfile p;p.loaded=true;p.bias=b.bef32(16);p.invalid_distance=b.bef32(20);p.zero=b.bef32(24);p.attenuation_max=b.bef32(28);p.distance_unit=b.bef32(32);p.screen_max=b.bef32(36);p.pan_unit=b.bef32(40);p.degenerate_length=b.bef32(44);p.perspective=b.bef32(48);p.one=b.bef32(52);p.half=b.bef32(56);
    if(!startup_sound_spatial_profile_valid(p))return fail("unreviewed source operands");
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
