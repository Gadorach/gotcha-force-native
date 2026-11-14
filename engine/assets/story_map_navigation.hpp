#pragma once
//Source Story map adjacency: 8017C1C4 / 8035B21C and direction order80352E90.
//A loaded navigation table is not a map model/camera/resource-ready receipt.
#include <array>
#include <bit>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
inline constexpr unsigned kStoryMapNodes=15;
struct StoryMapNavigationProfile {
    u32 map_fingerprint=0;
    std::array<u32,4> directions{};
    std::array<std::array<s8,8>,kStoryMapNodes> neighbors{};
    bool loaded=false;
};
inline bool story_map_navigation_load(ByteView bytes,u32 expected_map_fingerprint,
                                     StoryMapNavigationProfile* out,std::string* error=nullptr){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("Story map navigation: ")+why;return false;};
    if(!out||!bytes.data||bytes.size!=152)return fail("152-byte native profile required");
    if(bytes.be32(0)!=0x4746534eu||bytes.be16(4)!=1||bytes.be16(6)!=152||
       bytes.be32(8)!=expected_map_fingerprint||!expected_map_fingerprint||
       bytes.be16(12)!=kStoryMapNodes||bytes.data[14]!=4||bytes.data[15])return fail("identity/schema/map binding");
    StoryMapNavigationProfile next;next.map_fingerprint=expected_map_fingerprint;
    constexpr std::array<u32,4> source_order{0x40004,0x80008,0x20002,0x10001};
    for(unsigned i=0;i<4;++i){next.directions[i]=bytes.be32(16+i*4);if(next.directions[i]!=source_order[i])return fail("source direction priority");}
    for(unsigned node=0;node<kStoryMapNodes;++node)for(unsigned edge=0;edge<8;++edge){
        const s8 value=std::bit_cast<s8>(bytes.data[32+node*8+edge]);
        if(value < -1 || value>=s8(kStoryMapNodes))return fail("neighbor outside represented graph");
        next.neighbors[node][edge]=value;
    }
    next.loaded=true;*out=next;if(error)error->clear();return true;
}
} //namespace gf
