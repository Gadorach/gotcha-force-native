#pragma once
#include "engine/gameplay/character_movement_motion.hpp"
namespace gf {
//Normal G-Red resource policy1 only. Pure handler planning, not an original
//actor/solid/effect callback receipt. VM, resource spending and contact ordering
//remain with the same transactional source owner.
struct GRedAirDashState {
    u8 selector=15,stage=0,air_ready588=0;bool entered=false,contact_callback=false;
    f32 timer568=0,contact69c=0;
};
struct GRedAirDashPlan {
    GRedAirDashState state;CharacterMovementMotionState motion;
    bool request=false,advance=false,move=true,turn_windup=false,steer_active=false,cost_after_move=false;
    u8 program=0,cost_index=255;
};
inline bool gred_air_dash_plan(const CharacterMovementProfile& p,const GRedAirDashState& input,
                               const CharacterMovementMotionState& motion,GRedAirDashPlan* out){
    if(!out||!character_movement_profile_valid(p)||!p.air_dash_operands||p.model!=0x615||p.class_flags!=12||p.movement_type||
       !character_movement_motion_valid(motion)||(motion.flags5e0&0x2000u)||motion.vertical_only598||motion.field741||motion.field6cb||
       input.selector<15||input.selector>17||input.stage>2||input.air_ready588>1||
       (input.entered&&((input.selector==16&&input.stage==0)||(input.selector!=16&&input.stage!=1)))||
       !std::isfinite(input.timer568)||std::abs(input.timer568)>1e5f||
       !std::isfinite(input.contact69c)||input.contact69c<0||input.contact69c>1e5f)return false;
    GRedAirDashPlan plan;plan.state=input;plan.motion=motion;
    const bool entry=!input.entered;plan.state.entered=true;plan.request=entry;plan.advance=!entry;
    plan.state.contact69c=p.air_dash_contact_delay;
    if(input.selector==15){
        plan.program=10;
        if(entry){
            plan.state.stage=1;plan.state.contact_callback=false;plan.state.air_ready588=0;plan.move=false;
            plan.motion.speed44=f32(motion.speed44*p.air_dash_windup_factor);
            plan.motion.vertical48=f32(motion.vertical48*p.air_dash_windup_factor);
            plan.motion.planar_change4c=0;plan.motion.vertical_change50=p.air[11];
        }else{
            plan.turn_windup=true;plan.state.timer568=f32(input.timer568-1);
            if(plan.state.timer568<=0){plan.state.selector=16;plan.state.entered=false;plan.state.stage=0;plan.state.air_ready588=0;plan.state.contact_callback=false;plan.cost_index=2;plan.cost_after_move=true;}
        }
    }else if(input.selector==16){
        plan.program=11;plan.steer_active=true;plan.cost_index=3;
        if(entry){
            plan.state.stage=1;plan.state.timer568=p.air[8];plan.state.air_ready588=0;plan.state.contact_callback=false;
            plan.motion.speed44=p.air[5];plan.motion.planar_change4c=p.air[6];
            plan.motion.vertical48=p.air[7];plan.motion.vertical_change50=p.air[11];
        }else{
            if(input.stage==1){
                plan.state.timer568=f32(input.timer568-1);
                if(plan.state.timer568<=0){plan.state.stage=2;plan.state.air_ready588=1;plan.state.contact_callback=true;plan.motion.planar_change4c=0;}
            }
            if(plan.motion.speed44<0)plan.motion.speed44=plan.motion.planar_change4c=0;
        }
    }else{
        plan.program=12;
        if(entry){plan.state.stage=1;plan.state.air_ready588=1;plan.state.contact_callback=true;plan.motion.vertical_change50=p.air[10];}
    }
    *out=plan;return true;
}
inline bool gred_air_dash_spent(const GRedAirDashPlan& plan,s16 resource_after,GRedAirDashState* out){
    if(!out||resource_after<0||plan.state.selector<15||plan.state.selector>17)return false;
    auto state=plan.state;
    //Policy1's 550 is0: the active timer enables release/landing but does not
    //force recovery. Depletion switches after this update's resource spend.
    if(plan.program==11&&resource_after<1){
        state.selector=17;state.entered=false;state.stage=0;state.contact_callback=false;
    }
    *out=state;return true;
}
struct GRedAirDashContactTail {bool landing=false,falling=false;};
//617E4 tests lower completion AFTER motion/677B0 contact. A steep push is
//not touchdown and therefore cannot invoke landing/refill or bypass program12.
//Unknown simultaneous landing/completion remains refused by the native owner.
inline bool gred_air_dash_contact_tail(u8 selector,bool callback,bool lower_done,bool touchdown,GRedAirDashContactTail* out){
    if(!out||selector<15||selector>17||(touchdown&&(!callback||(selector==17&&lower_done))))return false;
    *out={touchdown,!touchdown&&selector==17&&lower_done};return true;
}
}
