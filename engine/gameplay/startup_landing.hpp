#pragma once
// B2190/B2290/B2708/B2B20/B2CB0/B2CC4 kinds0/2 only. Explicit ground/parent
// observations; no allocation-pool parity, source placement producer or audio.
#include "engine/assets/startup_landing.hpp"
#include "engine/gameplay/stage_ground.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
struct StartupLandingRequest {u8 kind=0;std::array<f32,3> anchor{};f32 factor=0,height668=0; s16 yaw5ae=0;StageGroundActor ground_actor;};
struct StartupLandingState {
    bool allocated=false,registered=false,visible=false;u8 phase=0,kind=0,entry=0;f32 time=0,duration=0,alpha=1;
    u16 scale_key=0,alpha_key=0;std::array<f32,3> position{},scale{},base_scale{};s16 yaw=0;Mat4 world;
};
namespace startup_landing_detail {
inline bool world(const StartupLandingProfile& p,StartupLandingState* s){
    for(auto v:s->scale)if(!stage_ground_detail::number(v))return false;
    auto m=mat4_translation(s->position.data());m=mat4_multiply(m,mat4_rotation_z(0));m=mat4_multiply(m,mat4_rotation_y(f32(s->yaw)*p.angle_unit));m=mat4_multiply(m,mat4_rotation_x(0));m=mat4_multiply(m,mat4_scale(s->scale.data()));
    for(auto v:m.m)if(!stage_ground_detail::number(v))return false;
    s->world=m;return true;
}
inline bool curves(const StartupLandingProfile& p,StartupLandingState* s,bool initialize){
    const auto& e=p.entries[s->entry];std::array<f32,3> scale=s->scale;std::array<f32,1> alpha={s->alpha};
    if(initialize){s->scale_key=s->alpha_key=0;scale=e.scale.empty()?std::array<f32,3>{p.one,p.one,p.one}:e.scale[0].value;alpha=e.alpha.empty()?std::array<f32,1>{p.one}:e.alpha[0].value;}
    else if(!linear_key_sample<3>(e.scale,s->time,&s->scale_key,&scale)||!linear_key_sample<1>(e.alpha,s->time,&s->alpha_key,&alpha))return false;
    for(unsigned k=0;k<3;++k)s->scale[k]=f32(scale[k]*s->base_scale[k]);
    s->alpha=alpha[0];return std::isfinite(s->alpha)&&s->alpha>=0&&s->alpha<=1;
}
}
inline bool startup_landing_construct(const StartupLandingProfile& p,const std::array<const StihGrid*,3>& grids,const StartupLandingRequest& input,StartupLandingState* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup landing construct: ")+why;return false;};
    if(!out||!startup_landing_profile_valid(p)||(input.kind!=0&&input.kind!=2)||!stage_ground_detail::vector(input.anchor)||!stage_ground_detail::number(input.factor)||input.factor<=0||!stage_ground_detail::number(input.height668)||input.height668<=0)return fail("profile/request domain");
    StartupLandingState s;s.allocated=true;s.phase=1;s.kind=input.kind;s.entry=input.kind==2?2:0;s.duration=p.entries[s.entry].duration;s.position=input.anchor;s.yaw=input.kind==2?0:input.yaw5ae;
    f32 y=0;if(!stage_ground_height(grids,p.ground,input.ground_actor,input.anchor,input.height668,&y,error))return false;
    const f32 distance=std::abs(f32(input.anchor[1]-y));
    if(f32(input.height668*p.rejection_fraction)<=distance){s.phase=2;*out=s;if(error)error->clear();return true;}
    s.position[1]=y;s.registered=s.visible=true;s.base_scale.fill(input.factor);
    if(!startup_landing_detail::curves(p,&s,true)||!startup_landing_detail::world(p,&s))return fail("initial curve/world domain");
    *out=s;if(error)error->clear();return true;
}
inline bool startup_landing_step(const StartupLandingProfile& p,f32 scene_delta,const StartupLandingState& input,StartupLandingState* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup landing step: ")+why;return false;};
    if(!out||!startup_landing_profile_valid(p)||!stage_ground_detail::number(scene_delta)||scene_delta<0||scene_delta>8192||input.phase<1||input.phase>3||input.entry>2||(input.kind!=0&&input.kind!=2))return fail("profile/state/delta domain");
    if((input.kind==2?input.entry!=2:input.entry>1)||input.duration!=p.entries[input.entry].duration||!std::isfinite(input.time)||input.time<0||input.time>8192||!stage_ground_detail::vector(input.position)||!stage_ground_detail::vector(input.scale)||!stage_ground_detail::vector(input.base_scale)||!std::isfinite(input.alpha)||input.alpha<0||input.alpha>1||(input.visible&&!input.registered))return fail("state coherence/domain");
    auto s=input;if(!s.allocated){*out=s;if(error)error->clear();return true;}
    if(s.phase==1){s.time=f32(s.time+scene_delta);if(!std::isfinite(s.time)||s.time>8192)return fail("time budget");
        bool changed=false;
        if(s.duration<=s.time){if(s.kind==0&&s.entry==0){s.entry=1;s.time=0;s.duration=p.entries[1].duration;s.base_scale=s.scale;changed=true;}else s.phase=2;}
        // B2B20 initializes new curves AND samples them in the same callback,
        // including the terminal transition. It never consumes a second segment.
        if(changed&&!startup_landing_detail::curves(p,&s,true))return fail("segment curve initialization");
        if(!startup_landing_detail::curves(p,&s,false))return fail("curve domain");
    }else if(s.phase==2){s.phase=3;s.visible=false;}
    else{s.allocated=false;s.registered=false;s.visible=false;}
    if(s.allocated&&s.registered&&!startup_landing_detail::world(p,&s))return fail("registered world domain");
    *out=s;if(error)error->clear();return true;
}
} // namespace gf
