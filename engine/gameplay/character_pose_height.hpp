#pragma once
#include "engine/assets/character_pose_height.hpp"
#include <array>
#include <span>
#include <cmath>
namespace gf {
struct CharacterPoseHeightState {u8 mode71d=0;f32 multiplier6d8=0,offset6d0=0;};
struct CharacterPoseHeightFields {
    u8 model_group3e8=0,field661=0;u32 behaviour5e0=0; //high byte of u16 model ID
    f32 scale_b4=0,actor_y=0,height66c=0,radius670=0;
    s8 middle_bone=-1;std::array<s8,4> lower_bones={-1,-1,-1,-1};
    bool world_positions_declared=false;
};
// Source4D08C scalar kernel. bone_y is an explicitly supplied accepted-world
// observation, NOT an implicit world producer or actor-lifetime acknowledgement.
// Branch-local validation prevents fake pose requirements for the early returns.
inline bool character_pose_height_evaluate(const CharacterPoseHeightProfile& p,const CharacterPoseHeightFields& fields,
                                           const CharacterPoseHeightState& input,std::span<const f32> bone_y,
                                           CharacterPoseHeightState* out,std::string* error){
    auto next=input;if(out)*out={};if(error)error->clear();
    auto fail=[&](const char* why){if(error)*error=std::string("pose height: ")+why;return false;};
    auto number=[](f32 x){return std::isfinite(x)&&std::abs(x)<=1e7f;};
    if(!out||!character_pose_height_profile_valid(p))return fail("source profile/output");
    if(fields.model_group3e8==5||fields.field661||!(fields.behaviour5e0&0x40u))next.offset6d0=p.zero;
    else if(next.mode71d){
        if(!number(fields.scale_b4))return fail("scale for explicit height mode");
        const auto mode=std::bit_cast<s8>(next.mode71d);next.offset6d0=f32(f32(p.step*f32(s32(mode)-1))*fields.scale_b4);
    }else{
        if(!fields.world_positions_declared||bone_y.size()<2||bone_y.size()>32||fields.middle_bone<0||unsigned(fields.middle_bone)>=bone_y.size())return fail("declared world/middle bone required");
        if(!number(fields.actor_y)||!number(fields.height66c)||!number(fields.radius670)||!number(next.multiplier6d8))return fail("pose-height scalar domain");
        for(auto y:bone_y)if(!number(y))return fail("world position domain");
        f32 height=std::fma(-fields.height66c,p.half,bone_y[1]);
        const f32 middle=std::fma(f32(bone_y[unsigned(fields.middle_bone)]-fields.actor_y),p.half,fields.actor_y);
        if(middle<height)height=middle;
        for(auto slot:fields.lower_bones){if(slot==-1)break;if(slot<0||unsigned(slot)>=bone_y.size())return fail("lower bone index");const auto low=f32(bone_y[unsigned(slot)]-fields.radius670);if(low<height)height=low;}
        const auto delta=f32(height-fields.actor_y);const auto gain=f32(p.gain*next.multiplier6d8);next.offset6d0=f32(delta*gain);if(next.offset6d0<p.zero)next.offset6d0=p.zero;
    }
    if(!number(next.offset6d0))return fail("derived offset domain");
    *out=next;return true;
}
// Opcode8 handler4C5FC only acts on lane mask bit2. Negative operand clears71D
// and changes multiplier; nonnegative operand replaces71D but keeps multiplier.
inline bool character_pose_height_command(const CharacterPoseHeightProfile& p,const CharacterPoseHeightFields& fields,
                                          const CharacterPoseHeightState& input,std::span<const f32> bone_y,u8 lane_mask,u8 operand,
                                          CharacterPoseHeightState* out,std::string* error){
    auto next=input;if(out)*out={};if(error)error->clear();
    if(!out||!character_pose_height_profile_valid(p)||(lane_mask&~15u)){if(error)*error="pose height command: profile/mask/output";return false;}
    if(!(lane_mask&2)){*out=next;return true;}
    const auto signed_operand=std::bit_cast<s8>(operand);if(signed_operand<0){next.mode71d=0;next.multiplier6d8=f32(p.negative_factor*f32(signed_operand));}else next.mode71d=operand;
    return character_pose_height_evaluate(p,fields,next,bone_y,out,error);
}
} // namespace gf
