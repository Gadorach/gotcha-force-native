#pragma once
// GFBR1: source constructor row + generic update operands, no guest pointers.
#include <array>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct StartupActorRuntimeProfile {u16 model_id=0;u32 model_hash=0;std::array<s16,9> row{};f32 size_threshold=0,reload_multiplier=0;bool loaded=false;};
inline bool startup_actor_runtime_valid(const StartupActorRuntimeProfile& p){
    return p.loaded&&(p.model_id==0x615||p.model_id==10)&&p.row[0]>=0&&std::isfinite(p.size_threshold)&&p.size_threshold>0&&p.size_threshold<=1e4f&&std::isfinite(p.reload_multiplier)&&p.reload_multiplier>=0&&p.reload_multiplier<=100;
}
inline bool startup_actor_runtime_load(ByteView bytes,u32 model_hash,StartupActorRuntimeProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup actor runtime: ")+why;return false;};
    if(!out||!bytes.data||bytes.size!=52||bytes.be32(0)!=0x47464252u||bytes.be16(4)!=1||bytes.be32(8)!=model_hash||bytes.be32(12)!=52||bytes.be16(16)!=18||bytes.be16(18)||bytes.be32(20)||bytes.be16(42))return fail("header/model/extent");
    StartupActorRuntimeProfile p;p.model_id=bytes.be16(6);p.model_hash=model_hash;for(unsigned i=0;i<9;++i)p.row[i]=std::bit_cast<s16>(bytes.be16(24+i*2));p.size_threshold=bytes.bef32(44);p.reload_multiplier=bytes.bef32(48);p.loaded=true;
    if(!startup_actor_runtime_valid(p))return fail("numeric/identity domain");
    *out=p;return true;
}
} // namespace gf
