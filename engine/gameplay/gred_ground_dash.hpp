#pragma once
#include "engine/gameplay/character_movement_heading.hpp"
namespace gf {
struct GRedDashSteering {
    s16 body72=0,camera5b0=0,travel5ae=0,desired5ac=0,delta5aa=0;
    u8 direction57c=0,coarse57d=0;f32 distance75c=0,speed44=0;bool distance_declared=false,completed547=false;
};
//65EB0 normal-G-Red branch. Distance75C must come from its accepted target
//producer; this pure numeric operation grants no target/camera/actor authority.
inline bool gred_ground_dash_steer(const CharacterMovementProfile& p,const GRedDashSteering& input,GRedDashSteering* out){
    if(!out||!character_movement_profile_valid(p)||p.model!=0x615||(p.class_flags&0x20u)||input.direction57c>3||input.coarse57d>3||
       !std::isfinite(input.speed44)||std::abs(input.speed44)>1e5f)return false;
    if(!input.completed547&&input.coarse57d!=2&&(!input.distance_declared||!std::isfinite(input.distance75c)||input.distance75c<0||input.distance75c>1e7f))return false;
    auto s=input;const s16 forward=source_heading_wrap(s32(s.camera5b0)-0x8000);
    if(!s.completed547){
        if(!s.coarse57d){if(s.distance75c<p.dash_steering[0])s.completed547=true;else s.travel5ae=forward;}
        else if(s.coarse57d==2)s.travel5ae=s.camera5b0;
        else{
            const f32 curve=s.distance75c>=p.dash_steering[0]?(s.distance75c>=p.dash_steering[1]?p.dash_steering[4]:p.dash_steering[3]):f32(p.dash_steering[3]-p.dash_near_delta);
            const f32 angle=f32(p.dash_angle_factor*curve);if(!std::isfinite(angle)||std::abs(angle)>1e7f)return false;
            const s16 side=source_heading_wrap(0x4000-source_heading_wrap(s32(angle)));
            s.travel5ae=source_heading_wrap(s32(forward)+(s.coarse57d==1?s32(side):-s32(side)));
        }
    }
    s.desired5ac=source_heading_wrap(s32(s.travel5ae)-s32(s.direction57c)*0x4000);
    s.delta5aa=source_heading_wrap(s32(s.desired5ac)-s.body72);
    if(s.speed44>0){const s16 turn=source_heading_wrap(s32(f32(p.dash_body_gain*f32(s.delta5aa))));s.body72=source_heading_wrap(s32(s.body72)+turn);s.delta5aa=source_heading_wrap(s32(s.delta5aa)-turn);}
    *out=s;return true;
}
struct GRedGroundDashState {
    u8 selector=12,direction_class=0,coarse57d=0;f32 timer568=0;bool recovery_entered=false;
};
struct GRedGroundDashPlan {
    GRedGroundDashState state;f32 speed44=0,change4c=0;
    bool request_program=false,advance_program=false,move=false;u8 program=0;
};
//Split source handler planning around the VM callback: timer/exit tests consume
//the post-callback control byte, not a guessed clip length or render-frame timer.
inline bool gred_ground_dash_plan(const CharacterMovementProfile& p,const GRedGroundDashState& input,
                                 f32 speed,f32 change,u8 control_before,GRedGroundDashPlan* out){
    if(!out||!character_movement_profile_valid(p)||p.model!=0x615||p.movement_type||input.direction_class>3||input.coarse57d>3||
       !std::isfinite(input.timer568)||std::abs(input.timer568)>1e5f||!std::isfinite(speed)||!std::isfinite(change))return false;
    GRedGroundDashPlan plan;plan.state=input;plan.speed44=speed;plan.change4c=change;
    const auto& operands=p.ground_dash[input.direction_class==0?0:input.direction_class==2?2:1];
    if(input.selector==12){plan.speed44=operands[0];plan.change4c=operands[1];plan.state.timer568=operands[3];plan.state.selector=13;plan.state.recovery_entered=false;plan.request_program=true;plan.program=u8(27+input.direction_class);}
    else if(input.selector==13){if(plan.speed44<operands[2]){plan.speed44=operands[2];plan.change4c=0;}plan.advance_program=!(control_before&1u);plan.move=true;}
    else if(input.selector==14){
        //CFF4's forced-continuation branch needs a separately reviewed owner.
        //Do not skip it or walk beyond the closed GFMP program on an unusual entry.
        if(!input.recovery_entered&&(control_before&2u))return false;
        plan.state.recovery_entered=true;plan.advance_program=true;plan.move=true;
    }else return false;
    *out=plan;return true;
}
inline bool gred_ground_dash_finish(const CharacterMovementProfile& p,const GRedGroundDashPlan& plan,u8 control_after,
                                   bool program_done,u32 held5b4,f32 delta1dc8,GRedGroundDashState* out){
    if(!out||!character_movement_profile_valid(p)||p.model!=0x615||p.movement_type||plan.state.coarse57d>3||plan.state.direction_class>3||
       (plan.state.selector!=13&&plan.state.selector!=14)||!std::isfinite(plan.state.timer568)||std::abs(plan.state.timer568)>1e5f||
       !std::isfinite(delta1dc8)||delta1dc8<0||delta1dc8>16)return false;
    auto state=plan.state;
    if(!plan.request_program&&state.selector==13&&control_after){
        state.timer568=f32(state.timer568-delta1dc8);
        if(state.timer568<=0||!(held5b4&p.held_dash_masks[state.coarse57d])){state.selector=14;state.recovery_entered=false;}
    }else if(state.selector==14&&program_done)state.selector=0;
    *out=state;return true;
}
}
