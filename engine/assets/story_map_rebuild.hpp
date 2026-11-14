#pragma once
//Source8017E138/E34C/E4F0 map-cache rebuild inputs. Implicit source event/gate
//IDs are row+1. No guest pointers or inferred full event availability rules.
#include <vector>
#include "engine/assets/story_map_navigation.hpp"
namespace gf {
struct StoryMapEventRow {u8 area=0,type=0,annotation=0,replay_flags=0;u16 links=0;};
struct StoryMapGateRow {u8 area=0;bool has_links=false;u16 links=0;};
struct StoryMapRebuildProfile {
    u32 map_fingerprint=0,table_fingerprint=0;
    std::vector<StoryMapEventRow> events;
    std::vector<StoryMapGateRow> gates;
    bool loaded=false;
};
inline bool story_map_rebuild_profile_valid(const StoryMapRebuildProfile& p){
    if(!p.loaded||!p.map_fingerprint||!p.table_fingerprint||p.events.empty()||p.events.size()>255||p.gates.size()>63)return false;
    for(const auto& row:p.events)if(row.area>=kStoryMapNodes)return false;
    for(const auto& row:p.gates)if((row.has_links&&row.area>=kStoryMapNodes)||(!row.has_links&&(row.area||row.links)))return false;
    return true;
}
inline bool story_map_rebuild_load(ByteView bytes,u32 expected_map_fingerprint,
                                   StoryMapRebuildProfile* out,std::string* error=nullptr){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("Story map rebuild: ")+why;return false;};
    if(!out||!bytes.data||bytes.size<24||bytes.size>4096)return fail("bounded native profile required");
    if(bytes.be32(0)!=0x47464d52u||bytes.be16(4)!=1||bytes.be16(6)!=bytes.size||!expected_map_fingerprint||bytes.be32(8)!=expected_map_fingerprint||bytes.be32(20))return fail("schema/map binding");
    const unsigned events=bytes.be16(16),gates=bytes.be16(18);
    if(!events||events>255||gates>63||bytes.size!=24+events*8+gates*4)return fail("table extents");
    u32 hash=2166136261u;for(std::size_t i=24;i<bytes.size;++i)hash=(hash^bytes.data[i])*16777619u;
    if(!hash||hash!=bytes.be32(12))return fail("table fingerprint"); //corruption guard, not authentication
    StoryMapRebuildProfile next;next.map_fingerprint=expected_map_fingerprint;next.table_fingerprint=hash;
    std::size_t at=24;
    for(unsigned i=0;i<events;++i,at+=8){if(bytes.be16(at+6))return fail("event reserved field");next.events.push_back({bytes.data[at],bytes.data[at+1],bytes.data[at+2],bytes.data[at+3],bytes.be16(at+4)});}
    for(unsigned i=0;i<gates;++i,at+=4){if(bytes.data[at+1]>1)return fail("gate presence field");next.gates.push_back({bytes.data[at],bytes.data[at+1]!=0,bytes.be16(at+2)});}
    next.loaded=true;if(!story_map_rebuild_profile_valid(next))return fail("source field domains");
    *out=std::move(next);if(error)error->clear();return true;
}
} //namespace gf
