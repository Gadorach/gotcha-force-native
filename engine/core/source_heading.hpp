#pragma once
#include <algorithm>
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
//452A0: atan2(x,z)→frsp→float multiplier→W1/GQR5 signed16 scale0 store.
//Shared existing bounded host-libm model, NOT original MSL/FPSCR/GQR timing.
inline bool source_heading_s16(f32 x,f32 z,s16* out){
    if(out)*out=0;
    if(!out||!std::isfinite(x)||!std::isfinite(z)||std::abs(x)>1e7f||std::abs(z)>1e7f)return false;
    const f32 radians=f32(std::atan2(f64(x),f64(z)));const f32 angle=f32(bits_to_f32(0x4622f983u)*radians);
    if(!std::isfinite(angle)||std::abs(angle)>1e7f)return false;
    *out=s16(s32(std::clamp(angle,-32768.f,32767.f)));return true;
}
inline s16 source_heading_wrap(s32 angle){return std::bit_cast<s16>(u16(angle));}
//45204/45238: signed16 angle -> float radians -> MSL sin/cos -> float.
//As with source_heading_s16, host libm is a bounded native model, not MSL parity.
inline void source_heading_xz(s16 angle,f32* x,f32* z){
    const f32 radians=f32(f32(angle)*bits_to_f32(0x38c90fdbu));
    if(x)*x=f32(std::sin(f64(radians)));
    if(z)*z=f32(std::cos(f64(radians)));
}
//46510's three explicit angle-sector domains; no floating rounding at wrap.
inline unsigned source_heading_sector(s16 angle,unsigned mode){
    const u32 bits=std::bit_cast<u16>(angle);
    if(mode==2)return ((bits+0x2000u)&0xffffu)>>14;
    if(mode==1){const s32 magnitude=angle<0?-s32(angle):s32(angle);return unsigned(magnitude+0x1000)>>13;}
    return ((bits+0x1000u)&0xffffu)>>13;
}
} // namespace gf
