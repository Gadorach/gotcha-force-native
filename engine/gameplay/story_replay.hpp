#pragma once
//Current-run replay context, not the completed-play menu availability counter.
//8017E5A0 tests event-header+7 against 1 << Story+64; F524/F560 use
//that same current index to address the per-play event-history row.
#include <array>
#include <algorithm>
#include "engine/gameplay/story_start.hpp"
namespace gf {
class StoryReplayContext {
public:
    StoryReplayContext()=default;
    static bool from_declared_index(u32 index,StoryReplayContext* out){
        if(out)*out={};
        if(!out||index>=8)return false;
        out->index_=u8(index);out->valid_=true;return true;
    }
    bool valid()const{return valid_;}
    u8 index()const{return index_;}
    u8 event_flag()const{return valid_?u8(1u<<index_):0;}
    u8 displayed_play()const{return valid_?u8(index_+1):0;}
    bool operator==(const StoryReplayContext&)const=default;
private:
    u8 index_=0;bool valid_=false;
};
//Value conversion ONLY: this does not turn a declared menu/dispatch plan into
//a genuine running-campaign, loaded-save, mission-result or RNG receipt.
inline bool story_replay_context_for_start_candidate(const StoryStartDispatchPlan& start,
                                                     StoryReplayContext* out){
    if(out)*out={};
    if(!out||!story_start_facts_valid(start.facts))return false;
    return StoryReplayContext::from_declared_index(start.selected_play,out);
}
inline bool story_event_replay_mask_matches(const StoryReplayContext& replay,u8 event_flags,bool* out){
    if(out)*out=false;
    if(!out||!replay.valid())return false;
    *out=(event_flags&replay.event_flag())!=0;return true;
}
//Known pointer-free persistent slice: Story+4524 through+4623, eight32-byte
//rows. Source event IDs are bit positions; input remains u32 to reject rather
//than silently truncate IDs outside this storage. Event catalog/lifecycle
//qualification remains the caller's responsibility, NOT granted by a bit write.
struct StoryReplayHistory {
    std::array<u8,256> bytes{};
    bool operator==(const StoryReplayHistory&)const=default;
};
inline bool story_replay_history_decode(ByteView bytes,StoryReplayHistory* out){
    if(!out||!bytes.data||bytes.size!=256)return false;
    StoryReplayHistory next;std::copy_n(bytes.data,256,next.bytes.begin());*out=next;return true;
}
inline bool story_replay_history_seen(const StoryReplayHistory& history,const StoryReplayContext& replay,
                                      u32 event_id,bool* out){
    if(out)*out=false;
    if(!out||!replay.valid()||event_id>=256)return false;
    *out=(history.bytes[unsigned(replay.index())*32+event_id/8]&(1u<<(event_id&7)))!=0;return true;
}
inline bool story_replay_history_mark_plan(const StoryReplayHistory& before,const StoryReplayContext& replay,
                                           u32 event_id,StoryReplayHistory* out){
    if(!out||!replay.valid()||event_id>=256)return false;
    auto next=before;next.bytes[unsigned(replay.index())*32+event_id/8]|=u8(1u<<(event_id&7));
    *out=next;return true;
}
} //namespace gf
