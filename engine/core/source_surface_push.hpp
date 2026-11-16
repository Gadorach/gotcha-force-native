#pragma once
#include <array>
#include "engine/core/source_heading.hpp"
namespace gf {
//Shared bounded host numeric model of677B0's angle and scale/add operations.
//No surface selection, actor, contact, FPSCR or lifetime authority.
inline bool source_surface_angle(const std::array<f32,3>& normal,s16* out){
    for(auto v:normal)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    const f32 horizontal=f32(std::sqrt(f64(f32(f32(normal[0]*normal[0])+f32(normal[2]*normal[2])))));
    return source_heading_s16(horizontal,normal[1],out);
}
inline bool source_surface_push_position(const std::array<f32,3>& position,const std::array<f32,3>& normal,
                                         f32 height,s16 angle,std::array<f32,3>* out){
    if(!out||!std::isfinite(height)||std::abs(height)>1e7f||angle<0||angle>16384||normal[1]<=0)return false;
    for(auto v:position)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    for(auto v:normal)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    f32 cosine=0;source_heading_xz(angle,nullptr,&cosine);
    const f32 distance=f32(f32(height-position[1])*cosine);std::array<f32,3> result{};
    for(unsigned k=0;k<3;++k){const f32 shift=f32(normal[k]*distance);result[k]=f32(position[k]+shift);
        if(!std::isfinite(result[k])||std::abs(result[k])>1e7f)return false;}
    *out=result;return true;
}
}
