#pragma once
// Source-bound32 published static TL records, including copyright/TM policy.
#include <array>
#include "engine/assets/hsd_record_animation.hpp"
namespace gf {
struct StartupStaticGraphicsEntry {u16 record=0;u8 group=0,kind=0;u32 root=0;};
struct StartupStaticGraphicsProfile {
    u32 bank_hash=0,main_camera_hash=0;u16 bank_records=0;std::array<StartupStaticGraphicsEntry,32> entries{};
    f32 alpha_start=0,alpha_step=0,zero=0;std::array<f32,3> screen_position{};f32 screen_rotation_x=0;bool loaded=false;
};
inline bool startup_static_graphics_profile_valid(const StartupStaticGraphicsProfile& p){
    if(!p.loaded||!p.bank_hash||!p.main_camera_hash||p.bank_records<37||p.bank_records>4096||f32_to_bits(p.alpha_start)!=0x3f800000u||f32_to_bits(p.alpha_step)!=0xbd888889u||f32_to_bits(p.zero)!=0||f32_to_bits(p.screen_rotation_x)!=0x3fc90fdbu)return false;
    for(f32 v:p.screen_position)if(!std::isfinite(v)||std::abs(v)>1e4f)return false;
    for(unsigned i=0;i<32;++i){const auto& e=p.entries[i];const unsigned record=36-i;const u8 kind=record==15?1:record==16?2:0,group=kind?0x44:record<27?0x45:0x15;
        if(e.record!=record||e.record>=p.bank_records||e.kind!=kind||e.group!=group||!e.root||e.root>=(64u<<20))return false;
        for(unsigned j=0;j<i;++j)if(e.root==p.entries[j].root)return false;}
    return true;
}
inline bool startup_static_graphics_load(ByteView f,StartupStaticGraphicsProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup static graphics profile: ")+why;return false;};
    if(!out||!f.data||f.size!=336||f.be32(0)!=0x47465453u||f.be16(4)!=1||f.be16(6)!=32||f.be32(8)!=336||f.be16(18)!=8||f.be32(20)!=80||f.be16(28)!=15||f.be16(30)!=16||f.data[32]!=12||f.data[33]!=1||f.data[34]!=2||f.data[35]!=5||f.data[64]!=0x45||f.data[65]!=0x15||f.data[66]!=0x44||f.data[67]!=27||f.be32(68)||f.be32(72)||f.be32(76))return fail("header/source policy/extent");
    StartupStaticGraphicsProfile p;p.bank_hash=f.be32(12);p.bank_records=f.be16(16);p.main_camera_hash=f.be32(24);p.alpha_start=f.bef32(36);p.alpha_step=f.bef32(40);p.zero=f.bef32(44);for(unsigned k=0;k<3;++k)p.screen_position[k]=f.bef32(48+k*4);p.screen_rotation_x=f.bef32(60);
    for(unsigned i=0;i<32;++i){const unsigned at=80+i*8;p.entries[i]={f.be16(at),f.data[at+2],f.data[at+3],f.be32(at+4)};}p.loaded=true;
    if(!startup_static_graphics_profile_valid(p))return fail("resource/operand/record domain");
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
