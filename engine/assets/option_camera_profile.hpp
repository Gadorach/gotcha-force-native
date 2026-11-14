#pragma once
#include <string>
#include "engine/assets/hsd_camera.hpp"
#include "engine/gameplay/option_transition.hpp"
namespace gf {
struct OptionCameraProfile {
    OptionTransitionConfig timing;
    HsdCameraClip camera;
};
inline bool option_camera_profile_load(ByteView file,ByteView archive,u32 archive_hash,
                                      const Camera& main_camera,OptionCameraProfile* out,std::string* error){
    if(!out){if(error)*error="option camera: missing output";return false;}
    *out={};auto fail=[&](const char* why){if(error)*error=std::string("option camera: ")+why;return false;};
    if(!file.data||file.size!=40||file.be32(0)!=0x47464f50u||file.be32(4)!=1||file.be32(8)!=archive_hash)
        return fail("size/version/source mismatch");
    OptionCameraProfile candidate;
    candidate.timing={file.be32(12),file.be32(16),file.be32(20),file.be32(24)};
    if(!option_transition_valid(candidate.timing))return fail("invalid timing domain");
    if(!hsd_camera_clip_load(archive,file.be32(28),&candidate.camera,error))return false;
    if(candidate.camera.descriptor!=file.be32(32)||candidate.camera.clip!=file.be32(36))return fail("published binding mismatch");
    const u64 samples=u64(candidate.timing.option_frame)-candidate.timing.entry_frame+1;
    const u64 tracks=candidate.camera.animation.base.size()+candidate.camera.animation.eye.size()+candidate.camera.animation.interest.size()+1;
    if(samples*(u64(candidate.timing.option_frame)+1)*tracks>(1u<<22))return fail("camera preflight work budget exceeded");
    Camera sampled=main_camera;
    if(!hsd_camera_sample(candidate.camera,candidate.timing.main_frame,&sampled,error))return false;
    for(unsigned k=0;k<3;++k)if(std::fabs(sampled.eye[k]-main_camera.eye[k])>0.01f||std::fabs(sampled.target[k]-main_camera.target[k])>0.01f)
        return fail("main-camera baseline mismatch");
    if(std::fabs(sampled.fov_y_degrees-main_camera.fov_y_degrees)>0.001f||
       std::fabs(sampled.near_plane-main_camera.near_plane)>0.001f||std::fabs(sampled.far_plane-main_camera.far_plane)>0.01f)
        return fail("main-camera projection mismatch");
    Camera::Basis expected_basis,sampled_basis;
    if(!main_camera.basis(&expected_basis)||!sampled.basis(&sampled_basis))return fail("invalid main basis");
    f32 alignment=0;for(unsigned k=0;k<3;++k)alignment+=expected_basis.up[k]*sampled_basis.up[k];
    if(alignment<0.9999f)return fail("main-camera orientation mismatch");
    for(u32 frame=candidate.timing.entry_frame;frame<=candidate.timing.option_frame;++frame)
        if(!hsd_camera_sample(candidate.camera,frame,&sampled,error))return false;
    *out=std::move(candidate);return true;
}
} // namespace gf
