#pragma once
#include "engine/graphics/transform.hpp"
namespace gf {
// Original 801CE0DC -> 80045EF4(mode 2) -> 80046158. Caption artwork is authored
// in local X/Z, with local Y facing the eye. Preserve projected world X as the
// horizontal axis. This is distinct from the unselected identifier's Y spin.
inline bool menu_caption_matrix(const f32 position[3],const f32 eye[3],Mat4* out){
    if(!out)return false;
    auto normalize=[](f32 v[3]){
        const f32 length=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
        if(!std::isfinite(length)||length<1e-6f)return false;
        for(unsigned k=0;k<3;++k)v[k]/=length;
        return true;
    };
    f32 b[3]={eye[0]-position[0],eye[1]-position[1],eye[2]-position[2]};
    f32 c[3]={0,-b[2],b[1]}; // cross(world-X, b)
    f32 a[3]={b[1]*c[2]-b[2]*c[1],b[2]*c[0]-b[0]*c[2],b[0]*c[1]-b[1]*c[0]};
    if(!normalize(a)||!normalize(b)||!normalize(c))return false;
    Mat4 matrix;
    for(unsigned k=0;k<3;++k){matrix.at(k,0)=a[k];matrix.at(k,1)=b[k];matrix.at(k,2)=c[k];matrix.at(k,3)=position[k];}
    *out=matrix;return true;
}
} // namespace gf
