#pragma once
// GFAA1 operands for80007C54/80008F18 +80007CAC/800091E4.
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct AttachmentAlternateProfile {std::array<u32,8> words{};f32 alpha=0;bool loaded=false;};
inline bool attachment_alternate_profile_valid(const AttachmentAlternateProfile& p){
    return p.loaded&&p.words==std::array<u32,8>{0x20080000,0x50140000,4,10,0x60002001,0x400a,0x20,7}&&f32_to_bits(p.alpha)==0x3ecccccdu;
}
inline bool attachment_alternate_load(ByteView bytes,AttachmentAlternateProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("attachment alternate profile: ")+why;return false;};
    if(!out||!bytes.contains(0,52)||bytes.size!=52||bytes.be32(0)!=0x47464141||bytes.be16(4)!=1||bytes.be16(6)!=9||bytes.be32(8)!=52||bytes.be32(12))return fail("header/extent");
    AttachmentAlternateProfile p;p.loaded=true;for(unsigned i=0;i<8;++i)p.words[i]=bytes.be32(16+i*4);p.alpha=bytes.bef32(48);
    if(!attachment_alternate_profile_valid(p))return fail("unreviewed source operands");
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
