#pragma once
#include "engine/core/types.hpp"
#include <string>
namespace gf {
// GFHC: source4C5FC/4D08C numeric operands; no actor/resource admission.
struct CharacterPoseHeightProfile {f32 zero=0,half=0,negative_factor=0,gain=0,step=0;bool loaded=false;};
inline bool character_pose_height_profile_valid(const CharacterPoseHeightProfile& p){return p.loaded&&f32_to_bits(p.zero)==0&&p.half==.5f&&f32_to_bits(p.negative_factor)==0xbd4ccccdu&&f32_to_bits(p.gain)==0x3f333333u&&p.step==5;}
inline bool character_pose_height_profile_load(ByteView b,CharacterPoseHeightProfile* out,std::string* error){
    if(out)*out={};
    if(!out||!b.data||b.size!=28||b.be32(0)!=0x47464843u||b.be32(4)!=1){if(error)*error="pose height: GFHC extent/version";return false;}
    CharacterPoseHeightProfile p;p.zero=b.bef32(8);p.half=b.bef32(12);p.negative_factor=b.bef32(16);p.gain=b.bef32(20);p.step=b.bef32(24);p.loaded=true;
    if(!character_pose_height_profile_valid(p)){if(error)*error="pose height: source operands";return false;}
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
