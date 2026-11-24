#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct BattleSideArchiveSlice {std::size_t offset=0,bytes=0;};
struct BattleSideArchiveLayout {
    BattleSideArchiveSlice placement;std::array<BattleSideArchiveSlice,8> models{};unsigned count=0;
};
//4279C reads an uncompressed sector table: block0 is placement data; following
//blocks are HSD archives for resource slots12..19. This is NOT a PZZ decoder.
inline bool battle_side_archive_layout(ByteView raw,BattleSideArchiveLayout* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("battle side archive: ")+why;return false;};
    if(!out||!raw.data||raw.size<2048||raw.size>(32u<<20)||raw.size%2048)return fail("sector extent/budget");
    const u32 blocks=raw.be32(0);if(!blocks||blocks>9)return fail("placement plus at most eight resource archives required");
    BattleSideArchiveLayout next;std::size_t at=2048;
    for(unsigned i=0;i<blocks;++i){
        const u32 sectors=raw.be32(4+4*i);if(!sectors||sectors>(32u<<20)/2048)return fail("uncompressed nonempty sector count required");
        const std::size_t bytes=std::size_t(sectors)*2048;if(!raw.contains(at,bytes))return fail("block outside retained archive");
        if(!i)next.placement={at,bytes};
        else {const u32 hsd_bytes=raw.be32(at);if(hsd_bytes<32||hsd_bytes>bytes)return fail("HSD extent outside resource sector");next.models[i-1]={at,hsd_bytes};}
        at+=bytes;
    }
    if(at!=raw.size)return fail("unaccounted archive tail");
    next.count=blocks-1;*out=next;if(error)error->clear();return true;
}
} // namespace gf
