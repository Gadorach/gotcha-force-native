#pragma once
#include "engine/gameplay/character_movement_motion.hpp"
#include "engine/core/source_surface_push.hpp"
namespace gf {
enum class GRedSurfaceContactKind {Above,Leaving,Landed,Pushed,Snapped};
struct GRedSurfaceContact {
    CharacterMovementMotionState motion;StageGroundSurfaceResult surface;
    GRedSurfaceContactKind kind=GRedSurfaceContactKind::Above;s16 angle=0;
};
struct GRedSurfaceContactFields {f32 timer698=0;u8 field7c9=0;bool allow_push=false,landing_callback=true;};
//677B0: detached, zero-offset, static-surface landing subset. The selected
//height AND normal must come from3E0A8's same result. Above/away early returns
//are not landing receipts; native acceptance still owns final contact.
//Steep projection requires explicit opt-in/GFMM6. It is NOT a landing callback.
//Attachment and actor7C9 paths refuse.
inline bool gred_surface_contact(const CharacterMovementProfile& p,const CharacterMovementStaticGround& ground,
                                 const CharacterMovementMotionState& before,const CharacterMovementMotionState& proposed,
                                 GRedSurfaceContactFields fields,GRedSurfaceContact* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("G-Red surface contact: ")+why;return false;};
    if(!out||!character_movement_profile_valid(p)||!p.surface_operands||p.model!=0x615||p.class_flags!=12||p.movement_type||
       !character_movement_motion_valid(before)||!character_movement_motion_valid(proposed)||!ground.valid_for(before)||
       ground.actor.offset6d0!=0||fields.field7c9||!std::isfinite(fields.timer698)||fields.timer698<0||fields.timer698>1e5f)return fail("reviewed normal static fields required");
    if(!fields.landing_callback&&!p.relaunch_floor_operands)return fail("null-callback snap requires reviewed GFMM7 caller");
    GRedSurfaceContact result;result.motion=proposed;
    if(!ground.surface(proposed.position,&result.surface,error))return false;
    if(!result.surface.inside||!result.surface.selected)return fail("missing/outside selected surface is not a fabricated UP floor");
    if(proposed.position[1]>result.surface.height){*out=result;if(error)error->clear();return true;}
    CharacterMotionVector delta{};
    for(unsigned k=0;k<3;++k)delta[k]=f32(proposed.position[k]-before.position[k]);
    const f32 square=f32(f32(f32(delta[0]*delta[0])+f32(delta[1]*delta[1]))+f32(delta[2]*delta[2]));
    if(!std::isfinite(square))return fail("displacement domain");
    if(square>p.contact_motion_epsilon){
        const f64 length=std::sqrt(f64(square));
        for(auto& value:delta)value=f32(value/length); //shared bounded host numeric model, not GX/FPSCR normalization parity
        const auto& n=result.surface.normal;
        const f32 dot=f32(f32(f32(delta[0]*n[0])+f32(delta[1]*n[1]))+f32(delta[2]*n[2]));
        if(dot>0){result.kind=GRedSurfaceContactKind::Leaving;*out=result;if(error)error->clear();return true;}
    }
    if(!source_surface_angle(result.surface.normal,&result.angle))return fail("surface angle domain");
    const s16 limit=p.landing_angles[fields.timer698>0?1:0];
    if(result.angle>limit){
        if(!fields.allow_push||!p.steep_operands)return fail("steep push/damping branch requires enabled GFMM6 path");
        if(!source_surface_push_position(proposed.position,result.surface.normal,result.surface.height,result.angle,&result.motion.position))
            return fail("steep projection domain");
        result.motion.planar_change4c=0;
        result.motion.speed44=f32(proposed.speed44*p.steep_damping);
        result.motion.vertical48=f32(proposed.vertical48*p.steep_damping);
        //50 is deliberately retained; no callback754, cached87 reset or refill.
        result.kind=GRedSurfaceContactKind::Pushed;*out=result;if(error)error->clear();return true;
    }
    if(before.position[1]<result.surface.height-.01f)return fail("native raised-surface/side-entry response remains unsupported");
    result.motion.position[1]=result.surface.height;
    //677B0 itself only snaps height. Zeroing48/50 belongs to65E64; a
    //declared null754 keeps those operands for the caller's separate tail.
    if(fields.landing_callback){result.motion.vertical48=result.motion.vertical_change50=0;result.kind=GRedSurfaceContactKind::Landed;}
    else result.kind=GRedSurfaceContactKind::Snapped;
    *out=result;if(error)error->clear();return true;
}
}
