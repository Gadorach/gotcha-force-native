#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Private source keys are DATA, not callable guest/native addresses. This profile
//does not confer actor/solver/frame authority merely by having loaded bytes.
struct BattleGroundIkProfile {
    u32 cross_threshold_bits=0;std::array<u8,9> axis_pairs{};std::array<u8,3> angle_axes{};
    std::array<u32,6> basis_keys{};bool loaded=false;
};
inline bool battle_ground_ik_profile_valid(const BattleGroundIkProfile& p){
    return p.loaded&&p.cross_threshold_bits==0x2edbe6feu&&p.axis_pairs==std::array<u8,9>{0,0,1,2,0,3,4,5,0}&&p.angle_axes==std::array<u8,3>{120,121,122}&&p.basis_keys==std::array<u32,6>{0x80045fc8,0x80046090,0x80046158,0x80046220,0x800462e4,0x800463ac};
}
inline bool battle_ground_ik_profile_load(ByteView b,BattleGroundIkProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="battle ground IK profile: source operands/table/extent";return false;};
    if(!out||!b.data||b.size!=64||b.be32(0)!=0x4746494bu||b.be32(4)!=1||b.be32(8)!=64||b.be32(12)||b.be32(56)||b.be32(60))return fail();
    BattleGroundIkProfile p;p.loaded=true;p.cross_threshold_bits=b.be32(16);for(unsigned i=0;i<9;++i)p.axis_pairs[i]=b.data[20+i];for(unsigned i=0;i<3;++i)p.angle_axes[i]=b.data[29+i];for(unsigned i=0;i<6;++i)p.basis_keys[i]=b.be32(32+4*i);
    if(!battle_ground_ik_profile_valid(p))return fail();
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
