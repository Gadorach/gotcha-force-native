#pragma once
#include <string>
#include "engine/gameplay/menu_navigation/config.hpp"
namespace gf {
// Versioned native navigation content. The archive key is a consistency check,
// not authentication. Original DOL data is exported offline, never read at runtime.
inline bool menu_navigation_load(ByteView file,u32 archive_hash,MenuNavigationConfig* out,std::string* error){
    if(!out){if(error)*error="menu navigation: no output";return false;}
    *out={};
    auto fail=[&](const char* reason){if(error)*error=std::string("menu navigation: ")+reason;return false;};
    if(!file.data||file.size!=52)return fail("invalid content size");
    if(file.be32(0)!=0x47464e56u||file.be32(4)!=1||file.be32(8)!=archive_hash||file.be32(12)!=7)
        return fail("magic/version/archive/count mismatch");
    const u32 limit=file.be32(16),restart=file.be32(20);
    if(limit>=255||restart>limit)return fail("invalid repeat parameters");
    MenuNavigationConfig candidate;candidate.repeat_limit=u8(limit);candidate.repeat_restart=u8(restart);
    for(unsigned node=0;node<7;++node)for(unsigned column=0;column<4;++column)
        candidate.neighbor[node][column]=file.data[24+node*4+column];
    if(!menu_navigation_valid(candidate))return fail("invalid/unreachable navigation graph");
    *out=candidate;return true;
}
} // namespace gf
