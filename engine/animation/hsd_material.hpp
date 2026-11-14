#pragma once
//80242408 default MObj scalar callback, material channels1..10 only.
// Source fmuls/fctiwz RGB bytes and fsubs-then-clamp transparency; finite subset.
#include "engine/assets/hsd_anim.hpp"
namespace gf {
inline bool hsd_material_callback(u32 channel,f32 value,HsdAnimPose::Material* out){
    if(!out||channel<1||channel>10||!std::isfinite(value)||std::abs(value)>1e7f)return false;
    auto next=*out;next.present=true;
    if(channel==10)next.alpha=std::clamp(f32(1.f-value),0.f,1.f);
    else{
        const f32 multiplied=f32(255.f*std::clamp(value,0.f,1.f));
        const u8 byte=u8(u32(multiplied));const f32 colour=f32(byte)/255.f;
        if(channel<=3)next.ambient[channel-1]=colour;
        else if(channel<=6)next.diffuse[channel-4]=colour;
        else{next.has_specular=true;next.specular[channel-7]=colour;}
    }
    *out=next;return true;
}
//8024139C converts the stored material alpha with fmuls/fctiwz at submission.
// Keep source float alpha in the player; use this byte only at the draw boundary.
inline bool hsd_material_draw_alpha(f32 alpha,u8* out){
    if(!out||!std::isfinite(alpha)||alpha<0||alpha>1)return false;
    *out=u8(u32(f32(255.f*alpha)));return true;
}
} // namespace gf
