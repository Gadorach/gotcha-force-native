#pragma once
// GFAF1: reviewed two-point source fade curves12/14, not HSD AObj tracks.
#include <array>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct ActorFadeCurve {u8 id=255;f32 end=0;std::array<u8,3> first{},last{};bool loop=false;};
struct ActorFadeProfile {std::array<ActorFadeCurve,3> curves{};u32 binding=0;bool loaded=false,spawn_curve=false;unsigned curve_count()const{return spawn_curve?3:2;}};
inline bool actor_fade_profile_valid(const ActorFadeProfile& p){
    if(!p.loaded||p.curves[0].id!=12||p.curves[1].id!=14)return false;
    for(unsigned i=0;i<p.curve_count();++i){const auto& c=p.curves[i];if(!std::isfinite(c.end)||c.end<1||c.end>8192)return false;}
    if(p.spawn_curve){const auto& c=p.curves[2];if(c.id||c.end!=80||c.loop||c.first!=std::array<u8,3>{255,255,255}||c.last!=std::array<u8,3>{})return false;}
    else if(p.curves[2].id!=255)return false;
    return true;
}
inline bool actor_fade_load_impl(ByteView bytes,ActorFadeProfile* out,std::string* error,bool spawn){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("actor fade: ")+why;return false;};
    const unsigned count=spawn?3:2,size=24+28*count;
    if(!out||!bytes.data||bytes.size!=size||bytes.be32(0)!=0x47464146u||bytes.be16(4)!=(spawn?2:1)||bytes.be16(6)!=count||bytes.be32(8)!=size||bytes.be32(12)||bytes.be32(16)||bytes.be32(20))return fail("header/size/time domain");
    ActorFadeProfile next;next.spawn_curve=spawn;next.binding=2166136261u;for(std::size_t i=0;i<bytes.size;++i){next.binding^=bytes.data[i];next.binding*=16777619u;}
    for(unsigned i=0;i<count;++i){const unsigned at=24+i*28;auto& c=next.curves[i];c.id=bytes.data[at];if(bytes.data[at+1]||bytes.be16(at+2)||bytes.be32(at+4)||bytes.be32(at+12)||bytes.data[at+11]!=255||bytes.data[at+23]!=255)return fail("curve reserved/start/alpha domain");
        c.end=bytes.bef32(at+16);const u32 control=bytes.be32(at+24);if(control!=0xff000001u&&control!=0xf0000000u)return fail("unreviewed control/loop semantics");c.loop=control==0xff000001u;
        for(unsigned k=0;k<3;++k){c.first[k]=bytes.data[at+8+k];c.last[k]=bytes.data[at+20+k];}}
    next.loaded=true;if(!actor_fade_profile_valid(next))return fail("curve identity/end domain");*out=next;return true;
}
inline bool actor_fade_load(ByteView bytes,ActorFadeProfile* out,std::string* error){return actor_fade_load_impl(bytes,out,error,false);}
//Separate GFAF2 entry. Legacy metadata still cannot request source curve0.
inline bool actor_fade_load_spawn(ByteView bytes,ActorFadeProfile* out,std::string* error){return actor_fade_load_impl(bytes,out,error,true);}
} // namespace gf
