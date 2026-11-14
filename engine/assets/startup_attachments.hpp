#pragma once
// GFAT1: bounded, model-bank-bound operands for the four reviewed bank children.
// Not C74EC shared efct resources, allocator capacity or actor scene admission.
#include <array>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct StartupAttachmentMode {u8 bone=0;std::array<s16,3> angles{};std::array<f32,3> position{};};
struct StartupAttachmentEntry {u8 id=0,flags=0,grow_mode=0;u16 record=0;std::array<StartupAttachmentMode,3> modes{};};
struct StartupAttachmentProfile {
    u32 model_hash=0;u16 model_id=0;u8 kind=0,mode_count=0;f32 one=0,zero=0,growth=0;
    std::array<StartupAttachmentEntry,2> entries{};bool loaded=false;
};
inline bool startup_attachment_profile_valid(const StartupAttachmentProfile& p){
    if(!p.loaded||!p.model_hash||p.kind>1||p.model_id!=(p.kind?0x615:10)||p.mode_count!=(p.kind?1:3)||p.one!=1||f32_to_bits(p.zero)!=0||!std::isfinite(p.growth)||p.growth<=0||p.growth>1)return false;
    for(unsigned i=0;i<2;++i){const auto& e=p.entries[i];
        if(e.id!=(p.kind?24+i:6+i)||e.record>=4096||e.grow_mode!=(p.kind?i+1:0)||(e.flags&0xe1u)!=0x41u||(e.flags&16u)||(!p.kind&&e.flags!=0x4b))return false;
        for(unsigned mode=0;mode<p.mode_count;++mode){const auto& m=e.modes[mode];if(m.bone>=32)return false;
            for(auto x:m.position)if(!std::isfinite(x)||std::abs(x)>1e4f)return false;
        }
        if(p.kind){const auto& a=e.modes[0].angles;const u8 flags=u8(0x41u|(a[0]?2u:0u)|(a[1]?4u:0u)|(a[2]?8u:0u));if(e.flags!=flags)return false;}
    }
    return p.entries[0].record!=p.entries[1].record;
}
inline bool startup_attachments_load(ByteView bytes,u32 model_hash,StartupAttachmentProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup attachments: ")+why;return false;};
    if(!out||!bytes.contains(0,36)||bytes.size>4096||bytes.be32(0)!=0x47464154||bytes.be16(4)!=1||bytes.be16(6)!=2||bytes.be32(8)!=bytes.size||bytes.be32(12)!=model_hash||bytes.be32(20))return fail("header/model binding");
    StartupAttachmentProfile p;p.loaded=true;p.model_hash=model_hash;p.model_id=bytes.be16(16);p.kind=bytes.data[18];p.mode_count=bytes.data[19];p.one=bytes.bef32(24);p.zero=bytes.bef32(28);p.growth=bytes.bef32(32);
    if(p.mode_count<1||p.mode_count>3||bytes.size!=36+2*(8+24*std::size_t(p.mode_count)))return fail("mode count/extent");
    for(unsigned i=0;i<2;++i){auto& e=p.entries[i];const auto at=36+i*(8+24*p.mode_count);e.id=bytes.data[at];e.flags=bytes.data[at+1];e.record=bytes.be16(at+2);e.grow_mode=bytes.data[at+4];
        if(bytes.data[at+5]||bytes.be16(at+6))return fail("reserved entry fields");
        for(unsigned j=0;j<p.mode_count;++j){auto& m=e.modes[j];const auto pos=at+8+j*24;m.bone=bytes.data[pos];
            if(bytes.data[pos+1]||bytes.be32(pos+20))return fail("reserved mode fields");
            for(unsigned k=0;k<3;++k){m.angles[k]=std::bit_cast<s16>(bytes.be16(pos+2+k*2));m.position[k]=bytes.bef32(pos+8+k*4);}
        }
    }
    if(!startup_attachment_profile_valid(p))return fail("identity/operand domain");
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
