#pragma once
#include <cmath>
#include <algorithm>
#include "engine/core/backends.hpp"
namespace gf {
inline bool linear_fog_valid(const LinearFog& fog){
    if(!fog.enabled)return fog.start==0&&fog.end==0&&fog.colour==std::array<u8,3>{};
    return std::isfinite(fog.start)&&std::isfinite(fog.end)&&fog.start>=0&&fog.end>fog.start&&fog.end<=1e7f;
}
// Portable linear eye-depth semantics, not GX packed-coefficient/Z quantization.
inline bool linear_fog_factor(const LinearFog& fog,f32 depth,f32* out){
    if(out)*out=0;
    if(!out||!linear_fog_valid(fog)||!std::isfinite(depth)||depth<=0||depth>1e7f)return false;
    if(fog.enabled)*out=std::clamp(f32(f32(depth-fog.start)/f32(fog.end-fog.start)),0.f,1.f);
    return true;
}
} // namespace gf
