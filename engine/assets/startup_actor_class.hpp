#pragma once
// Owned source operands/identity scope for the two reviewed startup classes.
#include <array>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct StartupActorClassProfile {std::array<u16,2> model_ids{};f32 angle_retention=0;bool loaded=false;};
inline bool startup_actor_class_profile_valid(const StartupActorClassProfile& p){
    return p.loaded&&p.model_ids[0]==0x615&&p.model_ids[1]==0x000a&&std::isfinite(p.angle_retention)&&p.angle_retention>=0&&p.angle_retention<=1;
}
inline bool startup_actor_class_load(ByteView bytes,StartupActorClassProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup actor class: ")+why;return false;};
    if(!out||!bytes.data||bytes.size!=24||bytes.be32(0)!=0x47464143u||bytes.be16(4)!=1||bytes.be16(6)!=2||bytes.be32(8)!=24||bytes.be32(12))return fail("header/version/extent");
    StartupActorClassProfile next;next.model_ids={bytes.be16(16),bytes.be16(18)};next.angle_retention=bytes.bef32(20);next.loaded=true;
    if(!startup_actor_class_profile_valid(next))return fail("identity/numeric domain");
    *out=next;return true;
}
} // namespace gf
