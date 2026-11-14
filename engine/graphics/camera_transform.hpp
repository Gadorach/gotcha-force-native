#pragma once
#include "engine/graphics/camera.hpp"
#include "engine/graphics/affine.hpp"
namespace gf {
inline bool camera_basis_view_matrix(const Camera::Basis& b,Mat4* out){
    if(!out)return false;
    Mat4 m;for(unsigned k=0;k<3;++k){m.at(0,k)=b.right[k];m.at(1,k)=b.up[k];m.at(2,k)=-b.forward[k];}
    for(unsigned r=0;r<3;++r)m.at(r,3)=-f32(f32(f32(m.at(r,0)*b.eye[0])+f32(m.at(r,1)*b.eye[1]))+f32(m.at(r,2)*b.eye[2]));
    if(!mat4_affine_bounded(m))return false;
    *out=m;return true;
}
// The same supported eye/target/up basis used by projection, with GX negative-Z
// view convention. No new camera fitting, projection or scene/viewer engine.
inline bool camera_view_matrix(const Camera& camera,Mat4* out){
    Camera::Basis b;if(!out||!camera.basis(&b))return false;
    return camera_basis_view_matrix(b,out);
}
inline bool camera_orientation_view_matrix(const f32 eye[3],const f32 target[3],const f32 up[3],Mat4* out){
    if(!eye||!target||!up||!out)return false;
    Camera camera;for(unsigned i=0;i<3;++i){camera.eye[i]=eye[i];camera.target[i]=target[i];camera.up[i]=up[i];}Camera::Basis basis;
    return camera.orientation(&basis)&&camera_basis_view_matrix(basis,out);
}
} // namespace gf
