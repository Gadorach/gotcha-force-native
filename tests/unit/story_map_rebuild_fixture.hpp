#pragma once
#include "engine/gameplay/story_map_rebuild.hpp"
namespace story_map_rebuild_fixture {
inline std::vector<gf::u8> bytes(const gf::StoryMapRebuildProfile& p){using namespace gf;
    std::vector<u8> b(24+p.events.size()*8+p.gates.size()*4);store_be32(b.data(),0x47464d52);store_be16(b.data()+4,1);store_be16(b.data()+6,u16(b.size()));store_be32(b.data()+8,p.map_fingerprint);store_be16(b.data()+16,u16(p.events.size()));store_be16(b.data()+18,u16(p.gates.size()));
    std::size_t at=24;for(const auto& row:p.events){b[at]=row.area;b[at+1]=row.type;b[at+2]=row.annotation;b[at+3]=row.replay_flags;store_be16(b.data()+at+4,row.links);at+=8;}
    for(const auto& row:p.gates){b[at]=row.area;b[at+1]=u8(row.has_links);store_be16(b.data()+at+2,row.links);at+=4;}
    u32 hash=2166136261u;for(std::size_t i=24;i<b.size();++i)hash=(hash^b[i])*16777619u;store_be32(b.data()+12,hash);return b;
}
inline gf::StoryMapRebuildProfile profile(){using namespace gf;StoryMapRebuildProfile p;p.map_fingerprint=123;p.table_fingerprint=456;p.loaded=true;
    p.events={{10,6,91,0,4},{10,1,92,255,8},{3,7,93,1,16},{10,6,94,128,32}};
    p.gates={{10,true,0x8000},{0,false,0},{3,true,2}};return p;
}
}
