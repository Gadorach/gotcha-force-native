#pragma once
// Bounded affine inverse, using the established cofactor/f32-buffer convention.
// Shared numeric utility, not a source camera/lifetime or hardware-parity token.
#include "engine/graphics/transform.hpp"
namespace gf {
inline bool mat4_affine_bounded(const Mat4& m){
    for(f32 v:m.m)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    return m.at(3,0)==0&&m.at(3,1)==0&&m.at(3,2)==0&&m.at(3,3)==1;
}
inline bool mat4_inverse_affine(const Mat4& m,Mat4* out){
    if(!out||!mat4_affine_bounded(m))return false;
    f64 cof[3][3];for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)cof[r][c]=f64(m.at((r+1)%3,(c+1)%3))*m.at((r+2)%3,(c+2)%3)-f64(m.at((r+1)%3,(c+2)%3))*m.at((r+2)%3,(c+1)%3);
    const f64 det=f64(m.at(0,0))*cof[0][0]+f64(m.at(0,1))*cof[0][1]+f64(m.at(0,2))*cof[0][2];if(!std::isfinite(det)||std::abs(det)<1e-12)return false;
    Mat4 result;for(unsigned r=0;r<3;++r){for(unsigned c=0;c<3;++c)result.at(r,c)=f32(cof[c][r]/det);
        result.at(r,3)=-f32(f32(f32(result.at(r,0)*m.at(0,3))+f32(result.at(r,1)*m.at(1,3)))+f32(result.at(r,2)*m.at(2,3)));}
    if(!mat4_affine_bounded(result))return false;
    *out=result;return true;
}
} // namespace gf
