#pragma once
#include "engine/assets/battle_ground_world.hpp"
#include "engine/gameplay/character_pose.hpp"
namespace gf {
struct BattleActorWorldInput {
    bool declared=false,retained1dbc_declared=false;u8 mode4a0=0,flags48a=0,field4a1=0;
    std::array<f32,3> position20{},scale58{1,1,1},retained1dbc{};std::array<s16,3> angles70{};s16 spin6b8=0;f32 offset6d0=0;
};
struct BattleActorFirstWorld {
    CharacterWorldPose world;Mat4 actor114,collider1aa4;std::array<f32,3> field1d54{};s16 effective_yaw72=0;
    bool evaluated=false,grounded_second_pass_pending=true,look_tail_pending=true;
};
//46754 and46BD0 FIRST(0) pass, not465FC's finished world frame. Explicit source
//actor scalar fields form the base; a caller cannot substitute a free matrix.
//48A retained-adjust and4A0 retained/paired branches remain unsupported. Same
//local/world kernels, no preview actor, renderer, skin fallback or actor ACK.
inline bool battle_actor_first_world(const BattleGroundWorldProfile& p,const CharacterPoseRig& rig,const CharacterLocalPose& local,const CharacterPoseFrame& modifiers,const BattleActorWorldInput& input,BattleActorFirstWorld* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("battle first world: ")+why;return false;};
    if(!out||!battle_ground_world_profile_valid(p)||!input.declared||!input.retained1dbc_declared||input.mode4a0||(input.flags48a&6u)||!character_pose_detail::rig(rig)||!character_pose_detail::local(rig,local))return fail("declared ordinary actor/local/rig/profile required");
    const Mat4 identity;if(modifiers.alternate_root||modifiers.inverse_kinematics||!std::equal(modifiers.actor_base.m,modifiers.actor_base.m+16,identity.m))return fail("no alternate/IK/free anchor in first pass");
    for(f32 x:input.position20)if(!character_pose_detail::finite(x))return fail("position domain");
    for(f32 x:input.scale58)if(!character_pose_detail::finite(x)||std::abs(x)>1e4f)return fail("scale domain");
    for(f32 x:input.retained1dbc)if(!character_pose_detail::finite(x))return fail("retained adjustment domain");
    if(!character_pose_detail::finite(input.offset6d0))return fail("offset domain");
    BattleActorFirstWorld next;auto angles=input.angles70;if(!input.field4a1&&input.spin6b8)angles[1]=std::bit_cast<s16>(u16(s32(angles[1])+s32(input.spin6b8)*1024));next.effective_yaw72=angles[1];
    auto base=mat4_multiply(mat4_translation(input.position20.data()),character_pose_detail::angles(angles));base=mat4_multiply(base,mat4_scale(input.scale58.data()));next.actor114=base;next.collider1aa4=base;next.collider1aa4.at(1,3)=f32(next.collider1aa4.at(1,3)+input.offset6d0);if(!character_pose_detail::matrix(next.collider1aa4))return fail("actor collider-frame domain");
    auto frame=modifiers;frame.actor_base=base;
    //46754 calls BA54(ADD):1D54=local bone1 translation+retained1DBC. The
    //shared world kernel already adds its adjust to local T, so pass1DBC here,
    //NOT the resulting1D54 (which would double the local bone1 translation).
    for(unsigned k=0;k<3;++k){next.field1d54[k]=f32(local.bones[1].translation[k]+input.retained1dbc[k]);frame.bone1_translation_adjust[k]=input.retained1dbc[k];if(!character_pose_detail::finite(next.field1d54[k]))return fail("bone1 adjustment sum domain");}
    if(!character_pose_direct_world(rig,local,frame,&next.world,error))return false;
    next.evaluated=true;*out=std::move(next);if(error)error->clear();return true;
}
} // namespace gf
