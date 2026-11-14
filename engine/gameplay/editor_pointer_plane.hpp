#pragma once
#include <array>
#include "engine/graphics/camera.hpp"
namespace gf {
//The editor's existing ray/Y=0 plane intersection, shared by hit observation and
//the mouse-hand root. No fitted screen offset or extra camera is introduced.
inline bool editor_pointer_plane(const Camera& camera,f32 x,f32 y,std::array<f32,3>* out){
    if(out)*out={};
    Camera::Basis b;if(!out||!camera.basis(&b)||!std::isfinite(x)||!std::isfinite(y))return false;
    const auto nx=(x/camera.viewport_width*2-1)*b.aspect*b.tan_half_fov,ny=(1-y/camera.viewport_height*2)*b.tan_half_fov;
    f32 direction[3];for(unsigned i=0;i<3;++i)direction[i]=b.forward[i]+nx*b.right[i]+ny*b.up[i];
    if(std::abs(direction[1])<1e-6)return false;
    const auto distance=-camera.eye[1]/direction[1];if(distance<=0||distance>camera.far_plane)return false;
    *out={camera.eye[0]+distance*direction[0],0,camera.eye[2]+distance*direction[2]};return true;
}
} //namespace gf
