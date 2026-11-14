#pragma once
#include <array>
#include "engine/core/types.hpp"
namespace gf {
//80182040: the end check is BEFORE each weight, not after the last addition.
// Callers bind validated64-weight profiles and samples-per-callback (2 or4).
inline bool weighted_slide_advance(const std::array<f32,64>& weights,unsigned samples,unsigned& index,f32& sum){
    for(unsigned i=0;i<samples;++i){if(index>=weights.size())return true;sum=f32(sum+weights[index++]);}
    return false;
}
//801820C4: single-precision multiply followed by division, not scale*(sum/total).
inline f32 weighted_slide_scale(f32 scale,f32 sum,f32 total){return f32(f32(scale*sum)/total);}
} // namespace gf
