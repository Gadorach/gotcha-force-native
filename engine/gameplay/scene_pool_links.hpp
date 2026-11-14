#pragma once
#include "engine/core/types.hpp"
#include <limits>
#include <string>
namespace gf::scene_pool_links {
//197A0C/197AD8 common typed-index list operations. Payload/hook semantics belong
//to the concrete owner; these links are never guest pointer encodings.
template<class Snapshot> bool take(Snapshot& s,u16* index,const char* bad_list,const char* bad_generation,std::string* error){
    if(s.free_count<=0||s.free_head<0||std::size_t(s.free_head)>=s.records.size()){if(error)*error=bad_list;return false;}
    const auto slot=u16(s.free_head);auto& r=s.records[slot];if(r.allocated09||r.generation==std::numeric_limits<u64>::max()){if(error)*error=bad_generation;return false;}
    --s.free_count;s.free_head=r.link0;r.link0=-1;r.allocated09=1;r.flag0a=0;r.link4=s.live_head;if(s.live_head>=0)s.records[unsigned(s.live_head)].link0=slot;s.live_head=slot;++r.generation;*index=slot;return true;
}
template<class Snapshot> void put(Snapshot& s,u16 index){
    auto& r=s.records[index];if(r.link0>=0)s.records[unsigned(r.link0)].link4=r.link4;else s.live_head=r.link4;if(r.link4>=0)s.records[unsigned(r.link4)].link0=r.link0;r.link0=s.free_head;s.free_head=index;r.allocated09=0;++s.free_count;
}
} // namespace gf::scene_pool_links
