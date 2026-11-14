#pragma once
//Represented field-reset slice of8017B8C0. This aggregates already-qualified
//pointer-free values, NOT the whole0x4628 runtime/checkpoint or a run owner.
#include "engine/gameplay/story_event_progress.hpp"
#include "engine/gameplay/story_reward_counters.hpp"
#include "engine/gameplay/story_statistics.hpp"
#include "engine/gameplay/story_replay.hpp"
namespace gf {
struct StoryNewRunFields {
    StoryStartFacts start{};StoryReplayContext replay;
    StoryEventProgress events;StoryRewardCounters rewards;StoryStatistics statistics;StoryReplayHistory history;
    std::array<u8,16> partner_tiers94{},map_states130{},map_parameters140{};
    //Unresolved original progress bytes are retained, never dereferenced or
    //declared padding. No entire native object is serialized by this component.
    std::array<u8,384> unclassified12b4{};
    u32 resume_marker20=0,source38=0,source44=0,source50=0,matches6c=0,weighted70=0;
    u32 source74=0,source78=0,partner8c=0,unlock90=0,location_mask114=0;
    u32 location118=0,previous11c=0,stable120=0,cycle124=0,accumulated4624=0;
    bool operator==(const StoryNewRunFields&)const=default;
};
struct StoryNewRunInventoryObservation {bool known=false,has_model0629=false;};
enum class StoryNewRunPending : u8 {
    ClearCollectionByte0,ClearBoxByte2174,InitializeEventPrerequisites,
    InitializeGatesWithoutAdvance,ResetScreenFade
};
struct StoryNewRunFieldPlan {
    StoryNewRunFields fields;
    std::array<StoryNewRunPending,2> external_prefix{StoryNewRunPending::ClearCollectionByte0,StoryNewRunPending::ClearBoxByte2174};
    std::array<StoryNewRunPending,3> external_suffix{StoryNewRunPending::InitializeEventPrerequisites,StoryNewRunPending::InitializeGatesWithoutAdvance,StoryNewRunPending::ResetScreenFade};
    //Source minor/phase assignment is requested AFTER initialization calls.
    //Nothing here supplies their completion or allows the introduction to run.
    u8 requested_minor=1,requested_phase=0;
};
inline bool story_new_run_field_reset_plan(const StoryNewRunFields& before,
                                          const StoryNewRunInventoryObservation& inventory,
                                          StoryNewRunFieldPlan* out){
    if(!out||!inventory.known||!story_start_facts_valid(before.start)||!before.replay.valid()||!story_reward_counters_valid(before.rewards))return false;
    StoryNewRunFieldPlan next;next.fields=before;auto& f=next.fields;
    f.source78=inventory.has_model0629?500u:300u;f.source74=f.source78;
    f.location118=f.previous11c=f.stable120=10;f.cycle124=0;f.location_mask114=0x400;
    f.matches6c=0;f.unlock90=0;f.partner8c=1;f.source50=0;f.source38=0;
    f.partner_tiers94.fill(0);f.events={};f.map_states130.fill(0);
    if(!story_statistics_new_run_clear_plan(&f.statistics))return false;
    f.source44=1;
    //No clear of map_parameters140, weighted70, saved/current/completed play,
    //history, reward tables, accumulated unlocks, marker or unresolved bytes.
    //Prerequisite initialization can subsequently change event/gate records;
    //their zero state here is pending, not a fully initialized campaign.
    *out=std::move(next);return true;
}
} //namespace gf
