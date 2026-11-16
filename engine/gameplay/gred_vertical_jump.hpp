#pragma once
#include "engine/gameplay/character_movement_motion.hpp"
#include "engine/gameplay/character_air_resource.hpp"
#include "engine/gameplay/character_movement_heading.hpp"
namespace gf {
//Selectors4/5/6/7 planner. Neutral mode stays closed; moving mode requires
//GFMM3 and the owner's ordered heading/VM/flat-contact consumers. Neither
//mode admits relaunch/dash, effects, attachments or original contact receipts.
struct GRedVerticalJumpState {
    u8 selector=0,cached87=0;bool entered=false;
    u8 ascent_stage=0;bool direction550=false;f32 timer690=0;
    u8 air_ready588=0;bool falling_after_dash=false;
    bool falling_after_relaunch=false;u8 falling_coarse57d=0;
};
struct GRedVerticalJumpPlan {
    GRedVerticalJumpState state;CharacterMovementMotionState motion;
    bool request=false,advance=false,air_move=false,clamp_wrapper=false,landing_prefix=false;
    u8 program=0,cost_index=255;
};
inline bool gred_vertical_jump_plan(const CharacterMovementProfile& p,const GRedVerticalJumpState& input,
                                   const CharacterMovementMotionState& motion,s16 resource,u32 held,GRedVerticalJumpPlan* out,bool moving=false){
    if(!out||!character_movement_profile_valid(p)||p.model!=0x615||p.class_flags!=12||p.movement_type||
       !character_movement_motion_valid(motion)||(!moving&&(motion.speed44!=0||motion.planar_change4c!=0))||resource<0||
       (motion.flags5e0&0x2000u)||motion.field741||motion.field6cb||motion.vertical_only598||
       !std::isfinite(input.timer690)||input.timer690<0||input.timer690>1e5f||
       (moving&&!p.air_operands)||(moving&&input.selector==5&&input.entered&&input.ascent_stage!=1&&input.ascent_stage!=2)||
       (held&~(moving?0xf0100u:0x100u))||input.cached87>1||input.falling_coarse57d>3||input.selector<4||input.selector>7)return false;
    GRedVerticalJumpPlan plan;plan.state=input;plan.motion=motion;
    //5's release/resource admission precedes the lower callback. Its entry
    //sets588=1; a freshly selected5 still performs its initial launch update.
    if(plan.state.selector==5&&plan.state.entered&&(!(held&0x100u)||!resource)){
        plan.state.selector=6;plan.state.entered=false;
    }
    const bool entry=!plan.state.entered;plan.state.entered=true;
    plan.request=entry;plan.advance=!entry;
    switch(plan.state.selector){
    case 4:
        plan.program=2;plan.state.cached87=0;
        if(entry)plan.motion.speed44=plan.motion.planar_change4c=0; //normal class12, not class flag1 momentum preservation
        plan.motion.vertical48=plan.motion.vertical_change50=0;break;
    case 5:
        plan.program=4;plan.state.cached87=1;plan.air_move=plan.clamp_wrapper=true;plan.cost_index=entry?0:1;
        if(entry){plan.motion.vertical48=p.air[1];plan.state.ascent_stage=1;plan.state.timer690=p.air_ready_delay;plan.state.air_ready588=1;}
        //Even neutral ascent evaluates cos(0), rather than using data68 alone.
        plan.motion.vertical_change50=std::fma(p.air[0],1.f,p.air[9]);
        break;
    case 6:
        plan.program=input.falling_after_relaunch?u8(21+input.falling_coarse57d):input.falling_after_dash?13:6;plan.state.cached87=1;plan.air_move=true;
        if(plan.state.timer690<=0)plan.state.air_ready588=1;
        if(entry){plan.motion.vertical_change50=p.air[10];plan.motion.planar_change4c=0;}
        break;
    case 7:
        plan.program=8;plan.state.cached87=0;plan.landing_prefix=entry;
        plan.motion.speed44=plan.motion.planar_change4c=0;
        plan.motion.vertical48=plan.motion.vertical_change50=0;break;
    default:return false;
    }
    *out=plan;return true;
}
inline bool gred_jump_acceleration(const CharacterMovementProfile& p,bool direction,CharacterMovementMotionState* motion){
    if(!motion||!character_movement_motion_valid(*motion)||!p.air_operands||!character_movement_profile_valid(p)||p.model!=0x615||p.class_flags!=12||p.movement_type)return false;
    f32 sine=0,cosine=1;source_heading_xz(direction?p.ascent_pitch:0,&sine,&cosine);
    auto result=*motion;result.planar_change4c=f32(p.air[0]*sine);result.vertical_change50=std::fma(p.air[0],cosine,p.air[9]);
    if(!direction)result.speed44=f32(result.speed44*p.air_damping[0]); //delta1DC8=1
    if(!character_movement_motion_valid(result))return false;
    *motion=result;return true;
}
//676D8 adds a direct camera-relative displacement; it does NOT turn the body
//or replace retained scalar momentum/travel heading in the following air step.
inline bool gred_jump_falling_drift(const CharacterMovementProfile& p,const CharacterMovementHeading& heading,CharacterMovementMotionState* motion){
    if(!motion||!character_movement_motion_valid(*motion)||!p.air_operands||!character_movement_profile_valid(p)||p.model!=0x615||p.class_flags!=12||p.movement_type)return false;
    auto result=*motion;
    if(heading.has_direction){f32 x=0,z=0;source_heading_xz(heading.desired5ac,&x,&z);const f32 distance=f32(p.rank[2]*p.air[4]); //source5F4=1
        result.position[0]=std::fma(distance,x,result.position[0]);result.position[2]=std::fma(distance,z,result.position[2]);}
    result.speed44=f32(result.speed44*p.air_damping[2]);
    if(!character_movement_motion_valid(result))return false;
    *motion=result;return true;
}
inline bool gred_vertical_jump_finish(const GRedVerticalJumpPlan& plan,bool lower_done,bool touchdown,GRedVerticalJumpState* out){
    if(!out||plan.state.selector<4||plan.state.selector>7||(touchdown&&plan.state.selector!=6))return false;
    auto state=plan.state;
    if(state.selector==4&&plan.advance&&lower_done){state.selector=5;state.entered=false;}
    else if(state.selector==6&&touchdown){state.selector=7;state.entered=false;}
    else if(state.selector==7&&plan.advance&&lower_done){state.selector=0;state.entered=false;}
    *out=state;return true;
}
//Reviewed677B0 flat, detached, zero-offset branch only. Sloped/attached
//responses are NOT synthesized. Called on a candidate; commits need world ACK.
inline bool gred_vertical_jump_flat_contact(const CharacterMovementStaticGround& ground,
                                           const CharacterMovementMotionState& before,const CharacterMovementMotionState& proposed,
                                           CharacterMovementMotionState* out,bool* touchdown,std::string* error,bool moving=false){
    if(!out||!touchdown||!character_movement_motion_valid(before)||!character_movement_motion_valid(proposed)||
       !ground.valid_for(before)||ground.actor.offset6d0!=0||(!moving&&(proposed.position[0]!=before.position[0]||
       proposed.position[2]!=before.position[2])))return false;
    CharacterMotionVector normal;f32 floor=0;
    if(!ground.height(proposed.position,&floor,error))return false;
    //677B0 returns before angle/response tests when the actor remains above
    //the queried surface. Overflight does not authorize a sloped landing.
    if(proposed.position[1]>floor){*out=proposed;*touchdown=false;return true;}
    if(!ground.normal(proposed.position,&normal,error)||
       std::abs(normal[0])>.00001f||std::abs(normal[1]-1)>.00001f||std::abs(normal[2])>.00001f||
       before.position[1]<floor-.01f)return false;
    if(moving){
        f32 previous_floor=0;CharacterMotionVector previous_normal;
        if(!ground.height(before.position,&previous_floor,error)||!ground.normal(before.position,&previous_normal,error)||
           std::abs(previous_floor-floor)>.01f||std::abs(previous_normal[0])>.00001f||
           std::abs(previous_normal[1]-1)>.00001f||std::abs(previous_normal[2])>.00001f)return false;
    }
    auto accepted=proposed;const bool hit=proposed.position[1]<=floor&&proposed.position[1]<=before.position[1];
    if(hit){accepted.position[1]=floor;accepted.vertical48=accepted.vertical_change50=0;}
    *out=accepted;*touchdown=hit;return true;
}
}
