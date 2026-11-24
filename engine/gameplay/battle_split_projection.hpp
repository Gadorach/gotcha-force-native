#pragma once
#include "engine/assets/battle_split_projection.hpp"
#include "engine/graphics/camera.hpp"
namespace gf {
//Declared numeric adapter only: half is the8CBD4 parity-selected viewport.
//The scene must separately qualify common33/31, viewer aliases, current poses,
//model publication and framebuffer history before acknowledging a battle frame.
inline bool battle_split_projection(const BattleSplitProjectionProfile& profile,unsigned half,Camera* camera,std::string* error){
    if(!profile.ready()||half>1||!camera){if(error)*error="battle split projection: source profile and half0/1 required";return false;}
    Camera next=*camera;next.fov_y_degrees=41;next.projection_aspect=bits_to_f32(0x402aaaab);
    next.near_plane=10;next.far_plane=40000;next.viewport_x=0;next.viewport_y=s32(half)*224;
    next.viewport_width=640;next.viewport_height=224;
    if(!next.basis(nullptr)){if(error)*error="battle split projection: valid independent camera pose required";return false;}
    *camera=next;if(error)error->clear();return true;
}
}
