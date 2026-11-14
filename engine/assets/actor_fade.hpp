#pragma once
// GFAF1: reviewed two-point source fade curves12/14, not HSD AObj tracks.
#include <array>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct ActorFadeCurve {u8 id=255;f32 end=0;std::array<u8,3> first{},last{};bool loop=false;};
struct ActorFadeProfile {std::array<ActorFadeCurve,2> curves{};u32 binding=0;bool loaded=false;};
inline bool actor_fade_profile_valid(const ActorFadeProfile& p){
    if(!p.loaded||p.curves[0].id!=12||p.curves[1].id!=14)return false;
    for(const auto& c:p.curves)if(!std::isfinite(c.end)||c.end<1||c.end>8192)return false;
    return true;
}
inline bool actor_fade_load(ByteView bytes,ActorFadeProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("actor fade: ")+why;return false;};
    if(!out||!bytes.data||bytes.size!=80||bytes.be32(0)!=0x47464146u||bytes.be16(4)!=1||bytes.be16(6)!=2||bytes.be32(8)!=80||bytes.be32(12)||bytes.be32(16)||bytes.be32(20))return fail("header/size/time domain");
    ActorFadeProfile next;next.binding=2166136261u;for(std::size_t i=0;i<bytes.size;++i){next.binding^=bytes.data[i];next.binding*=16777619u;}
    for(unsigned i=0;i<2;++i){const unsigned at=24+i*28;auto& c=next.curves[i];c.id=bytes.data[at];if(bytes.data[at+1]||bytes.be16(at+2)||bytes.be32(at+4)||bytes.be32(at+12)||bytes.data[at+11]!=255||bytes.data[at+23]!=255)return fail("curve reserved/start/alpha domain");
        c.end=bytes.bef32(at+16);const u32 control=bytes.be32(at+24);if(control!=0xff000001u&&control!=0xf0000000u)return fail("unreviewed control/loop semantics");c.loop=control==0xff000001u;
        for(unsigned k=0;k<3;++k){c.first[k]=bytes.data[at+8+k];c.last[k]=bytes.data[at+20+k];}}
    next.loaded=true;if(!actor_fade_profile_valid(next))return fail("curve identity/end domain");*out=next;return true;
}
} // namespace gf
