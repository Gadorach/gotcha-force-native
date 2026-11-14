#pragma once
#include "engine/graphics/camera.hpp"
namespace gf {
// Original8023EED0 normalizes the normal through the view-normal palette;
//8023EBE8 applies the reflection post matrix. Identity TObj subset only.
inline bool view_normal_reflection_uv(const f32 normal[3],const Camera::Basis& basis,f32 out[2]){
    if(!normal||!out)return false;
    f64 length=0;for(unsigned i=0;i<3;++i){if(!std::isfinite(normal[i])||!std::isfinite(basis.right[i])||!std::isfinite(basis.up[i]))return false;length+=f64(normal[i])*normal[i];}
    if(!std::isfinite(length)||length<1e-24)return false;
    length=std::sqrt(length);f64 x=0,y=0;
    for(unsigned i=0;i<3;++i){x+=normal[i]/length*basis.right[i];y+=normal[i]/length*basis.up[i];}
    const f32 u=f32(.5*x+.5),v=f32(-.5*y+.5);
    if(!std::isfinite(u)||!std::isfinite(v))return false;
    out[0]=u;out[1]=v;return true;
}
inline bool view_normal_reflection_uv(const f32 normal[3],const Camera& camera,f32 out[2]){
    Camera::Basis basis;return camera.basis(&basis)&&view_normal_reflection_uv(normal,basis,out);
}
} // namespace gf
