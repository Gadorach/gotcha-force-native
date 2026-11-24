#pragma once
#include "engine/assets/battle_camera_entry.hpp"
#include "engine/graphics/source_angles.hpp"
#include "engine/graphics/source_intersection.hpp"
#include "engine/graphics/camera_transform.hpp"
namespace gf {
struct BattleFirstCameraInput {
    std::array<f32,3> actor_position{};std::array<f32,14> values88c{};
    f32 offset6d0=0,scale_b4=0;s16 heading72=0;u8 group582=0,layout29=0,split3d=0;
};
struct BattleFirstCameraPose {
    std::array<f32,3> eye{},target{},up{},horizontal318{};
    Mat4 world114,view394;f32 distance348=0,height350=0,scale354=0;s16 actor5b0=0;
};
namespace battle_camera_pose_detail {
inline bool basis(const std::array<f32,3>& eye,const std::array<f32,3>& target,std::array<f32,3>* up_out,Mat4* world,Mat4* view,std::string* error){
    using namespace source_intersection_detail;
    if(!up_out||!world||!view||!valid(eye)||!valid(target))return false;
    auto forward=sub(target,eye);Vec right{forward[2],0,-forward[0]};auto up=cross(forward,right);
    auto normalize=[&](Vec& v){const auto length2=dot(v,v);if(!std::isfinite(length2)||length2<=bits_to_f32(0x358637be))return false;const auto length=f32(std::sqrt(length2));for(auto& x:v)x=f32(x/length);return valid(v);};
    if(!normalize(right)||!normalize(up)||!normalize(forward)){if(error)*error="battle camera basis: degeneracy consumer unsupported";return false;}
    *up_out=up;*world=mat4_identity();
    for(unsigned k=0;k<3;++k){world->at(k,0)=right[k];world->at(k,1)=up[k];world->at(k,2)=forward[k];world->at(k,3)=eye[k];}
    return camera_orientation_view_matrix(eye.data(),target.data(),up.data(),view);
}
}
//Declared numeric adapter; the native view owner supplies real actor observations.
//No free-camera fitting, invented projection, collision update or frame receipt.
inline bool battle_first_camera_pose(const BattleCameraEntryProfile& profile,const BattleFirstCameraInput& in,BattleFirstCameraPose* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("first battle camera pose: ")+why;return false;};
    using namespace source_intersection_detail;
    if(!out||!profile.ready()||!valid(in.actor_position)||in.group582||in.layout29>2||in.split3d>1||!std::isfinite(in.scale_b4)||in.scale_b4<=0||in.scale_b4>1e4f||!std::isfinite(in.offset6d0)||std::abs(in.offset6d0)>1e7f)return fail("fresh group0/current source scalar domain");
    if(!std::isfinite(in.values88c[0])||std::abs(in.values88c[0])>1e4f||!std::isfinite(in.values88c[3])||in.values88c[3]<=0||in.values88c[3]>1e4f)return fail("actual actor camera-table operands");
    BattleFirstCameraPose p;p.scale354=std::fma(.5f,f32(in.scale_b4-1),1.f);p.distance348=f32(4.f*in.values88c[3]);p.height350=in.values88c[0];
    if((in.layout29==1&&!in.split3d)||in.layout29==2){p.distance348=f32(p.distance348*bits_to_f32(0x3fa66666));p.height350=f32(p.height350*bits_to_f32(0x3f99999a));}
    p.target=in.actor_position;p.target[1]=f32(in.offset6d0+f32(in.actor_position[1]+p.height350));
    p.actor5b0=std::bit_cast<s16>(u16(u32(std::bit_cast<u16>(in.heading72))+0x8000u));
    const auto sy=source_angle_sin(p.actor5b0),cy=source_angle_cos(p.actor5b0),sp=source_angle_sin(1536),cp=source_angle_cos(1536);
    p.horizontal318={std::fma(p.distance348,sy,p.target[0]),p.target[1],std::fma(p.distance348,cy,p.target[2])};
    const auto horizontal=f32(p.distance348*cp);
    p.eye={std::fma(sy,horizontal,p.target[0]),std::fma(p.distance348,sp,p.target[1]),std::fma(cy,horizontal,p.target[2])};
    if(!valid(p.eye)||!valid(p.target)||!valid(p.horizontal318))return fail("derived camera positions");
    //Fresh0C314:360=0,35D=0,1DA=0. Neither degeneracy fallback is reached.
    if(!battle_camera_pose_detail::basis(p.eye,p.target,&p.up,&p.world114,&p.view394,error))return fail("shared source view-matrix evaluation");
    *out=p;if(error)error->clear();return true;
}
}
