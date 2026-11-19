#pragma once
#include "engine/gameplay/borgs/normal_gred_movement_programs.hpp"
#include "engine/gameplay/character_air_resource.hpp"
#include "engine/gameplay/character_movement_motion.hpp"
#include "engine/gameplay/gred_surface_contact.hpp"
namespace gf {
//6235C/62284 ordinary normal-G-Red subset. Lower and upper stage latches
//are intentionally separate; this is numeric planning, not an actor callback
//or original collision/effect receipt.
struct GRedRelaunchState {u8 lower_stage=0,upper_stage=0;};
struct GRedRelaunchPlan {
    GRedRelaunchState state;CharacterMovementMotionState motion;
    bool request=false;u8 program=NormalGRedMovementPrograms::relaunch_windup,cost_index=u8(AirMeterCharge::LiftEntry);bool entry=false;
};
inline bool gred_relaunch_plan(const CharacterMovementProfile& p,const GRedRelaunchState& input,
                              const CharacterMovementMotionState& motion,GRedRelaunchPlan* out){
    if(!out||!character_movement_profile_valid(p)||!p.air_operands||p.model!=0x615||p.class_flags!=12||p.movement_type||
       !character_movement_motion_valid(motion)||(motion.flags5e0&0x2000u)||motion.field741||motion.field6cb||motion.vertical_only598||
       input.lower_stage>3||input.upper_stage>3||input.upper_stage>input.lower_stage)return false;
    GRedRelaunchPlan plan;plan.state=input;plan.motion=motion;plan.entry=input.lower_stage==0;
    plan.cost_index=u8(plan.entry?AirMeterCharge::LiftEntry:AirMeterCharge::LiftUpdate);
    if(plan.entry){
        plan.state.lower_stage=1;plan.request=true;
        plan.motion.vertical48=f32(motion.vertical48+p.air[2]);
    }else if(input.lower_stage==2){
         plan.state.lower_stage=3;plan.program=NormalGRedMovementPrograms::relaunch_active;plan.request=true;
    }else if(input.lower_stage==3)plan.program=NormalGRedMovementPrograms::relaunch_active;
    if(!character_movement_motion_valid(plan.motion))return false;
    *out=plan;return true;
}
inline bool gred_relaunch_lower_finished(const GRedRelaunchPlan& plan,bool done,GRedRelaunchState* out){
    if(!out||plan.state.lower_stage<1||plan.state.lower_stage>3)return false;
    auto state=plan.state;
    if(!plan.request&&state.lower_stage==1&&done)state.lower_stage=2;
    *out=state;return true;
}
struct GRedRelaunchUpperPlan {u8 next_stage=0,program=NormalGRedMovementPrograms::relaunch_windup;bool request=false;};
inline bool gred_relaunch_upper_plan(const GRedRelaunchState& input,GRedRelaunchUpperPlan* out){
    if(!out||input.lower_stage<1||input.lower_stage>3||input.upper_stage>3)return false;
    GRedRelaunchUpperPlan plan;plan.next_stage=input.upper_stage;
    if(!input.upper_stage){plan.next_stage=1;plan.request=true;}
    else if(input.upper_stage==1){if(input.lower_stage>1)plan.next_stage=2;} //update14 now, request15 NEXT update
     else if(input.upper_stage==2){plan.next_stage=3;plan.program=NormalGRedMovementPrograms::relaunch_active;plan.request=true;}
     else plan.program=NormalGRedMovementPrograms::relaunch_active;
    *out=plan;return true;
}
//6A750 clears754 on selecting8;6235C does not reinstall it. Static ret1 is
//therefore a height snap, NOT65E64/landing7. At declared delta1DC8=1 the
//caller's negative48 multiplier is8043751C (.8);50/87/latches stay intact.
//Default zero698 is a legacy declaration; owners with a live countdown pass it.
inline bool gred_relaunch_static_contact(const CharacterMovementProfile& p,const CharacterMovementStaticGround& ground,
                                         const CharacterMovementMotionState& before,const CharacterMovementMotionState& proposed,
                                          GRedSurfaceContact* out,std::string* error,f32 timer698=0){
    if(!out||!p.relaunch_floor_operands)return false;
    GRedSurfaceContact result;
    if(!gred_surface_contact(p,ground,before,proposed,{timer698,0,true,false},&result,error))return false;
    if(result.kind==GRedSurfaceContactKind::Snapped&&result.motion.vertical48<0)
        result.motion.vertical48=f32(result.motion.vertical48*p.relaunch_floor_damping);
    *out=result;return true;
}
}
