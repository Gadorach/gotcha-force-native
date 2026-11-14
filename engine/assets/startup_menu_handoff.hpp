#pragma once
// Source-bound camera handoff operands; original addresses/recipes stay offline.
#include "engine/assets/hsd_camera.hpp"
namespace gf {
struct StartupMenuHandoffProfile {
    u32 main_camera_hash=0;
    f32 initial_speed=0,first_frame=0,acceleration=0,speed_limit=0,goal=0;
    bool loaded=false;
};
inline bool startup_menu_handoff_profile_valid(const StartupMenuHandoffProfile& p){
    if(!p.loaded||!p.main_camera_hash)return false;
    for(f32 v:{p.initial_speed,p.first_frame,p.acceleration,p.speed_limit,p.goal})if(!std::isfinite(v))return false;
    return p.initial_speed>=0&&p.initial_speed<=p.speed_limit&&p.acceleration>0&&p.acceleration<=p.speed_limit&&p.speed_limit<=16&&
           p.first_frame>=0&&p.first_frame<=p.goal&&p.goal<=hsd_anim_detail::kMaxFrame;
}
inline bool startup_menu_handoff_load(ByteView file,StartupMenuHandoffProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup menu handoff profile: ")+why;return false;};
    if(!out||!file.data||file.size!=40||file.be32(0)!=0x47464842u||file.be16(4)!=1||file.be16(6)!=5||file.be32(8)!=40||file.be32(16))return fail("header/version/size/reserved");
    StartupMenuHandoffProfile p;p.main_camera_hash=file.be32(12);p.initial_speed=file.bef32(20);p.first_frame=file.bef32(24);p.acceleration=file.bef32(28);p.speed_limit=file.bef32(32);p.goal=file.bef32(36);p.loaded=true;
    if(!startup_menu_handoff_profile_valid(p))return fail("operand/time/budget domain");
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
