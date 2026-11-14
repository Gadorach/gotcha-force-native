#pragma once
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct StartupRootProfile {
    f32 reset_scale=0,lifetime=0,step=0,angle_span=0,amplitude=0,bias=0,y_base=0,y_factor=0,angle_unit=0;
    bool loaded=false;
};
inline bool startup_root_profile_valid(const StartupRootProfile& p){
    if(!p.loaded)return false;
    for(f32 v:{p.reset_scale,p.lifetime,p.step,p.angle_span,p.amplitude,p.bias,p.y_base,p.y_factor,p.angle_unit})if(!std::isfinite(v))return false;
    return p.reset_scale>0&&p.reset_scale<=1e4f&&p.lifetime>0&&p.lifetime<=8192&&std::floor(p.lifetime)==p.lifetime&&p.step==1&&
           p.angle_span>=0&&p.angle_span<=32767&&std::floor(p.angle_span)==p.angle_span&&p.amplitude>=0&&p.amplitude<=1e4f&&p.bias>0&&p.bias<=1e4f&&p.y_base>0&&p.y_base<=1e4f&&p.y_factor>=0&&p.y_factor<=1e4f&&
           f32_to_bits(p.angle_unit)==0x38c90fdbu;
}
inline bool startup_root_load(ByteView bytes,StartupRootProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup root profile: ")+why;return false;};
    if(!out||!bytes.data||bytes.size!=52||bytes.be32(0)!=0x47465253||bytes.be16(4)!=1||bytes.be16(6)!=9||bytes.be32(8)!=52||bytes.be32(12))return fail("header/size/version");
    StartupRootProfile p;p.reset_scale=bytes.bef32(16);p.lifetime=bytes.bef32(20);p.step=bytes.bef32(24);p.angle_span=bytes.bef32(28);p.amplitude=bytes.bef32(32);p.bias=bytes.bef32(36);p.y_base=bytes.bef32(40);p.y_factor=bytes.bef32(44);p.angle_unit=bytes.bef32(48);p.loaded=true;
    if(!startup_root_profile_valid(p))return fail("unsupported numeric domain");
    *out=p;return true;
}
} // namespace gf
