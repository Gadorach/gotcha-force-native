#pragma once
#include "engine/assets/battle_look_tail.hpp"
#include "engine/core/source_heading.hpp"
#include "engine/graphics/affine.hpp"
#include "engine/gameplay/battle_actor_grounded_world.hpp"
namespace gf {
struct BattleLookTailInput {
    bool gates_declared=false;u8 scene_maskd2=0,owner_mask3e5=0,field741=0,field660=0,control1d98=0,phase18=0;u32 behavior5e0=0;
    bool yaw72_declared=false,target_flag_declared=false,target_present=false,position20_declared=false,destination50c_declared=false,prior12_declared=false;
    s16 yaw72=0;std::array<f32,3> position20{},destination50c{};std::array<s16,2> prior12{};
};
struct BattleLookTailWrite {u16 offset=0;s16 value=0;bool operator==(const BattleLookTailWrite&)const=default;};
enum class BattleLookTailBranch:u8 {SceneSuppressed,ActorSkipped,Damped,AimWithoutTarget,Aimed};
struct BattleLookTailPlan {
    std::vector<BattleLookTailWrite> writes;std::array<s16,2> next12{},forward_angles{},destination_angles{},wrapped_error{},applied_delta{};
    s16 target_heading=0,heading_delta=0;std::array<f32,3> local_forward{},local_destination{};std::array<f32,2> ratios{};
    BattleLookTailBranch branch=BattleLookTailBranch::SceneSuppressed;bool evaluated=false,next12_known=false,reset=false,aim_called=false,frame_used=false;
};
namespace battle_look_tail_detail {
inline s16 wrap(s32 x){return std::bit_cast<s16>(u16(x));}
inline bool scaled(s16 value,f32 factor,s16* out){const f32 x=f32(f32(value)*factor);if(!std::isfinite(x)||x< -2147483648.f||f64(x)>2147483647.0)return false;*out=wrap(s32(x));return true;}
inline bool angle_pair(const std::array<f32,3>& v,std::array<s16,2>* out){
    if(!source_intersection_detail::valid(v))return false;
    const f32 horizontal=f32(std::sqrt(f64(f32(f32(v[0]*v[0])+f32(v[2]*v[2])))));s16 pitch=0,yaw=0;if(!source_heading_s16(v[1],horizontal,&pitch)||!source_heading_s16(v[0],v[2],&yaw))return false;*out={wrap(-s32(pitch)),yaw};return true;
}
}
//465FC→47904, with the reached6C728 caller fixed to row0/bone12/reference3/
//mode2. Other6C728 modes are NOT silently admitted. Writes target extra angles
//191C/191E AFTER world construction: they affect NEXT world evaluation, not this
//one. The journal records reset-before-damp/aim ordering, not source atomicity.
inline bool battle_actor_look_tail_plan(const BattleLookTailProfile& p,const StartupActorSetupProfile& setup,const CharacterPoseRig& rig,const CharacterWorldPose& world,const BattleLookTailInput& input,BattleLookTailPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("battle look tail: ")+why;return false;};
    if(!out||!battle_look_tail_profile_valid(p)||!input.gates_declared)return fail("declared gates/profile/destination required");
    BattleLookTailPlan next;next.evaluated=true;
    auto done=[&](){*out=std::move(next);if(error)error->clear();return true;};
    if(input.scene_maskd2&input.owner_mask3e5)return done();
    if(input.field741||input.field660||(input.control1d98&0xf0u)==0x10u){next.branch=BattleLookTailBranch::ActorSkipped;return done();}
    std::array<s16,2> prior{};next.reset=input.phase18!=1||(input.behavior5e0&0x01000000u);
    if(next.reset){next.writes.push_back({0x191e,0});next.writes.push_back({0x191c,0});}
    else{if(!input.prior12_declared)return fail("reached retained bone12 angles unknown");prior=input.prior12;}
    if(!input.yaw72_declared||!input.target_flag_declared)return fail("reached yaw/target flag unknown");
    next.target_heading=input.yaw72;
    if(input.target_present){
        if(!input.position20_declared||!input.destination50c_declared)return fail("reached source20/destination50C unknown");
        const f32 dx=f32(input.destination50c[0]-input.position20[0]),dz=f32(input.destination50c[2]-input.position20[2]);if(!source_heading_s16(dx,dz,&next.target_heading))return fail("heading numeric domain");
    }
    next.heading_delta=battle_look_tail_detail::wrap(s32(next.target_heading)-s32(input.yaw72));const s32 limit=s32(p.words[2]);next.aim_called=next.heading_delta> -limit&&next.heading_delta<limit&&!(input.behavior5e0&3u);
    auto store=[&](std::array<s16,2> angles){next.next12=angles;next.next12_known=true;next.writes.push_back({0x191c,angles[0]});next.writes.push_back({0x191e,angles[1]});};
    if(!next.aim_called||!input.target_present){
        next.branch=next.aim_called?BattleLookTailBranch::AimWithoutTarget:BattleLookTailBranch::Damped;std::array<s16,2> angles;if(!battle_look_tail_detail::scaled(prior[0],p.decay(),&angles[0])||!battle_look_tail_detail::scaled(prior[1],p.decay(),&angles[1]))return fail("retained-angle damping domain");store(angles);return done();
    }
    next.branch=BattleLookTailBranch::Aimed;
    if(!startup_actor_setup_detail::valid(setup)||!character_pose_detail::rig(rig)||rig.binding.bones.size()<=12||world.matrices.size()!=rig.binding.bones.size()||world.joint_offsets.size()!=rig.binding.bones.size())return fail("reached class/rig/world extent");
    for(unsigned i:{3u,12u})if(world.joint_offsets[i]!=rig.binding.bones[i].joint_offset||!character_pose_detail::matrix(world.matrices[i]))return fail("reached logical frame identity/domain");
    if(!source_intersection_detail::valid(input.destination50c))return fail("reached target point domain");
    //66454/451B8 use full inverse DIRECTION multiplication.4516C uses full
    //inverse POINT multiplication. The common linear part is bone3's basis;
    //the latter frame's translation is bone12's origin. No column normalization
    //or transpose substitution (including sheared/nonuniform bases).
    Mat4 frame=world.matrices[3];for(unsigned k=0;k<3;++k)frame.at(k,3)=world.matrices[12].at(k,3);Mat4 inverse;if(!mat4_inverse_affine(frame,&inverse))return fail("source full inverse domain; no identity fallback");
    const std::array<f32,3> forward{world.matrices[12].at(0,2),world.matrices[12].at(1,2),world.matrices[12].at(2,2)};mat4_transform_direction(inverse,forward.data(),next.local_forward.data());mat4_transform_point(inverse,input.destination50c.data(),next.local_destination.data());
    if(!battle_look_tail_detail::angle_pair(next.local_forward,&next.forward_angles)||!battle_look_tail_detail::angle_pair(next.local_destination,&next.destination_angles))return fail("local heading domain");
    const ByteView data{setup.actor_data.data(),setup.actor_data.size()};auto signed_at=[&](unsigned at){return std::bit_cast<s16>(data.be16(at));};
    //Negative/zero selects the first pitch limit but the second yaw limit.
    const s16 pitch=next.destination_angles[0],yaw=next.destination_angles[1];const std::array<s16,2> limits{signed_at(pitch>0?0x162:0x160),signed_at(yaw>0?0x15c:0x15e)},weights{signed_at(pitch>0?0x16a:0x168),signed_at(yaw>0?0x164:0x166)};
    for(unsigned k=0;k<2;++k){if(limits[k]==0)return fail("selected source aim limit zero; unsupported division");next.ratios[k]=std::min(std::abs(f32(f32(next.destination_angles[k])/f32(limits[k]))),1.f);s16 desired=0;if(!battle_look_tail_detail::scaled(weights[k],next.ratios[k],&desired))return fail("weighted target angle domain");next.wrapped_error[k]=battle_look_tail_detail::wrap(s32(desired)-s32(next.forward_angles[k]));if(!battle_look_tail_detail::scaled(next.wrapped_error[k],p.aim_gain(),&next.applied_delta[k]))return fail("source mode2 correction domain");}
    store({battle_look_tail_detail::wrap(s32(prior[0])+s32(next.applied_delta[0])),battle_look_tail_detail::wrap(s32(prior[1])+s32(next.applied_delta[1]))});next.frame_used=true;return done();
}
struct BattleActorWorldAfterLook {BattleActorGroundedWorld geometry;BattleLookTailPlan look;CharacterPoseFrame next_modifiers;bool evaluated=false;};
//Ordinary4A0=0 has no47A2C paired-copy branch. This explicit entry evaluates the
//world first, then retained look writes; no current world matrix is retrofitted.
//The current actor/resource/lifecycle frame owner remains UNFULFILLED.
inline bool battle_actor_world_after_look(const BattleGroundWorldProfile& p,const BattleGroundIkProfile& ik,const BattleLookTailProfile& look_profile,const StartupActorSetupProfile& setup,const CharacterPoseRig& rig,const CharacterLocalPose& local,const CharacterPoseFrame& modifiers,const BattleActorWorldInput& actor,const BattleGroundContactInput& ground,BattleLookTailInput look,const ChallengeStageInputs& stage,BattleActorWorldAfterLook* out,std::string* error){
    if(out)*out={};
    if(!out){if(error)*error="battle world after look: destination";return false;}
    BattleActorWorldAfterLook next;if(!battle_actor_grounded_world(p,ik,setup,rig,local,modifiers,actor,ground,stage,&next.geometry,error))return false;
    look.yaw72_declared=true;look.yaw72=actor.angles70[1];look.position20_declared=true;look.position20=actor.position20;look.control1d98=ground.control1d98;look.behavior5e0=ground.flags5e0;
    if(modifiers.extra_angles.size()>12){look.prior12_declared=true;look.prior12={modifiers.extra_angles[12][0],modifiers.extra_angles[12][1]};}
    if(!battle_actor_look_tail_plan(look_profile,setup,rig,next.geometry.world,look,&next.look,error))return false;
    next.next_modifiers=modifiers;if(next.look.next12_known){if(next.next_modifiers.extra_angles.size()<=12){if(error)*error="battle world after look: bone12 write extent";return false;}next.next_modifiers.extra_angles[12][0]=next.look.next12[0];next.next_modifiers.extra_angles[12][1]=next.look.next12[1];}
    next.geometry.look_tail_pending=false;next.evaluated=true;*out=std::move(next);if(error)error->clear();return true;
}
} // namespace gf
