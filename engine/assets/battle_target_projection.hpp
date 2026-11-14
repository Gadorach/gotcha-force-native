#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct BattleTargetProjectionProfile {std::array<u32,4> words{};bool loaded=false;f32 ray_lift()const{return bits_to_f32(words[0]);}};
inline bool battle_target_projection_profile_valid(const BattleTargetProjectionProfile& p){return p.loaded&&p.words==std::array<u32,4>{0x44fa0000u,0x3f000000u,16,20};}
inline bool battle_target_projection_profile_load(ByteView b,BattleTargetProjectionProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="battle target projection profile: source operands/extent";return false;};
    if(!out||!b.data||b.size!=32||b.be32(0)!=0x47465450u||b.be32(4)!=1||b.be32(8)!=32||b.be32(12))return fail();
    BattleTargetProjectionProfile p;p.loaded=true;for(unsigned i=0;i<4;++i)p.words[i]=b.be32(16+4*i);if(!battle_target_projection_profile_valid(p))return fail();*out=p;if(error)error->clear();return true;
}
} // namespace gf
