#pragma once
#include "engine/assets/force_spec_actor_actions.hpp"
#include "engine/gameplay/force_spec_navigation.hpp"
#include <cmath>
namespace gf {
enum class ForceSpecActorAction {None,Demonstrate,Reset};
struct ForceSpecActorActionState {ForceSpecAngles angles;s8 cooldown6c=0;};
struct ForceSpecActorActionInput {
    u32 held=0,pressed=0;u8 hidden67=0,component17=0,entry6b=0;u16 model_id=0;u8 actor_index96=0;
    std::array<f32,3> base_position1c{};s16 actor_yaw72=0;
};
struct ForceSpecActorActionPlan {
    ForceSpecActorActionState state;ForceSpecActorAction action=ForceSpecActorAction::None;
    bool actor_callback_required=false,camera_callback_required=false,actor_yaw_write=false;
    s16 actor_yaw72=0;bool position_write=false,mode_bytes_write=false,root_yaw_write=false;
    std::array<f32,3> position20{};u8 field5da=0,field5db=0; s16 root_yaw5ae=0;
    u8 request_group=0,request_choice=0,request_mask=0,phase19=0,render_group82=0;
    s16 start=0;s8 duration=0,mode=0;
};
//Pure801DCFA0 caller policy, NOT a completed actor action. Publish the candidate
//UI state only with an actually qualified actor request/field consumer. The source
//caller never waits for a demo's VM done byte; no VM clock/finished flag is input.
inline bool force_spec_actor_action_plan(const ForceSpecActorActionProfile& profile,ForceSpecActorActionState state,
                                         const ForceSpecActorActionInput& input,ForceSpecActorActionPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("spec actor actions: ")+why;return false;};
    if(!out||!profile.valid())return fail("explicit profile/output required");
    ForceSpecActorActionPlan plan;plan.state=state;plan.state.angles=force_spec_rotate(state.angles,input.held);
    //Rotation accumulation precedes67/17/6B gates, not just active action input.
    if(input.hidden67){*out=plan;if(error)error->clear();return true;}
    plan.actor_callback_required=true;
    if(input.component17){*out=plan;if(error)error->clear();return true;}
    plan.camera_callback_required=true;
    if(input.entry6b==1){*out=plan;if(error)error->clear();return true;}
    if(plan.state.cooldown6c>0)--plan.state.cooldown6c;
    const bool attempt_demo=plan.state.cooldown6c==0&&(input.pressed&0x100u);
    if(attempt_demo){
        unsigned model=2;for(unsigned i=0;i<2;++i)if(profile.models[i]==input.model_id)model=i;
        if(model==2)return fail("demo capability for this model is unknown");
        if(profile.demonstration[model]){plan.action=ForceSpecActorAction::Demonstrate;plan.state.cooldown6c=s8(profile.cooldown);plan.field5da=plan.field5db=profile.cooldown;plan.root_yaw_write=true;plan.root_yaw5ae=input.actor_yaw72;}
        //An unavailable A attempt does NOT fall through to simultaneous X/Y.
    }else if(input.pressed&0xc00u){
        plan.action=ForceSpecActorAction::Reset;plan.state.angles={profile.pitch,profile.yaw};plan.state.cooldown6c=0;
        //The reset caller does not write5AE;57FF8 itself does not write it either.
    }
    if(plan.action!=ForceSpecActorAction::None){
        for(auto v:input.base_position1c)if(!std::isfinite(v)||std::abs(v)>1e7f)return fail("source base position required for request");
        plan.position_write=plan.mode_bytes_write=true;plan.position20=input.base_position1c;
        plan.request_group=profile.group;plan.request_choice=plan.action==ForceSpecActorAction::Demonstrate?profile.demo_choice:profile.reset_choice;
        plan.request_mask=profile.lane_mask;plan.start=profile.start;plan.duration=profile.duration;plan.mode=profile.mode;
        plan.phase19=0;plan.render_group82=u8(input.actor_index96+65u);
    }
    //This write follows the optional request, so demo saves PRIOR actor yaw.
    plan.actor_yaw_write=true;plan.actor_yaw72=std::bit_cast<s16>(u16(plan.state.angles.yaw));
    *out=plan;if(error)error->clear();return true;
}
} //namespace gf
