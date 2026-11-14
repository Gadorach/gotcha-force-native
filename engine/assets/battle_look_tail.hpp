#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct BattleLookTailProfile {std::array<u32,3> words{};bool loaded=false;f32 decay()const{return bits_to_f32(words[0]);}f32 aim_gain()const{return bits_to_f32(words[1]);}};
inline bool battle_look_tail_profile_valid(const BattleLookTailProfile& p){return p.loaded&&p.words==std::array<u32,3>{0x3f75c28fu,0x3dcccccdu,0x5000u};}
inline bool battle_look_tail_profile_load(ByteView b,BattleLookTailProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="battle look tail profile: source operands/extent";return false;};
    if(!out||!b.data||b.size!=32||b.be32(0)!=0x47464c54u||b.be32(4)!=1||b.be32(8)!=32||b.be32(12)||b.be32(28))return fail();
    BattleLookTailProfile p;p.loaded=true;for(unsigned i=0;i<3;++i)p.words[i]=b.be32(16+4*i);if(!battle_look_tail_profile_valid(p))return fail();*out=p;if(error)error->clear();return true;
}
} // namespace gf
