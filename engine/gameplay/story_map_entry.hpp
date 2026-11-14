#pragma once
//Represented sourceBE78/C138 map-entry field/call sequence. Resource/widget/
//actor/fade operations remain pending requests, not construction completion.
#include "engine/gameplay/story_map_rebuild.hpp"
namespace gf {
struct StoryMapEntryFields {
    StoryStartAction action=StoryStartAction::Continue;
    u8 minor=0,phase=0;u32 reason44=0;
    u32 source58=0,source5c=0,context7c=0,view128=0,source12c=0;
    u32 location_mask114=0,location118=10,previous11c=10,stable120=10;
    u32 list_status9cc=0,event_status9f0=0,partner_c4=0,pending_gate9e0=0;
    //Logical replacement of9EC; distinct from cache.selected_event_id (9C8).
    u32 bound_event_row=0;u8 portrait_a0c=0;
    StoryMapRebuildState cache;
    bool operator==(const StoryMapEntryFields&)const=default;
};
enum class StoryMapEntryPending : u8 {
    RequestMapScene0,RequestPartnerResources,RequestCurrentEventResources,
    ConstructCommonWidgets,BindEventRow1,ConstructEventList,
    ConstructEventDetails,RequestActor3,ConstructPartnerWidgets,RevealBlack32
};
struct StoryMapEntryPlan {StoryMapEntryFields fields;std::vector<StoryMapEntryPending> pending;bool cache_rebuilt=false;};
struct StoryMapEntryDependencies {
    //Source reads busy AFTER the phase0 scene/resource requests. The caller must
    //supply that observation, never stale pre-request readiness or a fake ACK.
    bool resources_ready_after_requests=false;
};
inline bool story_map_entry_fields_valid(const StoryMapEntryFields& f){
    return unsigned(f.action)<=2&&f.reason44<=4&&f.location118<kStoryMapNodes&&f.previous11c<kStoryMapNodes&&f.stable120<kStoryMapNodes&&f.bound_event_row<256;
}
inline bool story_map_entry_plan(const StoryMapEntryFields& before,const StoryMapRebuildProfile& profile,
                                 const StoryEventProgress& progress,const StoryReplayHistory& history,
                                 const StoryReplayContext& replay,const StoryMapEntryDependencies& dependencies,
                                 StoryMapEntryPlan* out,std::string* error=nullptr){
    auto fail=[&](const char* why){if(error)*error=std::string("Story map entry: ")+why;return false;};
    if(!out||!story_map_entry_fields_valid(before)||!story_map_rebuild_profile_valid(profile)||before.minor!=0||before.phase>1)return fail("supported entry fields/profile/output");
    StoryMapEntryPlan next;next.fields=before;auto& f=next.fields;
    if(f.phase==0){
        f.phase=1;next.pending.push_back(StoryMapEntryPending::RequestMapScene0);
        if(f.reason44==1||f.reason44==3||f.reason44==4)next.pending.push_back(StoryMapEntryPending::RequestPartnerResources);
        if(f.reason44==3||f.reason44==4)next.pending.push_back(StoryMapEntryPending::RequestCurrentEventResources);
    }
    if(!dependencies.resources_ready_after_requests){*out=std::move(next);if(error)error->clear();return true;}
    next.pending.push_back(StoryMapEntryPending::ConstructCommonWidgets);
    f.event_status9f0=0;f.portrait_a0c=255;f.phase=0;
    switch(f.reason44){
    case 0:f.minor=1;f.view128=0;f.context7c=1;f.source12c=0;f.portrait_a0c=0;break;
    case 1:
        f.minor=5;f.source5c=0;f.view128=2;f.context7c=0;f.source12c=1;f.list_status9cc=2;f.partner_c4=0;
        f.cache.selected_event_id=1;f.bound_event_row=1;next.pending.push_back(StoryMapEntryPending::BindEventRow1);
        if(!story_map_rebuild_plan(f.cache,profile,progress,history,replay,f.action==StoryStartAction::Special?StoryMapRebuildMode::Special:StoryMapRebuildMode::Normal,f.location118,&f.cache,error))return false;
        next.cache_rebuilt=true;next.pending.push_back(StoryMapEntryPending::ConstructEventList);break;
    case 2:f.minor=9;f.view128=2;f.context7c=0;f.source12c=0;break;
    case 3:case 4:
        f.minor=f.reason44==4?7:6;f.view128=2;f.context7c=2;f.source12c=1;f.list_status9cc=2;f.event_status9f0=f.reason44==4?2:0;f.portrait_a0c=0;
        next.pending.push_back(StoryMapEntryPending::ConstructEventList);next.pending.push_back(StoryMapEntryPending::ConstructEventDetails);
        if(f.reason44==4){next.pending.push_back(StoryMapEntryPending::RequestActor3);next.pending.push_back(StoryMapEntryPending::ConstructPartnerWidgets);}break;
    }
    //Source applies the location fallback AFTER optional reason1 cache rebuild.
    //Do not recompute that cache again here or pretend fallback10 was available.
    const u32 mask=f.action==StoryStartAction::Special?0x7fffu:f.location_mask114;
    if(!(mask&(1u<<f.location118)))f.location118=f.previous11c=f.stable120=10;
    f.reason44=0;next.pending.push_back(StoryMapEntryPending::RevealBlack32);
    *out=std::move(next);if(error)error->clear();return true;
}
//C138 runs on the NEXT source callback after Continue entry selected minor1.
//Its resulting minor2/3 is a route value, not successful navigation/dialogue.
inline bool story_map_after_entry_plan(const StoryMapEntryFields& before,const StoryMapRebuildProfile& profile,
                                       const StoryEventProgress& progress,const StoryReplayHistory& history,
                                       const StoryReplayContext& replay,StoryMapEntryPlan* out,std::string* error=nullptr){
    auto fail=[&](const char* why){if(error)*error=std::string("Story map after entry: ")+why;return false;};
    if(!out||!story_map_entry_fields_valid(before)||!story_map_rebuild_profile_valid(profile)||before.minor!=1||before.pending_gate9e0>profile.gates.size())return fail("supported fields/profile/gate identity/output");
    StoryMapEntryPlan next;next.fields=before;auto& f=next.fields;
    if(f.phase==0){
        if(!story_map_rebuild_plan(f.cache,profile,progress,history,replay,f.action==StoryStartAction::Special?StoryMapRebuildMode::Special:StoryMapRebuildMode::Normal,f.location118,&f.cache,error))return false;
        next.cache_rebuilt=true;f.phase=0;
        if(f.pending_gate9e0==0){f.minor=2;f.source5c=f.source58;f.context7c=1;}else f.minor=3;
    }
    *out=std::move(next);if(error)error->clear();return true;
}
} //namespace gf
