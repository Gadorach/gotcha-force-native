#pragma once
#include <optional>
#include "engine/gameplay/character_movement_motion.hpp"
namespace gf {
struct GRedGroundCorrectionFields {
    CharacterMotionVector previous2c{};u8 state4a0=0,force732=0,override1d9b=0;bool declared=false;
};
enum class GRedGroundCorrectionKind {Skipped,Height,Rewound,Falling};
struct GRedGroundLossTransition {
    u8 selector=6,fall_marker590=1,lower_program=13;
    f32 timer690=0,timer698=0;bool clear71b=true;
};
struct GRedGroundCorrectionPlan {
    CharacterMovementMotionState motion;GRedGroundCorrectionKind kind=GRedGroundCorrectionKind::Skipped;
    f32 queried_height=0,height_difference=0;bool queried=false;
    std::optional<GRedGroundLossTransition> falling{};
};
//68DF8/6A5A4: bounded detached, unblocked normal-G-Red numeric candidate.
//History is an explicit same-call input, not an inferred previous render frame.
//No input/VM dispatch, callback execution, actor, collision or lifetime receipt.
//Forced732, stored1D9B momentum, attachment/offset and blocked-upper paths refuse.
inline bool gred_ground_correction_plan(const CharacterMovementProfile& p,const CharacterMovementStaticGround& ground,
                                         const CharacterMovementMotionState& input,const GRedGroundCorrectionFields& fields,
                                         GRedGroundCorrectionPlan* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("G-Red ground correction: ")+why;return false;};
    if(!out||!character_movement_profile_valid(p)||!p.ground_loss_operands||p.model!=0x615||p.class_flags!=12||p.movement_type||
       !character_movement_motion_valid(input)||input.speed44<0||input.vertical_only598||input.field741||input.field6cb||
       !fields.declared||fields.force732||fields.override1d9b||(fields.state4a0!=0&&fields.state4a0!=2)||
       (input.flags5e0&0x81002003u)||!ground.valid_for(input)||ground.actor.offset6d0!=0||
       fields.previous2c[1]!=ground.actor.previous30)return fail("qualified GFMM8, explicit static history and reviewed source fields required");
    for(auto value:fields.previous2c)if(!std::isfinite(value)||std::abs(value)>1e7f)return fail("previous-position domain");
    GRedGroundCorrectionPlan plan;plan.motion=input;
    if((input.flags5e0&0x40u)||fields.state4a0==2){*out=plan;if(error)error->clear();return true;}
    if(!ground.height(input.position,&plan.queried_height,error))return false;
    plan.queried=true;plan.motion.position[1]=plan.queried_height;
    plan.height_difference=f32(plan.queried_height-fields.previous2c[1]);
    if(!std::isfinite(plan.height_difference))return fail("height-difference domain");
    plan.kind=GRedGroundCorrectionKind::Height;
    if(plan.height_difference< -ground.step674){
        //Restore incoming24, NOT previous30. Horizontal history can increase
        //44 and change travel5AE only when distance/5F8 is strictly greater.
        plan.motion.position[1]=input.position[1];
        const f32 x=f32(input.position[0]-fields.previous2c[0]),z=f32(input.position[2]-fields.previous2c[2]);
        f32 distance=std::fma(x,x,f32(z*z));
        if(!std::isfinite(distance))return fail("horizontal-history domain");
        if(distance>p.ground_loss_zero)distance=f32(std::sqrt(f64(distance)));
        const f32 speed=f32(distance/p.rank[2]);
        if(speed>input.speed44){
            if(!source_heading_s16(x,z,&plan.motion.travel5ae))return fail("history heading domain");
            plan.motion.speed44=speed;
        }
        plan.motion.flags5e0=input.flags5e0&0x00ff0041u;
        plan.kind=GRedGroundCorrectionKind::Falling;
        plan.falling=GRedGroundLossTransition{6,1,13,p.ground_loss_zero,p.ground_loss_contact_delay,true};
    }else if(plan.height_difference>ground.step674&&
             (input.position[0]!=fields.previous2c[0]||input.position[2]!=fields.previous2c[2])){
        plan.motion.position=fields.previous2c;plan.kind=GRedGroundCorrectionKind::Rewound;
    }
    if(!character_movement_motion_valid(plan.motion))return fail("derived movement domain");
    *out=plan;if(error)error->clear();return true;
}
}
