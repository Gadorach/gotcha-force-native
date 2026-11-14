#pragma once
// Explicit published camera/clip binding. This does not scan for the first
// plausible descriptor or require geometry/joint clips to be evaluatable.
#include "engine/assets/hsd_anim.hpp"
#include "engine/graphics/camera.hpp"
namespace gf {
struct HsdCameraClip {
    HsdCameraAnim animation;
    u32 descriptor=0,clip=0;
    f32 end_frame=0,roll=0;
    f32 aspect=0,base_end_frame=0,eye_end_frame=0,interest_end_frame=0;
    bool has_base=false,has_eye=false,has_interest=false;
};
inline bool hsd_camera_clip_load(ByteView archive,unsigned clip_index,HsdCameraClip* out,std::string* error){
    using namespace hsd_anim_detail;
    if(!out){if(error)*error="hsd camera: no output";return false;}
    *out={};
    auto fail=[&](const char* why){if(error)*error=std::string("hsd camera: ")+why;return false;};
    hsd_detail::Reader r;if(!open_container(archive,&r,error))return false;
    const std::size_t symbols=32+r.block.size+std::size_t(archive.be32(8))*4;
    const u32 publics=archive.be32(12),externs=archive.be32(16),file_size=archive.be32(0);
    if(publics>4096||externs>4096)return fail("symbol budget exceeded");
    const std::size_t strings=symbols+(std::size_t(publics)+externs)*8;
    u32 scene=0;bool found=false;
    for(u32 i=0;i<publics;++i){
        const u32 name=archive.be32(symbols+i*8+4);
        if(name>=file_size-strings)return fail("invalid symbol name");
        std::size_t end=strings+name;
        while(end<file_size&&archive.data[end]){if(end-strings-name>=256)return fail("symbol name budget exceeded");++end;}
        if(end==file_size)return fail("unterminated symbol name");
        if(std::string(reinterpret_cast<const char*>(archive.data+strings+name),end-strings-name)=="scene_data"){
            if(found)return fail("ambiguous scene export");
            scene=archive.be32(symbols+i*8);found=true;
        }
    }
    u32 record=0,descriptor=0,clips=0;
    if(!found||!r.ptr(std::size_t(scene)+4,&record)||!record||!r.has(record,8)||
       !r.ptr(record,&descriptor)||!descriptor||!r.ptr(record+4,&clips)||!clips)
        return fail("missing published camera binding");
    u32 selected=0;bool terminated=false;
    for(unsigned i=0;i<=256;++i){
        u32 clip=0;if(!r.ptr(std::size_t(clips)+i*4,&clip))return fail("invalid clip array");
        if(!clip){terminated=true;break;}
        if(i==clip_index)selected=clip;
    }
    if(!terminated||!selected)return fail("clip index/termination invalid");
    if(!r.has(descriptor,0x38)||r.block.be16(descriptor+4)!=0||r.block.be16(descriptor+6)!=1)return fail("unsupported/truncated camera flags/projection");
    HsdCameraClip candidate;candidate.descriptor=descriptor;candidate.clip=selected;
    auto& camera=candidate.animation;camera.present=true;
    u32 klass=0,eye=0,interest=0;
    if(!r.ptr(descriptor,&klass)||klass||!r.ptr(descriptor+0x18,&eye)||!r.ptr(descriptor+0x1c,&interest)||!eye||!interest)
        return fail("invalid camera descriptor fields/class");
    auto position=[&](u32 at,f32 xyz[3]){
        u32 cls=0,constraints=0;
        if(!r.has(at,0x14)||!r.ptr(at,&cls)||cls||!r.ptr(at+16,&constraints)||constraints||!r.finite(r.block,at+4,3))return false;
        for(unsigned k=0;k<3;++k)xyz[k]=r.block.bef32(at+4+k*4);
        return true;
    };
    if(!position(eye,camera.static_eye)||!position(interest,camera.static_interest))return fail("unsupported/invalid static WObj");
    candidate.roll=r.block.bef32(descriptor+0x20);
    camera.near_plane=r.block.bef32(descriptor+0x28);camera.far_plane=r.block.bef32(descriptor+0x2c);camera.fov=r.block.bef32(descriptor+0x30);
    const f32 aspect=r.block.bef32(descriptor+0x34);
    candidate.aspect=aspect;
    if(!std::isfinite(candidate.roll)||!std::isfinite(aspect)||aspect<=0||aspect>16)return fail("invalid roll/aspect");
    if(!std::isfinite(camera.near_plane)||!std::isfinite(camera.far_plane)||!std::isfinite(camera.fov)||
       camera.near_plane<=0||camera.far_plane<=camera.near_plane||camera.fov<=0||camera.fov>=179)
        return fail("invalid camera projection values");
    if(!r.has(selected,12))return fail("truncated camera clip");
    u32 base=0,eye_anim=0,interest_anim=0;
    if(!r.ptr(selected,&base)||!r.ptr(selected+4,&eye_anim)||!r.ptr(selected+8,&interest_anim))return fail("unrelocated camera animation fields");
    std::size_t decoded_bytes=0,decoded_tracks=0;
    auto tracks=[&](u32 aobj,std::vector<HsdAnimTrack>& result){
        if(!aobj)return true;
        u32 first=0;
        if(!r.has(aobj,16)||r.block.be32(aobj)!=0||!r.ptr(aobj+8,&first)||!first)return false;
        const f32 end=r.block.bef32(aobj+4);
        if(!std::isfinite(end)||end<=0||end>kMaxFrame)return false;
        std::vector<u32> seen;
        for(u32 node=first;node;){
            if(decoded_tracks>=128||std::find(seen.begin(),seen.end(),node)!=seen.end()||!is_fobjdesc(r,node))return false;
            seen.push_back(node);++decoded_tracks;
            const u32 length=r.block.be32(node+4),data=r.block.be32(node+16);
            if(!length||length>(1u<<20)-decoded_bytes)return false;
            decoded_bytes+=length;
            HsdAnimTrack track;track.channel=r.block.data[node+12];track.frac_value=r.block.data[node+13];track.frac_slope=r.block.data[node+14];
            track.startframe=r.block.bef32(node+8);track.data.assign(r.block.data+data,r.block.data+data+length);result.push_back(std::move(track));
            if(!r.ptr(node,&node))return false;
        }
        candidate.end_frame=std::max(candidate.end_frame,r.block.bef32(aobj+4));
        return true;
    };
    if(!tracks(base,camera.base))return fail("invalid camera base tracks");
    candidate.has_base=base!=0;if(base)candidate.base_end_frame=r.block.bef32(base+4);
    for(const auto& track:camera.base)if(track.channel<9||track.channel>12)return fail("unsupported camera channel");
    auto window=[&](u32 at,std::vector<HsdAnimTrack>& result,HsdAnimSpline& spline){
        if(!at)return true;
        u32 aobj=0,constraints=0;
        if(!r.has(at,8)||!r.ptr(at,&aobj)||!r.ptr(at+4,&constraints)||constraints||!tracks(aobj,result))return false;
        bool path=false;
        for(const auto& track:result){if(track.channel<4||track.channel>7)return false;path=path||track.channel==4;}
        if(path){
            u32 object=0,data=0;
            if(!aobj||!r.ptr(aobj+12,&object)||!object||!r.has(object,0x40)||
               !(r.block.be32(object+4)&0x4000u)||!r.ptr(object+16,&data)||!data||!read_spline(r,data,&spline))return false;
        }
        return true;
    };
    if(!window(eye_anim,camera.eye,camera.eye_spline)||!window(interest_anim,camera.interest,camera.interest_spline))
        return fail("invalid/unsupported window animation or spline");
    if(eye_anim){u32 aobj=0;if(!r.ptr(eye_anim,&aobj))return fail("eye AObj binding");candidate.has_eye=aobj!=0;if(aobj)candidate.eye_end_frame=r.block.bef32(aobj+4);}
    if(interest_anim){u32 aobj=0;if(!r.ptr(interest_anim,&aobj))return fail("interest AObj binding");candidate.has_interest=aobj!=0;if(aobj)candidate.interest_end_frame=r.block.bef32(aobj+4);}
    *out=std::move(candidate);return true;
}
inline bool hsd_camera_sample(const HsdCameraClip& clip,u32 frame,Camera* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("hsd camera: ")+why;return false;};
    if(!out||!clip.animation.present||!std::isfinite(clip.end_frame)||frame>clip.end_frame)return fail("invalid output/clip/frame");
    f32 roll=clip.roll;bool found=false;
    if(frame&&!hsd_anim_detail::sample_channel(clip.animation.base,9,frame,&roll,&found,error))return false;
    if(!std::isfinite(roll)||std::fabs(roll)>1e-6f)return fail("nonzero camera roll unsupported");
    HsdAnimPose pose;
    if(frame){if(!hsd_anim_detail::camera_pose(clip.animation,frame,&pose,error))return false;}
    else{ // No interpreter update means the authored camera, not zero FObj values.
        for(unsigned k=0;k<3;++k){pose.camera_eye[k]=clip.animation.static_eye[k];pose.camera_interest[k]=clip.animation.static_interest[k];}
        pose.camera_fov_y_degrees=clip.animation.fov;pose.camera_near=clip.animation.near_plane;pose.camera_far=clip.animation.far_plane;
    }
    Camera candidate=*out;
    for(unsigned k=0;k<3;++k){candidate.eye[k]=pose.camera_eye[k];candidate.target[k]=pose.camera_interest[k];}
    candidate.up[0]=candidate.up[2]=0;candidate.up[1]=1;
    candidate.fov_y_degrees=pose.camera_fov_y_degrees;candidate.near_plane=pose.camera_near;candidate.far_plane=pose.camera_far;
    if(!candidate.basis(nullptr))return fail("invalid sampled camera");
    *out=candidate;return true;
}
} // namespace gf
