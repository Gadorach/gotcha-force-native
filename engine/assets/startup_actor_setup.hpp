#pragma once
// GFCS1 owns original actor-data/class-config/rank operands, no guest pointers.
#include <array>
#include <algorithm>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct StartupActorSetupProfile {
    u16 model_id=0;u32 model_hash=0;
    std::array<u8,432> actor_data{};std::array<u8,192> class_config{};
    std::array<std::array<f32,3>,32> ranks{};
    f32 one=0,size_divisor=0;bool loaded=false;
};
namespace startup_actor_setup_detail {
inline s8 signed_byte(u8 v){return std::bit_cast<s8>(v);}
inline ByteView data(const StartupActorSetupProfile& p){return {p.actor_data.data(),p.actor_data.size()};}
inline ByteView config(const StartupActorSetupProfile& p){return {p.class_config.data(),p.class_config.size()};}
inline bool valid(const StartupActorSetupProfile& p){
    if(!p.loaded||(p.model_id!=0x615&&p.model_id!=10)||p.one!=1||!std::isfinite(p.size_divisor)||p.size_divisor<=0||p.size_divisor>1e7f)return false;
    const int lower=signed_byte(p.actor_data[0xa2]),upper=signed_byte(p.actor_data[0xa1]);
    if(lower<0||upper<lower||upper>=32||p.actor_data[0xa3]!=0)return false;
    for(const auto& rank:p.ranks)for(f32 v:rank)if(!std::isfinite(v)||v<=0||v>1e4f)return false;
    const auto d=data(p),c=config(p);
    if(std::bit_cast<s16>(d.be16(4))<0)return false;
    for(unsigned at:{0x44u,0x48u,0x4cu,0x50u,0x54u})if(!std::isfinite(c.bef32(at))||c.bef32(at)<=0||c.bef32(at)>1e4f)return false;
    for(unsigned at:{0x74u,0x78u,0x7cu})if(!std::isfinite(d.bef32(at))||std::abs(d.bef32(at))>1e7f)return false;
    return true;
}
} // namespace startup_actor_setup_detail
inline bool startup_actor_setup_load(ByteView file,u32 model_hash,StartupActorSetupProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup actor setup: ")+why;return false;};
    if(!out||!file.data||file.size!=1064||file.be32(0)!=0x47464353u||file.be16(4)!=1||file.be32(8)!=model_hash||file.be32(12)!=1064||
       file.be16(16)!=432||file.be16(18)!=192||file.be16(20)!=32||file.be16(22)||file.be32(24)!=48||file.be32(28)!=480||file.be32(32)!=672||file.be32(36)!=1056||file.be32(40)||file.be32(44))return fail("header/model scope/version/extents");
    StartupActorSetupProfile next;next.model_id=file.be16(6);next.model_hash=model_hash;
    std::copy_n(file.data+48,432,next.actor_data.begin());std::copy_n(file.data+480,192,next.class_config.begin());
    for(unsigned i=0;i<32;++i)for(unsigned j=0;j<3;++j)next.ranks[i][j]=file.bef32(672+i*12+j*4);
    next.one=file.bef32(1056);next.size_divisor=file.bef32(1060);next.loaded=true;
    if(!startup_actor_setup_detail::valid(next))return fail("unsupported rank/config/numeric domain");
    *out=next;return true;
}
} // namespace gf
