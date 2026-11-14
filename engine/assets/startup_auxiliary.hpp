#pragma once
#include <array>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct StartupAuxiliaryPart {u8 model=0;s8 bone=0;s16 turn=0,angle_x=0,angle_z=0;std::array<f32,3> position{};f32 size=0;};
struct StartupAuxiliaryEntry {u8 id=0,channel=0;std::array<StartupAuxiliaryPart,2> parts{};};
struct StartupAuxiliaryProfile {u32 model_hash=0,root=0;u16 joints=0,meshes=0,parent_model=0,source_record=0;std::array<StartupAuxiliaryEntry,2> entries{};std::array<std::array<f32,3>,2> pulse{};f32 angle_unit=0;bool loaded=false;};
inline bool startup_auxiliary_profile_valid(const StartupAuxiliaryProfile& p){
    if(!p.loaded||!p.model_hash||!p.root||p.joints<1||p.joints>8||p.meshes<1||p.meshes>16||p.parent_model!=0x615||p.source_record!=51||f32_to_bits(p.angle_unit)!=0x38c90fdb)return false;
    for(unsigned i=0;i<2;++i){const auto& e=p.entries[i];if(e.id!=40+i||e.channel!=i*2)return false;
        for(const auto& part:e.parts){if(part.model!=51||part.bone<-1||part.bone>=32||part.turn<0||!std::isfinite(part.size)||part.size<=0||part.size>16)return false;for(auto v:part.position)if(!std::isfinite(v)||std::abs(v)>1e4f)return false;}
        for(auto v:p.pulse[i])if(!std::isfinite(v)||v<=0||v>16)return false;
    }
    return true;
}
inline bool startup_auxiliary_load(ByteView b,StartupAuxiliaryProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup auxiliary profile: ")+why;return false;};
    if(!out||!b.contains(0,160)||b.size!=160||b.be32(0)!=0x47464155||b.be16(4)!=1||b.be16(6)!=2||b.be32(8)!=160)return fail("header/extent");
    StartupAuxiliaryProfile p;p.loaded=true;p.model_hash=b.be32(12);p.root=b.be32(16);p.joints=b.be16(20);p.meshes=b.be16(22);
    for(unsigned i=0;i<2;++i){const auto at=24+i*52;auto& e=p.entries[i];e.id=b.data[at];e.channel=b.data[at+1];if(b.data[at+2]!=2||b.data[at+3])return fail("fixed inline count/reserved");
        for(unsigned n=0;n<2;++n){const auto q=at+4+n*24;auto& v=e.parts[n];v.model=b.data[q];v.bone=std::bit_cast<s8>(b.data[q+1]);v.turn=std::bit_cast<s16>(b.be16(q+2));v.angle_x=std::bit_cast<s16>(b.be16(q+4));v.angle_z=std::bit_cast<s16>(b.be16(q+6));for(unsigned k=0;k<3;++k)v.position[k]=b.bef32(q+8+k*4);v.size=b.bef32(q+20);}
        for(unsigned k=0;k<3;++k)p.pulse[i][k]=b.bef32(128+i*12+k*4);
    }
    p.angle_unit=b.bef32(152);p.parent_model=b.be16(156);p.source_record=b.be16(158);if(!startup_auxiliary_profile_valid(p))return fail("identity/numeric domain");*out=p;if(error)error->clear();return true;
}
} // namespace gf
