#pragma once
// GFFB1 source configuration/operands. Original capture-buffer offset is evidence
// only, never a native pointer/address or permission to access guest memory.
#include <array>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct StartupFeedbackProfile {
    std::array<u32,8> source_words{};u32 mode=0,format=0;f32 alpha=0,alpha_factor=0,zero=0;
    std::array<u16,4> copy{},destination{};bool loaded=false;
};
inline bool startup_feedback_profile_valid(const StartupFeedbackProfile& p){
    if(!p.loaded||(p.mode!=1&&p.mode!=2)||p.format!=6||!std::isfinite(p.alpha)||p.alpha<0||p.alpha>1||f32_to_bits(p.alpha_factor)!=0x437f0000||f32_to_bits(p.zero)!=0||p.source_words[0]!=p.mode||p.source_words[2]!=f32_to_bits(p.alpha)||p.source_words[3]!=p.format)return false;
    if(p.copy[0]||p.copy[1]||!p.copy[2]||!p.copy[3]||p.copy[2]>1024||p.copy[3]>1024||!p.destination[2]||!p.destination[3]||u32(p.destination[0])+p.destination[2]>p.copy[2]||u32(p.destination[1])+p.destination[3]>p.copy[3])return false;
    for(unsigned n=0;n<2;++n)if(p.source_words[4+n]!=(u32(p.copy[n*2])<<16|p.copy[n*2+1])||p.source_words[6+n]!=(u32(p.destination[n*2])<<16|p.destination[n*2+1]))return false;
    return true;
}
inline bool startup_feedback_load(ByteView b,StartupFeedbackProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup feedback profile: ")+why;return false;};
    if(!out||!b.data||b.size!=56||b.be32(0)!=0x47464642||b.be16(4)!=1||b.be16(6)!=8||b.be32(8)!=56||b.be32(12))return fail("header/extent");
    StartupFeedbackProfile p;p.loaded=true;for(unsigned n=0;n<8;++n)p.source_words[n]=b.be32(16+n*4);p.mode=p.source_words[0];p.alpha=b.bef32(24);p.format=p.source_words[3];for(unsigned n=0;n<4;++n){p.copy[n]=b.be16(32+n*2);p.destination[n]=b.be16(40+n*2);}p.alpha_factor=b.bef32(48);p.zero=b.bef32(52);
    if(!startup_feedback_profile_valid(p))return fail("unsupported source mode/format/operands");
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
