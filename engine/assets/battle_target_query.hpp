#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct BattleTargetQueryProfile {std::array<u32,4> words{};bool loaded=false;f32 maximum_squared_distance()const{return bits_to_f32(words[1]);}};
inline bool battle_target_query_profile_valid(const BattleTargetQueryProfile& p){return p.loaded&&p.words==std::array<u32,4>{0,0x4dbebc20u,6,0x1e00};}
inline bool battle_target_query_profile_load(ByteView b,BattleTargetQueryProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="battle target query profile: exact source operands/extent";return false;};
    if(!out||!b.data||b.size!=32||b.be32(0)!=0x47465154u||b.be32(4)!=1||b.be32(8)!=32||b.be32(12))return fail();
    BattleTargetQueryProfile p;p.loaded=true;for(unsigned i=0;i<4;++i)p.words[i]=b.be32(16+4*i);if(!battle_target_query_profile_valid(p))return fail();*out=p;if(error)error->clear();return true;
}
} // namespace gf
