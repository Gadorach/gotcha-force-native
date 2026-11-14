#pragma once
#include <algorithm>
#include "engine/core/types.hpp"
namespace gf {
//GC AX signed envelope / unsigned mix coefficient arithmetic, depending on the
//caller's explicit gain conversion. Floor-negative shift then signed16 clamp.
inline s16 pcm_scale_q15_clipped(s16 sample,s32 gain){
    const s64 product=s64(sample)*gain;const s64 shifted=product>=0?product/32768:-((-product+32767)/32768);return s16(std::clamp<s64>(shifted,-32768,32767));
}
} //namespace gf
