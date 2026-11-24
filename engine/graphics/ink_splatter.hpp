#pragma once
#include <array>
#include "engine/core/types.hpp"

namespace gf {
inline constexpr unsigned kInkSplatterSize=32;
// Original authored demo art. Rendering owns the texture, not game state.
inline std::array<u8,kInkSplatterSize*kInkSplatterSize*4> ink_splatter_rgba(){
    std::array<u8,kInkSplatterSize*kInkSplatterSize*4> rgba{};
    constexpr std::array<std::array<int,3>,9> blobs{{
        {15,15,9},{9,13,6},{21,12,6},{23,20,5},{11,22,5},{4,6,2},{25,5,3},{28,23,2},{7,28,2}
    }};
    for(unsigned y=0;y<kInkSplatterSize;++y)for(unsigned x=0;x<kInkSplatterSize;++x){
        int alpha=0;
        for(const auto& blob:blobs){
            const int dx=int(x)-blob[0],dy=int(y)-blob[1],edge=blob[2]*blob[2]-dx*dx-dy*dy;
            const int a=edge<=0?0:edge<=3?128:255;
            if(a>alpha)alpha=a;
        }
        const auto base=4*(y*kInkSplatterSize+x);
        rgba[base]=24;rgba[base+1]=16;rgba[base+2]=36;rgba[base+3]=u8(alpha);
    }
    return rgba;
}
}
