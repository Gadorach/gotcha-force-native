#pragma once
// Default TObj callback8023E0C4, bounded single-stage UV + reviewed colour-lerp
// subset. Original callback clamps blend/TEV bytes before255*f32/truncation.
#include <array>
#include <algorithm>
#include <cmath>
#include <limits>
#include "engine/core/types.hpp"
namespace gf {
struct HsdTextureState {
    std::array<f32,2> translate{},scale{1,1};std::array<u8,2> repeat{1,1};
    f32 blend=1;bool colour_lerp=false;u32 low=0,high=0xffffffffu;
    //Retained third source constant; the reviewed mix(TEV0,KONST,TEXC)
    //expression does not read it. Only the explicit callback entry below admits it.
    u32 tev1=0;
};
inline bool hsd_texture_uv(const std::array<f32,2>& translate,const std::array<f32,2>& scale,
                           const std::array<u8,2>& repeat,std::array<f32,2>* multiplier,std::array<f32,2>* offset){
    if(!multiplier||!offset)return false;
    std::array<f32,2> m{},o{};
    for(unsigned k=0;k<2;++k){
        if(!std::isfinite(scale[k])||!std::isfinite(translate[k])||!repeat[k])return false;
        m[k]=std::abs(scale[k])<std::numeric_limits<f32>::epsilon()?0:repeat[k]/scale[k];o[k]=-translate[k]*m[k];
        if(!std::isfinite(m[k])||!std::isfinite(o[k])||std::abs(m[k])>1e7f||std::abs(o[k])>1e7f)return false;
    }
    *multiplier=m;*offset=o;return true;
}
inline bool hsd_texture_callback(u32 channel,f32 value,HsdTextureState* out){
    if(!out||!std::isfinite(value)||std::abs(value)>1e7f)return false;
    auto next=*out;
    if(channel==2||channel==3)next.translate[channel-2]=value;
    else if(channel==4||channel==5)next.scale[channel-4]=value;
    else if(channel>=6&&channel<=8){if(value!=0)return false;}
    else if(channel==9||channel==24)next.blend=std::clamp(value,0.f,1.f);
    else if(channel>=12&&channel<=19){
        if(!next.colour_lerp)return false; // Explicit reviewed TEV expression required.
        const u32 byte=u32(f32(255.f*std::clamp(value,0.f,1.f)));const unsigned shift=(3-(channel%4))*8;
        auto& colour=channel<16?next.high:next.low;colour=(colour&~(0xffu<<shift))|(byte<<shift);
    }else return false; // Image/palette/LOD/general TEV remain separate consumers.
    *out=next;return true;
}
inline bool hsd_texture_callback_retaining_tev1(u32 channel,f32 value,HsdTextureState* out){
    if(channel<20||channel>23)return hsd_texture_callback(channel,value,out);
    if(!out||!out->colour_lerp||!std::isfinite(value)||std::abs(value)>1e7f)return false;
    const u32 byte=u32(f32(255.f*std::clamp(value,0.f,1.f)));
    const unsigned shift=(23-channel)*8;
    out->tev1=(out->tev1&~(0xffu<<shift))|(byte<<shift);return true;
}
} // namespace gf
