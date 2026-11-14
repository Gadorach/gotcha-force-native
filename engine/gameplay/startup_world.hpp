#pragma once
// Source80046D00 alternate-root chain, not supplied-anchor direct world.
#include "engine/gameplay/character_pose.hpp"
namespace gf {
struct StartupActorTransform {
    f32 position[3]={}; //actor20/24/28, supplied source state, not fitted placement
    std::array<s16,3> angles{}; //actor70/72/74
    f32 scale[3]={1,1,1}; //actor58/5C/60
    u8 model_flags=0; //actor48A: retained bone1-adjust branch remains unsupported
};
inline bool character_pose_startup_world(const CharacterPoseRig& rig,const CharacterLocalPose& local,
                                        const StartupActorTransform& actor,const CharacterPoseFrame& modifiers,
                                        CharacterWorldPose* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup world: ")+why;return false;};
    if(!out||!character_pose_detail::rig(rig)||!character_pose_detail::local(rig,local))return fail("rig/pose/destination");
    const auto n=local.bones.size();const u32 all=n==32?0xffffffffu:(u32(1)<<n)-1;const Mat4 identity;
    if(!modifiers.alternate_root||modifiers.inverse_kinematics||!std::equal(modifiers.actor_base.m,modifiers.actor_base.m+16,identity.m)||(actor.model_flags&6)||
       (modifiers.enabled_bones&all)!=all||modifiers.extra_angles.size()!=n||modifiers.scales.size()!=n)return fail("branch/opaque anchor/IK/retained-bone domain");
    for(auto v:actor.position)if(!character_pose_detail::finite(v))return fail("actor position");
    for(auto v:actor.scale)if(!character_pose_detail::finite(v)||v==0||std::abs(v)>1e4f)return fail("actor scale");
    for(auto v:modifiers.bone1_translation_adjust)if(!character_pose_detail::finite(v))return fail("bone1 adjustment");
    for(const auto& s:modifiers.scales)for(auto v:s)if(!character_pose_detail::finite(v)||v==0||std::abs(v)>1e4f)return fail("bone scale");
    auto base=mat4_multiply(mat4_translation(actor.position),character_pose_detail::angles(actor.angles));base=mat4_multiply(base,mat4_scale(actor.scale));
    CharacterWorldPose next;next.matrices.resize(n);next.joint_offsets.resize(n);
    for(u8 slot:rig.binding.preorder_slots){const auto& p=local.bones[slot];auto parent=slot==0?base:next.matrices[rig.binding.bones[slot].parent];
        if(slot==1)parent=mat4_multiply(parent,character_pose_detail::angles(modifiers.bone1_look_angles,true));
        Mat4 q;f32 affine[12];if(!gf_transform_quaternion(p.quaternion,affine))return fail("quaternion matrix");for(unsigned i=0;i<12;++i)q.m[i]=affine[i];
        for(unsigned k=0;k<3;++k)q.at(k,3)=p.translation[k]+(slot==1?modifiers.bone1_translation_adjust[k]:0);
        auto matrix=mat4_multiply(parent,q);matrix=mat4_multiply(matrix,character_pose_detail::angles(modifiers.extra_angles[slot]));matrix=mat4_multiply(matrix,mat4_scale(modifiers.scales[slot].data()));
        if(!character_pose_detail::matrix(matrix))return fail("computed matrix domain");
        next.matrices[slot]=matrix;next.joint_offsets[slot]=p.joint_offset;
    }
    *out=std::move(next);return true;
}
} // namespace gf
