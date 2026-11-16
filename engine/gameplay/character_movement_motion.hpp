#pragma once
#include "engine/assets/character_movement.hpp"
#include "engine/core/source_heading.hpp"
#include "engine/gameplay/stage_ground_normal.hpp"
#include "engine/gameplay/stage_ground_surface.hpp"
namespace gf {
using CharacterMotionVector=std::array<f32,3>;
struct CharacterMovementMotionState {
    CharacterMotionVector position{};f32 speed44=0,vertical48=0,planar_change4c=0,vertical_change50=0;
    s16 travel5ae=0;u32 flags5e0=0;u8 vertical_only598=0,field741=0,field6cb=0;
};
inline bool character_movement_motion_valid(const CharacterMovementMotionState& s){
    for(f32 value:s.position)if(!std::isfinite(value)||std::abs(value)>1e7f)return false;
    for(f32 value:{s.speed44,s.vertical48,s.planar_change4c,s.vertical_change50})if(!std::isfinite(value)||std::abs(value)>1e5f)return false;
    return true;
}
//Read-only same-state static-grid adapter. Three real grids are required;
//the normal routine's default UP is not a substitute for missing grids.
struct CharacterMovementStaticGround {
    std::array<const StihGrid*,3> grids{};StageGroundParameters parameters;
    StageGroundActor actor;f32 query_height668=0,step674=0;
    bool valid_for(const CharacterMovementMotionState& state)const{
        for(const auto* grid:grids)if(!grid||!grid->loaded())return false;
        return actor.fields_declared&&!actor.attached1db&&actor.current24==state.position[1]&&
            stage_ground_parameters_valid(parameters)&&std::isfinite(query_height668)&&std::isfinite(step674)&&step674>=0;
    }
    bool height(const CharacterMotionVector& point,f32* out,std::string* error)const{
        return stage_ground_height(grids,parameters,actor,point,query_height668,out,error);
    }
    bool normal(const CharacterMotionVector& point,CharacterMotionVector* out,std::string* error)const{
        StageGroundNormal result;
        if(!out||!stage_ground_surface_normal(grids,parameters,actor,point,query_height668,&result,error))return false;
        *out=result.normal;return true;
    }
    bool surface(const CharacterMotionVector& point,StageGroundSurfaceResult* out,std::string* error)const{
        return stage_ground_surface(grids,parameters,actor,point,query_height668,out,error);
    }
};
inline CharacterMotionVector character_movement_cross(const CharacterMotionVector& a,const CharacterMotionVector& b){
    return {f32(a[1]*b[2]-a[2]*b[1]),f32(a[2]*b[0]-a[0]*b[2]),f32(a[0]*b[1]-a[1]*b[0])};
}
//670DC static branch: source tangent/height/step operations. This produces a
//candidate, NOT actor/body-contact or whole-frame acceptance.
inline bool character_movement_ground_step(const CharacterMovementProfile& p,const CharacterMovementMotionState& input,
                                           f32 rate5f4,const CharacterMovementStaticGround& ground,
                                           CharacterMovementMotionState* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("ground motion: ")+why;return false;};
    if(!out||!character_movement_profile_valid(p)||!character_movement_motion_valid(input)||!std::isfinite(rate5f4)||rate5f4<0||rate5f4>16)return fail("profile/state/clock domain");
    auto s=input;
    if(!s.vertical_only598&&std::abs(s.speed44)>p.epsilon){
        if(!ground.valid_for(input))return fail("current static ground observation required");
        f32 x=0,z=0;source_heading_xz(s.travel5ae,&x,&z);const f32 delta=f32(rate5f4*p.rank[2]);
        CharacterMotionVector velocity{f32(s.speed44*x),0,f32(s.speed44*z)};
        if(!(s.flags5e0&0x40u)){
            CharacterMotionVector normal;
            if(!ground.normal(s.position,&normal,error))return false;
            velocity=character_movement_cross(character_movement_cross(normal,velocity),normal);
            const f64 length=std::hypot(f64(velocity[0]),f64(velocity[1]),f64(velocity[2]));
            if(!std::isfinite(length)||length==0)return fail("surface tangent cannot be normalized");
            const f32 distance=f32(s.speed44*delta);
            for(auto& value:velocity)value=f32(f32(value/length)*distance);
            s.position[0]=f32(s.position[0]+velocity[0]);s.position[2]=f32(s.position[2]+velocity[2]);
        }else{
            s.position[0]=std::fma(velocity[0],delta,s.position[0]);s.position[2]=std::fma(velocity[2],delta,s.position[2]);
        }
        f32 y=0;if(!ground.height(s.position,&y,error))return false;
        if(s.flags5e0&0x40u){if(s.position[1]<y)s.position[1]=y;}
        else s.position[1]=y;
        s.speed44=std::max(0.f,std::fma(s.planar_change4c,rate5f4,s.speed44));
        if(!(s.flags5e0&0x40u)){
            const f32 difference=f32(s.position[1]-input.position[1]);
            if(difference< -ground.step674)s.position[1]=input.position[1];
            else if(difference>ground.step674)s.position=input.position;
        }
    }
    if(!character_movement_motion_valid(s))return fail("derived motion domain");
    *out=s;if(error)error->clear();return true;
}
enum class CharacterMovementCeilingMode {Unknown,DeclaredStatic,DeclaredSourceBypass};
struct CharacterMovementCeiling {
    CharacterMovementCeilingMode mode=CharacterMovementCeilingMode::Unknown;
    f32 stage_top34=0,query_height668=0,previous_y30=0;bool attached=false;
};
//67310/67458 numeric path with the explicit68030 ceiling boundary. No moving
//platform, collision, contact-tail or full actor-frame acknowledgement.
inline bool character_movement_air_step(const CharacterMovementProfile& p,const CharacterMovementMotionState& input,
                                        f32 rate5f4,f32 acceleration_multiplier,bool clamp_wrapper,
                                        const CharacterMovementCeiling& ceiling,CharacterMovementMotionState* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("air motion: ")+why;return false;};
    if(!out||!character_movement_profile_valid(p)||!character_movement_motion_valid(input)||!std::isfinite(rate5f4)||rate5f4<0||rate5f4>16||
       !std::isfinite(acceleration_multiplier)||acceleration_multiplier<0||acceleration_multiplier>16||ceiling.attached||unsigned(ceiling.mode)>2)return fail("profile/state/clock/static domain");
    auto s=input;f32 x=0,z=0;source_heading_xz(s.travel5ae,&x,&z);const f32 delta=f32(rate5f4*p.rank[2]);
    s.position[0]=std::fma(delta,f32(s.speed44*x),s.position[0]);s.position[1]=std::fma(s.vertical48,delta,s.position[1]);s.position[2]=std::fma(delta,f32(s.speed44*z),s.position[2]);
    if(s.vertical48>0){
        if(ceiling.mode==CharacterMovementCeilingMode::Unknown)return fail("ascending motion requires ceiling-policy observation");
        if(ceiling.mode==CharacterMovementCeilingMode::DeclaredStatic){
            for(f32 value:{ceiling.stage_top34,ceiling.query_height668,ceiling.previous_y30})if(!std::isfinite(value)||std::abs(value)>1e7f)return fail("ceiling observation domain");
            if(s.position[1]+ceiling.query_height668>ceiling.stage_top34){s.vertical48=0;s.position[1]=std::max(ceiling.previous_y30,f32(ceiling.stage_top34-ceiling.query_height668));}
        }
    }
    s.speed44=std::max(0.f,std::fma(s.planar_change4c,rate5f4,s.speed44));
    s.vertical48=std::fma(rate5f4,f32(s.vertical_change50*acceleration_multiplier),s.vertical48);
    const f32 minimum=s.field741||s.field6cb?p.special_min_vertical:p.limits[2];
    s.vertical48=std::max(s.vertical48,minimum);
    if(clamp_wrapper){
        if(s.planar_change4c>0)s.speed44=std::clamp(s.speed44,0.f,p.limits[0]);
        if(!(s.field741||s.field6cb))s.vertical48=std::clamp(s.vertical48,p.limits[2],p.limits[1]);
    }
    if(!character_movement_motion_valid(s))return fail("derived motion domain");
    *out=s;if(error)error->clear();return true;
}
}
