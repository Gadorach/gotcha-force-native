#pragma once
#include "engine/assets/story_map_rebuild.hpp"
#include "engine/gameplay/story_event_progress.hpp"
#include "engine/gameplay/story_replay.hpp"
namespace gf {
//Native values for represented source map caches. Unused list tails are retained:
//E138/E34C clear counts/masks, NOT whole lists; even an empty area's first byte
//is read for the resulting selected-event VALUE. That is not event admission.
struct StoryMapRebuildState {
    std::array<std::array<u8,64>,kStoryMapNodes> event_ids{},annotations{};
    std::array<u8,kStoryMapNodes> counts{};
    std::array<u16,kStoryMapNodes> links{};
    u32 type67_areas=0;
    u32 selection_row=0,selection_column=0,selection_count=0,selected_event_id=0;
    bool operator==(const StoryMapRebuildState&)const=default;
};
enum class StoryMapRebuildMode : u8 { Normal,Special };
inline bool story_map_rebuild_plan(const StoryMapRebuildState& before,
                                   const StoryMapRebuildProfile& profile,
                                   const StoryEventProgress& progress,const StoryReplayHistory& history,
                                   const StoryReplayContext& replay,StoryMapRebuildMode mode,u32 location,
                                   StoryMapRebuildState* out,std::string* error=nullptr){
    auto fail=[&](const char* why){if(error)*error=std::string("Story map rebuild: ")+why;return false;};
    if(!out||!story_map_rebuild_profile_valid(profile)||location>=kStoryMapNodes||unsigned(mode)>unsigned(StoryMapRebuildMode::Special))return fail("profile/location/mode/output");
    if(mode==StoryMapRebuildMode::Special&&!replay.valid())return fail("SPECIAL requires explicit current replay");
    auto next=before;next.counts.fill(0);next.links.fill(0);next.type67_areas=0;
    for(unsigned index=0;index<profile.events.size();++index){const unsigned id=index+1;const auto& row=profile.events[index];bool include=false;
        if(mode==StoryMapRebuildMode::Special){if(!story_replay_history_seen(history,replay,id,&include))return fail("history index");}
        else include=(progress.events[id][0]&1)!=0&&(progress.events[id][0]&2)==0;
        if(!include)continue;
        auto& count=next.counts[row.area];if(count>=64)return fail("source64-entry area capacity");
        next.event_ids[row.area][count]=u8(id);next.annotations[row.area][count]=row.annotation;++count;
        if(mode==StoryMapRebuildMode::Normal){
            if(row.type==6||row.type==7)next.type67_areas|=1u<<row.area;
            next.links[row.area]|=row.links;
        }
    }
    //E4F0 is a separate post-event rebuild pass. SPECIAL skips it and does not
    //derive normal event links/type bits from the historical events it lists.
    if(mode==StoryMapRebuildMode::Normal)for(unsigned index=0;index<profile.gates.size();++index){
        const auto& row=profile.gates[index];const auto flags=progress.gates[index+1][0];
        if(row.has_links&&(flags&1)!=0&&(flags&2)==0)next.links[row.area]|=row.links;
    }
    next.selection_row=0;next.selection_column=0;next.selection_count=next.counts[location];next.selected_event_id=next.event_ids[location][0];
    *out=std::move(next);if(error)error->clear();return true;
}
} //namespace gf
