#pragma once
#include <array>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct OptionOutputProfile {std::array<f32,5> level_gain{};u16 preview_updates=0;};
inline bool option_output_load(ByteView file,u32 widget_archive_hash,OptionOutputProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("option output: ")+why;return false;};
    if(!out||!file.data||file.size!=40||file.be32(0)!=0x47464f53u||file.be32(4)!=1||file.be32(8)!=widget_archive_hash)return fail("size/version/source mismatch");
    OptionOutputProfile next;
    for(unsigned i=0;i<5;++i){next.level_gain[i]=file.bef32(12+i*4);
        if(!std::isfinite(next.level_gain[i])||next.level_gain[i]<0||next.level_gain[i]>1||
           (i&&next.level_gain[i]<next.level_gain[i-1]))return fail("invalid gain curve");}
    const u16 on=file.be16(32);
    if((on&0xe000u)!=0x8000u||!(on&0x1fffu)||(on&0x1fffu)>118||file.be16(34)!=0x4000u||file.be16(36)!=0xffffu||file.be16(38))return fail("unsupported rumble preview");
    next.preview_updates=u16(on&0x1fffu);*out=next;return true;
}
} // namespace gf
