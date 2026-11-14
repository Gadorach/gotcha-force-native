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
} // namespace gf
