#pragma once
#include "engine/assets/character_preview.hpp"
#include "engine/gameplay/startup_actor_setup.hpp"
#include "engine/graphics/camera.hpp"
#include "engine/gameplay/force_spec_navigation.hpp"
namespace gf {
struct CharacterPreviewSetup {StartupActorInitialSetup actor;std::array<f32,3> position{};};
inline bool character_preview_setup(const CharacterPreviewProfile& p,const StartupActorSetupProfile& actor,CharacterPreviewSetup* out,std::string* error){
    if(out)*out={};
    if(!out||!character_preview_profile_valid(p)||std::find(p.models.begin(),p.models.end(),actor.model_id)==p.models.end())return false;
    CharacterPreviewSetup next;if(!startup_actor_initial_setup(actor,{},&next.actor,error)||next.actor.field7fc<=0)return false;
    //58244 setsB4=1/7FC.583CC has no extraB4 override for0615/000A;
    // its group0/group6 base Y is15 for these exact identities.
    const f32 scale=f32(p.one/next.actor.field7fc);if(!std::isfinite(scale)||scale<=0||scale>1e4f||!startup_actor_dimensions(actor,scale,&next.actor))return false;
    next.actor.actor_scale.fill(scale);next.position=p.position;*out=next;return true;
}
inline bool character_preview_camera(const CharacterPreviewProfile& p,f32 height,ForceSpecAngles controls,Camera* out,s8* source_up=nullptr){
    if(!out||!character_preview_profile_valid(p)||!std::isfinite(height)||std::abs(height)>1e7f)return false;
    const auto phase=u16(controls.pitch);const f32 angle=std::bit_cast<s16>(phase)*p.angle_unit;const f32 sine=std::sin(angle),cosine=std::cos(angle);const s8 up=phase>=0x4000&&phase<=0xc000?-1:1;
    Camera camera;camera.eye[0]=camera.target[0]=0;camera.eye[1]=std::fma(p.radius,sine,height);camera.eye[2]=p.radius*cosine;camera.target[1]=height;camera.target[2]=0;camera.near_plane=p.near_plane;camera.far_plane=p.far_plane;camera.fov_y_degrees=p.fov;camera.viewport_width=640;camera.viewport_height=480;
    // Original LookAt normalizes the nonzero cross product even near a pole.
    // This restricted X=0 orbit supplies its already-orthogonal up vector to
    // Camera, preserving orientation without relaxing the generic basis guard.
    const f32 fy=camera.target[1]-camera.eye[1],fz=-camera.eye[2];const f32 length=std::hypot(fy,fz);const f32 cross=-fz*up;if(!std::isfinite(length)||length<=0||cross==0)return false;const f32 sign=cross>0?1.f:-1.f;
    camera.up[0]=0;camera.up[1]=-sign*fz/length;camera.up[2]=sign*fy/length;if(!camera.basis(nullptr))return false;
    *out=camera;if(source_up)*source_up=up;return true;
}
} // namespace gf
