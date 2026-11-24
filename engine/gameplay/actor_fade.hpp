#pragma once
// Source800061F0/80006238/80006284 andfade-mobj8008D7EC. Delta is supplied
// actor1DC8, NOT recovered wall-clock/FPS. No actor/world/startup admission.
#include "engine/assets/actor_fade.hpp"
namespace gf {
struct ActorFadeChannel {u8 curve=255,loops=0;f32 time=0;u32 rgba=0;bool active=false;};
struct ActorFadeState {std::array<ActorFadeChannel,2> channels{};u32 material_rgba=0x000000ffu,profile_binding=0;bool initialized=false;};
struct ActorFadeMaterial {std::array<u8,3> rgb{};bool source_class=false;};
namespace actor_fade_detail {
inline const ActorFadeCurve* curve(const ActorFadeProfile& p,u8 id){for(unsigned i=0;i<p.curve_count();++i)if(p.curves[i].id==id)return &p.curves[i];return nullptr;}
inline bool state(const ActorFadeProfile& p,const ActorFadeState& s){
    if(!actor_fade_profile_valid(p)||!s.initialized||s.profile_binding!=p.binding||(s.material_rgba&255u)!=255)return false;
    for(const auto& c:s.channels){if(!std::isfinite(c.time)||c.time<0||c.time>8208)return false;if(c.curve!=255&&!curve(p,c.curve))return false;if(c.active&&(c.curve==255||(c.rgba&255u)!=255))return false;}
    return true;
}
} // namespace actor_fade_detail
inline bool actor_fade_initialize(const ActorFadeProfile& p,ActorFadeState* out){
    if(!out||!actor_fade_profile_valid(p))return false;
    ActorFadeState next;next.initialized=true;next.profile_binding=p.binding;*out=next;return true;
}
inline bool actor_fade_clear(const ActorFadeProfile& p,const ActorFadeState& input,ActorFadeState* out){
    if(!out||!actor_fade_detail::state(p,input))return false;
    auto next=input;for(auto& c:next.channels)c.active=false;next.material_rgba=0x000000ffu;*out=next;return true;
}
inline bool actor_fade_request(const ActorFadeProfile& p,const ActorFadeState& input,u8 id,unsigned channel,ActorFadeState* out){
    if(!out||channel>=2||!actor_fade_detail::state(p,input)||!actor_fade_detail::curve(p,id))return false;
    auto next=input;next.channels[channel]={id,0,0,0x000000ffu,true};*out=next;return true;
}
inline bool actor_fade_update(const ActorFadeProfile& p,const ActorFadeState& input,f32 delta,ActorFadeState* out){
    if(!out||!actor_fade_detail::state(p,input)||!std::isfinite(delta)||delta<0||delta>16)return false;
    auto next=input;
    for(auto& c:next.channels){if(!c.active)continue;const auto* curve=actor_fade_detail::curve(p,c.curve);if(!curve)return false;
        unsigned budget=0;while(c.time>=curve->end){if(++budget>32)return false;
            if(!curve->loop){c.time=curve->end;c.active=false;break;}
            c.loops=u8(unsigned(c.loops)+1u);c.time=f32(f32(c.time-curve->end)+0.f);}
        u32 rgb=0;
        if(c.active){for(unsigned k=0;k<3;++k){const f32 slope=f32(f32(s32(curve->last[k])-s32(curve->first[k]))/curve->end);
                const f32 value=std::fma(c.time,slope,f32(curve->first[k]));if(!std::isfinite(value)||value<-256||value>511)return false;
                rgb=(rgb<<8)|u8(s32(value));}}
        // At stop, sourceprev==next creates0/0; nontrapping fctiwz low bytes=0.
        // Model those resulting RGB bytes explicitly; never cast NaN in C++.
        c.rgba=(rgb<<8)|255u;c.time=f32(c.time+delta);next.material_rgba=c.rgba;
    }
    *out=next;return true;
}
inline bool actor_fade_material(const ActorFadeProfile& p,const ActorFadeState& state,ActorFadeMaterial* out){
    if(!out||!actor_fade_detail::state(p,state))return false;
    *out={{{u8(state.material_rgba>>24),u8(state.material_rgba>>16),u8(state.material_rgba>>8)}},true};return true;
}
} // namespace gf
