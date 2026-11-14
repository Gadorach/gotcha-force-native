#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct BattleActorBirthProfile {std::array<u32,7> words{};bool loaded=false;};
inline bool battle_actor_birth_profile_valid(const BattleActorBirthProfile& p){return p.loaded&&p.words==std::array<u32,7>{0x1e00,0x144,0x80,0x540,0x394,0,0x80068344};}
inline bool battle_actor_birth_profile_load(ByteView b,BattleActorBirthProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="battle actor birth profile: source operands/extent";return false;};
    if(!out||!b.data||b.size!=48||b.be32(0)!=0x47464248u||b.be32(4)!=1||b.be32(8)!=48||b.be32(12)||b.be32(44))return fail();
    BattleActorBirthProfile p;p.loaded=true;for(unsigned i=0;i<7;++i)p.words[i]=b.be32(16+4*i);if(!battle_actor_birth_profile_valid(p))return fail();*out=p;if(error)error->clear();return true;
}
} // namespace gf
