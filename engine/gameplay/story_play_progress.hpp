#pragma once
//801815DC phase0, reached by the original Story ending flow, not battle-win
//policy. Pure scalar candidate only: caller still owes that lifecycle provenance,
//the save operation and its actual result, cover, and return to title.
#include "engine/gameplay/story_start.hpp"
namespace gf {
struct StoryPlayProgress {
    u32 selected_play=0, completed_plays=0;
    u32 current_unlocks=0, accumulated_unlocks=0;
    bool clear_pending=false;
    bool operator==(const StoryPlayProgress&)const=default;
};
inline bool story_play_completion_plan(const StoryPlayProgress& before,StoryPlayProgress* out){
    if(!out||before.selected_play>7||before.completed_plays>8)return false;
    auto next=before;
    if(next.selected_play<7){
        ++next.selected_play;
        //Deliberately NOT max(completed,next selected). The source increments
        //completed by ONE when the newly selected play exceeds the old value.
        if(next.selected_play>next.completed_plays)++next.completed_plays;
    }else next.completed_plays=8;
    next.clear_pending=true;
    next.accumulated_unlocks|=next.current_unlocks;
    *out=next;return true;
}
} //namespace gf
