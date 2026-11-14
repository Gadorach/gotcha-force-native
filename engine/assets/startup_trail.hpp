#pragma once
// Evidence-bound trail7E operands/UVs and loose original effect TPL. The runtime
// never reads DOL/AFS/PZZ. FNV binds content consistency, not authentication.
#include "engine/assets/tpl.hpp"
#include <array>
#include <cmath>
namespace gf {
struct StartupTrailProfile {
    u32 atlas_hash=0;u16 width=0,height=0;std::array<std::array<f32,3>,2> endpoints{};
    std::array<std::array<u16,4>,33> uv{};u32 colour=0;f32 alpha=0;bool loaded=false;
};
inline u32 startup_trail_hash(ByteView bytes){u32 h=2166136261u;for(std::size_t i=0;i<bytes.size;++i)h=(h^bytes.data[i])*16777619u;return h;}
inline bool startup_trail_valid(const StartupTrailProfile& p){
    if(!p.loaded||!p.atlas_hash||!p.width||!p.height||p.width>4096||p.height>4096||p.colour!=0xffffffffu||f32_to_bits(p.alpha)!=0x3f000000u)return false;
    if(p.endpoints!=std::array<std::array<f32,3>,2>{std::array<f32,3>{50,0,0},std::array<f32,3>{-50,0,0}})return false;
    for(const auto& uv:p.uv)if(uv[0]>=p.width||uv[2]>=p.width||uv[1]>=p.height||uv[3]>=p.height)return false;
    return true;
}
inline bool startup_trail_load(ByteView bytes,StartupTrailProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup trail profile: ")+why;return false;};
    if(!out||!bytes.data||bytes.size!=360||bytes.be32(0)!=0x4746544cu||bytes.be16(4)!=1||bytes.be16(6)!=360||bytes.be32(12)!=0||bytes.be32(16)!=0)return fail("header/extent/image index/reserved");
    if(bytes.data[24]!=1||bytes.data[25]!=15||bytes.data[26]!=33||bytes.data[27]!=2||bytes.be32(60)!=0||bytes.be32(64)!=0x3f800000u||bytes.be32(68)!=0x40000000u||bytes.be32(72)!=0x40400000u||bytes.data[356]!=0||bytes.data[357]!=0||bytes.data[358]!=2||bytes.data[359]!=2)return fail("source bone/history/UV/orientation/draw/queue operands");
    if(bytes.be32(340)!=1||bytes.be32(344)!=4||bytes.be32(348)!=1||bytes.be32(352)!=0)return fail("source additive blend operands");
    StartupTrailProfile p;p.atlas_hash=bytes.be32(8);p.width=bytes.be16(20);p.height=bytes.be16(22);for(unsigned i=0;i<2;++i)for(unsigned k=0;k<3;++k)p.endpoints[i][k]=bytes.bef32(28+i*12+k*4);p.colour=bytes.be32(52);p.alpha=bytes.bef32(56);for(unsigned i=0;i<33;++i)for(unsigned k=0;k<4;++k)p.uv[i][k]=bytes.be16(76+i*8+k*2);p.loaded=true;if(!startup_trail_valid(p))return fail("content domain");*out=p;if(error)error->clear();return true;
}
} // namespace gf
