#pragma once
//Source801820D4/82110 copy two full0x1800-byte tables and their counts.
//Only persistence/copy semantics: no drop formulas, award decision, RNG rewind,
//mission result or genuine save/load authority is supplied here.
#include <array>
#include <span>
#include "engine/core/types.hpp"
namespace gf {
inline constexpr std::size_t kStoryRewardCounterCapacity=1536;
inline constexpr std::size_t kStoryRewardCounterBytes=0x3008;
struct StoryRewardCounters {
    //Keep the source stored/complemented words, including inactive slots. No
    //interpreting arbitrary words as model IDs/variants or host pointers.
    std::array<u32,kStoryRewardCounterCapacity> active{},backup{};
    u32 active_count=0,backup_count=0;
    bool operator==(const StoryRewardCounters&)const=default;
};
inline bool story_reward_counters_valid(const StoryRewardCounters& value){
    return value.active_count<=kStoryRewardCounterCapacity&&value.backup_count<=kStoryRewardCounterCapacity;
}
//Explicit big-endian segment order matches+1518..451Fhex, NOT native struct
//layout or the entire0x4628 runtime block. Full campaign schema remains separate.
inline bool story_reward_counters_decode_declared(ByteView bytes,StoryRewardCounters* out){
    if(!out||!bytes.data||bytes.size!=kStoryRewardCounterBytes)return false;
    const u32 active_count=bytes.be32(0x1800),backup_count=bytes.be32(0x3004);
    if(active_count>kStoryRewardCounterCapacity||backup_count>kStoryRewardCounterCapacity)return false;
    StoryRewardCounters next;next.active_count=active_count;next.backup_count=backup_count;
    for(std::size_t i=0;i<kStoryRewardCounterCapacity;++i){next.active[i]=bytes.be32(i*4);next.backup[i]=bytes.be32(0x1804+i*4);}
    *out=next;return true;
}
inline bool story_reward_counters_encode(const StoryRewardCounters& value,std::span<u8> out){
    if(!story_reward_counters_valid(value)||out.size()!=kStoryRewardCounterBytes)return false;
    for(std::size_t i=0;i<kStoryRewardCounterCapacity;++i){store_be32(out.data()+i*4,value.active[i]);store_be32(out.data()+0x1804+i*4,value.backup[i]);}
    store_be32(out.data()+0x1800,value.active_count);store_be32(out.data()+0x3004,value.backup_count);return true;
}
inline bool story_reward_counters_snapshot_plan(const StoryRewardCounters& before,StoryRewardCounters* out){
    if(!out||!story_reward_counters_valid(before))return false;
    auto next=before;next.backup=before.active;next.backup_count=before.active_count;*out=next;return true;
}
inline bool story_reward_counters_restore_plan(const StoryRewardCounters& before,StoryRewardCounters* out){
    if(!out||!story_reward_counters_valid(before))return false;
    auto next=before;next.active=before.backup;next.active_count=before.backup_count;*out=next;return true;
}
} //namespace gf
