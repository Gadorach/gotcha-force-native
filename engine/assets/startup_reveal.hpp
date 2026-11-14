#pragma once
// Offline-bound title reveal resources/operands; no runtime DOL or archive search.
#include "engine/assets/hsd_record_animation.hpp"
namespace gf {
struct StartupRevealProfile {
    u32 bank_hash=0;u16 bank_records=0;
    std::array<u32,2> actor_model_hashes{};
    std::array<u32,5> roots{};
    f32 start=0,rate=0;bool loaded=false;
};
inline bool startup_reveal_profile_valid(const StartupRevealProfile& p){
    if(!p.loaded||!p.bank_hash||p.bank_records<5||p.bank_records>4096||!p.actor_model_hashes[0]||!p.actor_model_hashes[1]||f32_to_bits(p.start)!=0||f32_to_bits(p.rate)!=0x3f800000u)return false;
    for(unsigned i=0;i<5;++i){if(!p.roots[i]||p.roots[i]>=(64u<<20))return false;for(unsigned j=0;j<i;++j)if(p.roots[i]==p.roots[j])return false;}
    return true;
}
inline bool startup_reveal_load(ByteView file,StartupRevealProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup reveal profile: ")+why;return false;};
    if(!out||!file.data||file.size!=64||file.be32(0)!=0x47465256u||file.be16(4)!=1||file.be32(8)!=64||file.be16(52)!=40||file.be16(54)!=120||file.data[56]!=2||file.data[57]!=10||file.data[58]!=1||file.data[59]!=0x44||file.be32(60))return fail("header/extent/source policy");
    StartupRevealProfile p;p.bank_records=file.be16(6);p.bank_hash=file.be32(12);for(unsigned i=0;i<2;++i)p.actor_model_hashes[i]=file.be32(16+i*4);for(unsigned i=0;i<5;++i)p.roots[i]=file.be32(24+i*4);p.start=file.bef32(44);p.rate=file.bef32(48);p.loaded=true;
    if(!startup_reveal_profile_valid(p))return fail("resource/operand domain");
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
