#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct StartupAuxiliaryTurnProfile {
    f32 movement_threshold=0,pi=0,binary_divisor=0,history_step=0,orientation_factor=0,cross_epsilon_sq=0,cos_min=0,cos_max=0;
    std::array<f32,3> fallback{};f32 zero=0;bool loaded=false;
};
inline bool startup_auxiliary_turn_valid(const StartupAuxiliaryTurnProfile& p){
    const std::array<u32,12> bits={f32_to_bits(p.movement_threshold),f32_to_bits(p.pi),f32_to_bits(p.binary_divisor),f32_to_bits(p.history_step),f32_to_bits(p.orientation_factor),f32_to_bits(p.cross_epsilon_sq),f32_to_bits(p.cos_min),f32_to_bits(p.cos_max),f32_to_bits(p.fallback[0]),f32_to_bits(p.fallback[1]),f32_to_bits(p.fallback[2]),f32_to_bits(p.zero)};
    return p.loaded&&bits==std::array<u32,12>{0x3c23d70a,0x40490fdb,0x47000000,0x3d0efa35,0x3c8efa35,0x2edbe6fe,0xbf800000,0x3f800000,0,0x3f800000,0,0};
}
inline bool startup_auxiliary_turn_load(ByteView b,StartupAuxiliaryTurnProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup auxiliary turn profile: ")+why;return false;};
    if(!out||!b.contains(0,64)||b.size!=64||b.be32(0)!=0x47465455||b.be16(4)!=1||b.be16(6)!=12||b.be32(8)!=64||b.be32(12))return fail("header/extent");
    StartupAuxiliaryTurnProfile p;p.loaded=true;p.movement_threshold=b.bef32(16);p.pi=b.bef32(20);p.binary_divisor=b.bef32(24);p.history_step=b.bef32(28);p.orientation_factor=b.bef32(32);p.cross_epsilon_sq=b.bef32(36);p.cos_min=b.bef32(40);p.cos_max=b.bef32(44);for(unsigned k=0;k<3;++k)p.fallback[k]=b.bef32(48+k*4);p.zero=b.bef32(60);
    if(!startup_auxiliary_turn_valid(p))return fail("unreviewed source operands");
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
