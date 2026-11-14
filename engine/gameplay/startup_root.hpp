#pragma once
// Source80058044 reset and80057EA0 exit arithmetic. Effects remain explicit.
#include <array>
#include "engine/assets/startup_root.hpp"
namespace gf {
struct StartupRootState {
    std::array<f32,3> actor_scale{},root_scale{};
    f32 scalar_b4=0,scalar_b8=0,remaining=0; // retained source fields, no extra semantics inferred
    bool initialized=false;
};
struct StartupRootStep {StartupRootState state;u8 substate=0;bool fade12=false,sound242=false,hide=false,updated=false;};
inline bool startup_root_state_valid(const StartupRootState& s){
    if(!s.initialized||!std::isfinite(s.remaining)||s.remaining<0||s.remaining>8192)return false;
    for(f32 v:{s.scalar_b4,s.scalar_b8})if(!std::isfinite(v)||std::abs(v)>1e4f)return false;
    for(auto values:{s.actor_scale,s.root_scale})for(auto v:values)if(!std::isfinite(v)||std::abs(v)>1e4f)return false;
    return true;
}
inline bool startup_root_reset(const StartupRootProfile& p,StartupRootState* out){
    if(!out||!startup_root_profile_valid(p))return false;
    StartupRootState s;s.actor_scale.fill(p.reset_scale);s.root_scale.fill(p.reset_scale);s.scalar_b4=s.scalar_b8=p.reset_scale;s.initialized=true;*out=s;return true;
}
inline bool startup_root_exit_step(const StartupRootProfile& p,const StartupRootState& state,u8 substate,StartupRootStep* out){
    if(!out||!startup_root_profile_valid(p)||!startup_root_state_valid(state)||substate>2)return false;
    StartupRootStep next;next.state=state;next.substate=substate;
    if(substate==2){*out=next;return true;}
    if(substate==0){next.state.remaining=p.lifetime;next.substate=1;next.fade12=next.sound242=true;}
    else if(state.remaining<=0||state.remaining>p.lifetime)return false;
    next.state.remaining=f32(next.state.remaining-p.step);
    const f32 ratio=f32(next.state.remaining/p.lifetime),scaled=f32(p.angle_span*ratio);
    if(!std::isfinite(scaled)||scaled< -32768||scaled>32767)return false;
    const s16 angle=s16(s32(scaled)); //source fctiwz, then signed16 PSQ load
    const f32 radians=f32(f32(angle)*p.angle_unit),sine=f32(std::sin(f64(radians)));
    const f32 xz=std::fma(p.amplitude,sine,p.bias),y=std::fma(-p.y_factor,xz,p.y_base);
    next.state.root_scale={xz,y,xz};next.updated=true;
    if(next.state.remaining<=0){next.substate=2;next.hide=true;}
    if(!startup_root_state_valid(next.state))return false;
    *out=next;return true;
}
} // namespace gf
