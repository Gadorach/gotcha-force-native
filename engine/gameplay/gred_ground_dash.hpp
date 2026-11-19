#pragma once
#include "engine/gameplay/borgs/normal_gred_movement_programs.hpp"
#include "engine/gameplay/character_movement_heading.hpp"
#include "engine/gameplay/character_movement_motion.hpp"
#include "engine/gameplay/character_motion_root_state.hpp"
namespace gf {
inline bool gred_ground_dash_program(const CharacterMovementProfile& profile,const CharacterMovementHeading& heading,u8* program,u8* direction_class){
    if(!program||!direction_class||!character_movement_profile_valid(profile)||profile.model!=0x615||(profile.class_flags&0x20u)||!heading.has_direction)return false;
    const auto klass=profile.direction_classes[source_heading_sector(heading.delta5aa,0)];
    *direction_class=klass;*program=NormalGRedMovementPrograms::ground_dash[klass];return true;
}
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
    if(input.selector==12){plan.speed44=operands[0];plan.change4c=operands[1];plan.state.timer568=operands[3];plan.state.selector=13;plan.state.recovery_entered=false;plan.request_program=true;plan.program=NormalGRedMovementPrograms::ground_dash[input.direction_class];}
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
//Ordered numeric subset of60C94/60F78/61160 and their caller tails.
//The older request-only entry API above remains a labelled legacy prefix.
//These candidates grant NO VM/root-sample provenance, contact or frame receipt.
struct GRedGroundDashOrderedState {
    GRedGroundDashState dash;u8 recovery546=0;bool exit545=false;
};
struct GRedGroundDashOrderedPlan {
    GRedGroundDashOrderedState state;CharacterMovementMotionState motion;
    bool request_program=false,advance_program=false,steer_before_program=false;
    u8 program=0;f32 steering_speed44=0;
};
struct GRedGroundDashOrderedUpdate {
    GRedGroundDashOrderedState state;CharacterMovementMotionState motion;
    bool steer_after_program=false,clear_control1d0f=false;
};
struct GRedGroundDashOrderedResult {
    GRedGroundDashOrderedState state;CharacterMovementMotionState motion;bool idle_reset=false;
};
inline bool gred_ground_dash_ordered_domain(const CharacterMovementProfile& p,const GRedGroundDashOrderedState& s,
                                            const CharacterMovementMotionState& motion){
    return character_movement_profile_valid(p)&&p.model==0x615&&p.class_flags==12&&!p.movement_type&&p.ordered_dash_operands&&
        character_movement_motion_valid(motion)&&motion.speed44>=0&&!(motion.flags5e0&0x2040u)&&s.recovery546<=1&&
        s.dash.direction_class<=3&&s.dash.coarse57d<=3&&std::isfinite(s.dash.timer568)&&std::abs(s.dash.timer568)<=1e5f&&
        (s.dash.selector==12||s.dash.selector==13||s.dash.selector==14);
}
//Active steering uses the INCOMING speed, before the minimum clamp and VM.
//Entry requests first; recovery advances first. Their steering is post-VM.
inline bool gred_ground_dash_ordered_plan(const CharacterMovementProfile& p,const GRedGroundDashOrderedState& input,
                                         const CharacterMovementMotionState& motion,u8 control_before,GRedGroundDashOrderedPlan* out){
    if(!out||!gred_ground_dash_ordered_domain(p,input,motion)||
       (!input.dash.recovery_entered&&input.recovery546)||
       (input.dash.selector!=14&&(input.dash.recovery_entered||input.recovery546))||
       (input.dash.selector==12&&input.exit545))return false;
    GRedGroundDashPlan legacy;
    if(!gred_ground_dash_plan(p,input.dash,motion.speed44,motion.planar_change4c,control_before,&legacy))return false;
    GRedGroundDashOrderedPlan plan;plan.state=input;plan.motion=motion;
    plan.request_program=legacy.request_program;plan.advance_program=legacy.advance_program;plan.program=legacy.program;
    plan.steer_before_program=input.dash.selector==13;plan.steering_speed44=motion.speed44;
    plan.motion.speed44=legacy.speed44;plan.motion.planar_change4c=legacy.change4c;
    if(input.dash.selector==12){
        plan.state.dash.timer568=legacy.state.timer568;
        plan.motion.vertical48=p.zero;plan.motion.vertical_change50=p.air[10]; //data6C
    }
    *out=plan;return true;
}
//Call AFTER the indicated VM operation. Recovery needs that operation's root
//state even when lane1 metadata1D9B==0: it suppresses position, not speed1DAC.
//The caller must own that sample; updates>0 is a numeric check, not a lease.
inline bool gred_ground_dash_ordered_update(const CharacterMovementProfile& p,const GRedGroundDashOrderedPlan& plan,
                                           u8 control_after,u32 held5b4,f32 delta1dc8,
                                           const CharacterMotionRootState* root_after_program,GRedGroundDashOrderedUpdate* out){
    if(!out||!gred_ground_dash_ordered_domain(p,plan.state,plan.motion)||
       !std::isfinite(delta1dc8)||delta1dc8<0||delta1dc8>16)return false;
    GRedGroundDashOrderedUpdate update;update.state=plan.state;update.motion=plan.motion;
    auto& s=update.state;
    if(s.dash.selector==12)update.steer_after_program=true;
    else if(s.dash.selector==13){
        if(control_after){
            s.dash.timer568=f32(s.dash.timer568-delta1dc8);
            if(s.dash.timer568<=0||!(held5b4&p.held_dash_masks[s.dash.coarse57d]))s.exit545=true;
        }
    }else{
        if(!root_after_program||!character_motion_root_state_valid(*root_after_program)||!root_after_program->updates||
           root_after_program->speed1dac>1e5f)return false;
        if(!s.dash.recovery_entered)s.exit545=false; //61160 entry clears545
        s.dash.recovery_entered=true;
        if(!s.exit545&&control_after==3){s.exit545=true;update.clear_control1d0f=true;}
        const f32 root_speed=root_after_program->speed1dac;
        if(!s.recovery546){
            update.motion.speed44=f32(update.motion.speed44*p.dash_recovery_damping);
            if(update.motion.speed44<root_speed){update.motion.speed44=root_speed;s.recovery546=1;}
        }else update.motion.speed44=root_speed;
        update.steer_after_program=update.motion.speed44>0;
    }
    if(!character_movement_motion_valid(update.motion))return false;
    *out=update;return true;
}
//Call only AFTER shared ground motion (including its scalar update). No early
//idle reset on done:61084 tests1D0E after670DC. Idle flag/program writes remain
//the caller's separately scoped VM operation; this result does not execute them.
inline bool gred_ground_dash_ordered_finish(const CharacterMovementProfile& p,const GRedGroundDashOrderedUpdate& update,
                                           const CharacterMovementMotionState& moved,bool program_done,GRedGroundDashOrderedResult* out){
    if(!out||!gred_ground_dash_ordered_domain(p,update.state,update.motion)||
       !gred_ground_dash_ordered_domain(p,update.state,moved))return false;
    GRedGroundDashOrderedResult result;result.state=update.state;result.motion=moved;auto& s=result.state;
    if(s.dash.selector==12){s.dash.selector=13;s.dash.recovery_entered=false;s.exit545=false;s.recovery546=0;}
    else if(s.dash.selector==13&&s.exit545){s.dash.selector=14;s.dash.recovery_entered=false;s.exit545=false;s.recovery546=0;}
    else if(s.dash.selector==14&&program_done){
        s.dash.selector=0;s.dash.recovery_entered=false;s.exit545=false;s.recovery546=0;
        result.motion.speed44=result.motion.planar_change4c=0;result.idle_reset=true;
        result.motion.vertical48=result.motion.vertical_change50=0;
    }
    *out=result;return true;
}
} //namespace gf
