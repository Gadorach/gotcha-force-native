#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct BattleGroundWorldProfile {std::array<u32,12> words{};bool loaded=false;f32 value(unsigned i)const{return i<12?bits_to_f32(words[i]):0;}};
inline bool battle_ground_world_profile_valid(const BattleGroundWorldProfile& p){return p.loaded&&p.words==std::array<u32,12>{0,0x3f800000u,0x38c90fdbu,0x41a00000u,0x3f4ccccdu,0x40000000u,0x3e000000u,0x40400000u,0x3e800000u,0x3f7fbe77u,0x3f800000u,0xbf800000u};}
inline bool battle_ground_world_profile_load(ByteView b,BattleGroundWorldProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="battle ground world profile: source operands/extent";return false;};
    if(!out||!b.data||b.size!=64||b.be32(0)!=0x47464257u||b.be32(4)!=1||b.be32(8)!=64||b.be32(12))return fail();
    BattleGroundWorldProfile p;p.loaded=true;for(unsigned i=0;i<12;++i)p.words[i]=b.be32(16+4*i);if(!battle_ground_world_profile_valid(p))return fail();*out=p;if(error)error->clear();return true;
}
} // namespace gf
