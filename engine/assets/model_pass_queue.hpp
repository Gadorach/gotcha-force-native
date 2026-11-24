#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <string>
namespace gf {
//097B4/09838 global queue operands only. No startup camera/fog/bank policy,
//actor callback, root ownership, frame or renderer admission.
struct ModelPassQueueProfile {std::array<s32,8> translucent_groups{};std::array<u8,6> passes{};bool loaded=false;};
inline bool model_pass_queue_profile_valid(const ModelPassQueueProfile& p){return p.loaded&&p.translucent_groups==std::array<s32,8>{0,1,2,-1,3,-1,-1,-1}&&p.passes==std::array<u8,6>{1,4,2,2,2,2};}
inline bool model_pass_queue_profile_load(ByteView b,ModelPassQueueProfile* out,std::string* error){
    if(out)*out={};
    if(!out||!b.contains(0,56)||b.size!=56||b.be32(0)!=0x47465150||b.be32(4)!=1||b.be32(8)!=56||b.be32(12)||b.be16(54)){if(error)*error="model pass queues: source recipe extent";return false;}
    ModelPassQueueProfile p;p.loaded=true;for(unsigned i=0;i<8;++i)p.translucent_groups[i]=std::bit_cast<s32>(b.be32(16+4*i));for(unsigned i=0;i<6;++i)p.passes[i]=b.data[48+i];
    if(!model_pass_queue_profile_valid(p)){if(error)*error="model pass queues: source group/pass operands";return false;}
    *out=p;if(error)error->clear();return true;
}
}
