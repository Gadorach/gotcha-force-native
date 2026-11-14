#pragma once
#include "engine/gameplay/battle_actor_first_world.hpp"
#include "engine/gameplay/startup_actor_setup.hpp"
#include "engine/gameplay/challenge_stage_inputs.hpp"
namespace gf {
struct BattleGroundContact {std::array<f32,3> point{};std::array<u8,4> source_bone_axes{};};
struct BattleGroundConstraint {u8 bone=0,axis4=0,axis5=0;}; //source padding is not fabricated
struct BattleGroundContactInput {
    bool declared=false;u32 flags5e0=0;u8 control1d98=0,control1d99=0;f32 scalar_b4=0;
    f32 prior_velocity1d60=0;StageGroundActor ground;
};
struct BattleGroundContactPlan {
    std::vector<BattleGroundContact> contacts;std::vector<BattleGroundConstraint> constraints;
    std::array<f32,3> adjust1d54{};f32 velocity1d60=0,chosen_lift=0;u8 examined=0,ground_calls=0;
    bool damped_only=false,second_world_pass_pending=true,ik_solver_pending=false;
};
//46E7C→46F08 mode0 /46EF4/47344 damped field work. This checks declared first
//world identities, not live same-owner authority or a finished collider frame.
//Two-bone47368, constraint4784C, second46914 and look47904 still must run.
inline bool battle_ground_contact_plan(const BattleGroundWorldProfile& p,const StartupActorSetupProfile& setup,const CharacterPoseRig& rig,const BattleActorFirstWorld& first,const BattleGroundContactInput& input,const ChallengeStageInputs& stage,BattleGroundContactPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("battle ground contact: ")+why;return false;};
    if(!out||!battle_ground_world_profile_valid(p)||!startup_actor_setup_detail::valid(setup)||!character_pose_detail::rig(rig)||!first.evaluated||!input.declared||setup.class_config[1]!=0)return fail("declared grounded-class/first-world/profile required");
    const auto n=rig.binding.bones.size();if(first.world.matrices.size()!=n||first.world.joint_offsets.size()!=n)return fail("first-world rig extent");for(unsigned i=0;i<n;++i)if(first.world.joint_offsets[i]!=rig.binding.bones[i].joint_offset||!character_pose_detail::matrix(first.world.matrices[i]))return fail("first-world rig identity/domain");
    for(f32 x:first.field1d54)if(!character_pose_detail::finite(x))return fail("adjustment domain");
    if(!character_pose_detail::finite(input.prior_velocity1d60))return fail("velocity domain");
    BattleGroundContactPlan next;next.adjust1d54=first.field1d54;
    const unsigned mode=input.control1d98&15u;
    if((input.flags5e0&0x20000000u)||mode==1||mode==3){next.damped_only=true;next.velocity1d60=f32(input.prior_velocity1d60*p.value(4));if(!(input.flags5e0&0x20000000u)&&mode==3)next.adjust1d54[1]=f32(next.adjust1d54[1]+next.velocity1d60);if(!character_pose_detail::finite(next.adjust1d54[1]))return fail("damped adjustment domain");*out=std::move(next);if(error)error->clear();return true;}
    if(mode!=0)return fail("other ground-control handlers require separate consumer");
    if(!std::isfinite(input.scalar_b4)||input.scalar_b4<=0||input.scalar_b4>1e4f)return fail("positive sourceB4 required");
    StartupActorInitialSetup dimensions;if(!startup_actor_dimensions(setup,input.scalar_b4,&dimensions))return fail("class dimension products");const f32 lift_height=dimensions.field670,down_limit=dimensions.field674;const f32 root_y=first.world.matrices[0].at(1,3);const f32 threshold=f32(f32(p.value(5)*f32(input.control1d99&15u))*input.scalar_b4);const u32 signed_mask=std::bit_cast<u32>(s32(std::bit_cast<s8>(input.control1d99)));
    f32 best=p.value(0);
    for(unsigned i=0;i<4;++i){const unsigned at=0x22+8*i;const s8 bone=std::bit_cast<s8>(setup.class_config[at]);if(bone==-1)break;++next.examined;if(signed_mask&(1u<<(i+6)))continue;if(bone<0||unsigned(bone)>=n)return fail("consumed foot bone outside first world");
        const auto& m=first.world.matrices[unsigned(bone)];std::array<f32,3> point{m.at(0,3),m.at(1,3),m.at(2,3)};const f32 above=f32(f32(point[1]-lift_height)-root_y);if(above>threshold)continue;
        if(!input.ground.fields_declared||input.ground.current24!=first.actor114.at(1,3)||f32(input.ground.current24+input.ground.offset6d0)!=first.collider1aa4.at(1,3))return fail("ground owner/actor frame fields differ");
        f32 ground=0;if(!stage.ground(input.ground,point,lift_height,&ground,error))return false;++next.ground_calls;const f32 target_y=f32(lift_height+ground),change=f32(target_y-point[1]);if(!character_pose_detail::finite(target_y)||!character_pose_detail::finite(change))return fail("ground target numeric domain");if(change< -down_limit)continue;
        //4702C compares prior lift to this lift;47030 skips the write unless
        //prior>current. This is the MINIMUM accepted lift, not a maximum.
        if(next.contacts.empty()||change<best)best=change;
        point[1]=target_y;BattleGroundContact contact;contact.point=point;std::copy_n(setup.class_config.data()+at,4,contact.source_bone_axes.begin());next.contacts.push_back(contact);
        if(!(signed_mask&(1u<<(i+4))))next.constraints.push_back({setup.class_config[at],setup.class_config[at+4],setup.class_config[at+5]});
    }
    if(best< -down_limit)best=f32(-f32(down_limit*p.value(6)));
    next.chosen_lift=best;const f32 normalized=f32(best/input.scalar_b4);const f32 fused=std::fma(p.value(7),input.prior_velocity1d60,normalized);next.velocity1d60=f32(fused*p.value(8));next.adjust1d54[1]=f32(next.adjust1d54[1]+next.velocity1d60);if(!character_pose_detail::finite(next.velocity1d60)||!character_pose_detail::finite(next.adjust1d54[1]))return fail("spring field domain");next.ik_solver_pending=!next.contacts.empty()||!next.constraints.empty();*out=std::move(next);if(error)error->clear();return true;
}
} // namespace gf
