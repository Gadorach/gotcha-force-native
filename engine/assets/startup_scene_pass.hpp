#pragma once
// Original single-view startup render/light operands; native IDs, not pointers.
#include <array>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct StartupScenePassProfile {
    u32 bank_hash=0;std::array<s32,8> translucent_groups{};std::array<u8,6> passes{};std::array<u8,4> overlay_order{};
    u8 selector=0,scene33=0,scene29=0,sort_enabled=0,forward=0,backward=0;s16 hold=0;
    std::array<u32,3> target_colours{};u32 fog_type=0,fog_colour=0;f32 fog_start=0,fog_end=0;bool loaded=false;
};
inline bool startup_scene_pass_valid(const StartupScenePassProfile& p){
    return p.loaded&&p.bank_hash&&p.translucent_groups==std::array<s32,8>{0,1,2,-1,3,-1,-1,-1}&&p.passes==std::array<u8,6>{1,4,2,2,2,2}&&p.overlay_order==std::array<u8,4>{1,0,2,3}&&p.selector==1&&p.scene33==1&&p.scene29==0&&p.sort_enabled==1&&p.forward&&p.forward<128&&p.backward&&p.backward<128&&p.hold>0&&p.fog_type==2&&std::isfinite(p.fog_start)&&std::isfinite(p.fog_end)&&p.fog_start>0&&p.fog_end>p.fog_start;
}
inline bool startup_scene_pass_load(ByteView b,StartupScenePassProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup scene pass: ")+why;return false;};
    if(!out||!b.data||b.size!=96||b.be32(0)!=0x47465250||b.be16(4)!=1||b.be16(6)!=8||b.be32(8)!=96||b.be16(62))return fail("header/extent/reserved");
    StartupScenePassProfile p;p.loaded=true;p.bank_hash=b.be32(12);for(unsigned i=0;i<8;++i)p.translucent_groups[i]=std::bit_cast<s32>(b.be32(16+i*4));for(unsigned i=0;i<6;++i)p.passes[i]=b.data[48+i];for(unsigned i=0;i<4;++i)p.overlay_order[i]=b.data[54+i];p.selector=b.data[58];p.scene33=b.data[59];p.scene29=b.data[60];p.sort_enabled=b.data[61];p.forward=b.data[64];p.backward=b.data[65];p.hold=std::bit_cast<s16>(b.be16(66));for(unsigned i=0;i<3;++i)p.target_colours[i]=b.be32(68+i*4);p.fog_type=b.be32(80);p.fog_start=b.bef32(84);p.fog_end=b.bef32(88);p.fog_colour=b.be32(92);
    if(!startup_scene_pass_valid(p))return fail("unsupported source policy/operands");
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
