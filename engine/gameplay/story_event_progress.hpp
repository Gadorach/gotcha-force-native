#pragma once
//Pointer-free progress slice, not an entire Story checkpoint or event evaluator.
//Source8017F374..F4F4:256 eight-byte records at+A34 and64 two-byte records at+1234.
//Only the represented flag operations are exposed. Other bytes/bits are retained
//verbatim, including counters used by the source prerequisite interpreters.
#include <array>
#include <span>
#include <algorithm>
#include "engine/core/types.hpp"
namespace gf {
struct StoryEventProgress {
    std::array<std::array<u8,8>,256> events{};
    std::array<std::array<u8,2>,64> gates{};
    bool operator==(const StoryEventProgress&)const=default;
};
inline constexpr std::size_t kStoryEventProgressBytes=0x880;
//Exact field-ordered byte segment only. A full0x4628 source dump is deliberately
//not accepted; this is neither a card codec nor a load/run/result receipt.
inline bool story_event_progress_decode_declared(ByteView bytes,StoryEventProgress* out){
    if(!out||!bytes.data||bytes.size!=kStoryEventProgressBytes)return false;
    StoryEventProgress next;std::size_t at=0;
    for(auto& row:next.events)for(auto& byte:row)byte=bytes.data[at++];
    for(auto& row:next.gates)for(auto& byte:row)byte=bytes.data[at++];
    *out=next;return true;
}
inline bool story_event_progress_encode(const StoryEventProgress& value,std::span<u8> out){
    if(out.size()!=kStoryEventProgressBytes)return false;
    std::size_t at=0;
    for(const auto& row:value.events)for(auto byte:row)out[at++]=byte;
    for(const auto& row:value.gates)for(auto byte:row)out[at++]=byte;
    return true;
}
enum class StoryEventFlag : u8 { Flag01=1,Flag02=2 };
inline bool story_event_flag_valid(StoryEventFlag flag){return flag==StoryEventFlag::Flag01||flag==StoryEventFlag::Flag02;}
//Return the source bit value0/1/2, not a booleanized substitute for source==2.
//Invalid source indices read0; this checked API also reports the invalid input.
inline bool story_event_progress_bits(const StoryEventProgress& value,s32 event_id,StoryEventFlag flag,u8* out){
    if(out)*out=0;
    if(!out||event_id<0||event_id>=256||!story_event_flag_valid(flag))return false;
    *out=value.events[unsigned(event_id)][0]&u8(flag);return true;
}
inline bool story_gate_progress_bits(const StoryEventProgress& value,s32 gate_id,StoryEventFlag flag,u8* out){
    if(out)*out=0;
    if(!out||gate_id<0||gate_id>=64||!story_event_flag_valid(flag))return false;
    *out=value.gates[unsigned(gate_id)][0]&u8(flag);return true;
}
enum class StoryEventFlagMutation : u8 { Set01,Set02,Clear02 };
inline bool story_event_progress_flag_plan(const StoryEventProgress& before,s32 event_id,
                                           StoryEventFlagMutation mutation,StoryEventProgress* out){
    if(!out||event_id<0||event_id>=256||unsigned(mutation)>unsigned(StoryEventFlagMutation::Clear02))return false;
    auto next=before;auto& flags=next.events[unsigned(event_id)][0];
    if(mutation==StoryEventFlagMutation::Clear02)flags&=u8(0xfd);
    else flags|=mutation==StoryEventFlagMutation::Set01?u8(1):u8(2);
    *out=next;return true;
}
inline bool story_gate_progress_flag_plan(const StoryEventProgress& before,s32 gate_id,
                                          StoryEventFlag flag,StoryEventProgress* out){
    if(!out||gate_id<0||gate_id>=64||!story_event_flag_valid(flag))return false;
    auto next=before;next.gates[unsigned(gate_id)][0]|=u8(flag);*out=next;return true;
}
} //namespace gf
