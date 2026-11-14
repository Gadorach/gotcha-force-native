#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/story_map_entry.hpp"
#include "tests/unit/story_map_rebuild_fixture.hpp"
namespace {unsigned checks=0;void check(bool x){++checks;if(!x)std::abort();}}
int main(){using namespace gf;const auto profile=story_map_rebuild_fixture::profile();StoryEventProgress progress;progress.events[1][0]=progress.events[3][0]=1;StoryReplayHistory history;StoryReplayContext replay;check(StoryReplayContext::from_declared_index(2,&replay));
    StoryMapEntryFields before;before.source58=17;before.source5c=23;before.context7c=2;before.view128=2;before.source12c=1;before.location118=3;before.previous11c=1;before.stable120=2;before.location_mask114=1u<<3;before.bound_event_row=2;before.cache.selected_event_id=4;
    for(auto& row:before.cache.event_ids)row.fill(238);
    for(auto& row:before.cache.annotations)row.fill(177);
    before.cache.counts.fill(11);before.cache.links.fill(0xffff);
    StoryMapEntryPlan loading;std::string error;check(story_map_entry_plan(before,profile,progress,history,replay,{},&loading,&error));check(loading.fields.phase==1&&loading.fields.minor==0&&!loading.cache_rebuilt&&loading.pending==std::vector<StoryMapEntryPending>{StoryMapEntryPending::RequestMapScene0});
    StoryMapEntryPlan waiting;check(story_map_entry_plan(loading.fields,profile,progress,history,replay,{},&waiting,&error)&&waiting.fields==loading.fields&&waiting.pending.empty());
    StoryMapEntryPlan ready;check(story_map_entry_plan(waiting.fields,profile,progress,history,replay,{true},&ready,&error));check(ready.fields.minor==1&&ready.fields.phase==0&&ready.fields.cache==before.cache&&!ready.cache_rebuilt);check(ready.pending==std::vector<StoryMapEntryPending>{StoryMapEntryPending::ConstructCommonWidgets,StoryMapEntryPending::RevealBlack32});
    check(ready.fields.view128==0&&ready.fields.context7c==1&&ready.fields.source12c==0&&ready.fields.portrait_a0c==0&&ready.fields.event_status9f0==0);
    StoryMapEntryPlan after;check(story_map_after_entry_plan(ready.fields,profile,progress,history,replay,&after,&error));check(after.cache_rebuilt&&after.fields.minor==2&&after.fields.source5c==17&&after.fields.cache.selected_event_id==3&&after.fields.bound_event_row==2&&after.pending.empty());
    auto gate=ready.fields;gate.pending_gate9e0=3;check(story_map_after_entry_plan(gate,profile,progress,history,replay,&after,&error)&&after.fields.minor==3&&after.fields.source5c==gate.source5c);
    gate.phase=1;check(story_map_after_entry_plan(gate,profile,progress,history,replay,&after,&error)&&after.fields==gate&&!after.cache_rebuilt);
    for(unsigned reason=0;reason<5;++reason){auto input=before;input.reason44=reason;input.location_mask114=0;StoryMapEntryPlan plan;check(story_map_entry_plan(input,profile,progress,history,replay,{true},&plan,&error));
        const unsigned expected_minor[5]={1,5,9,6,7};check(plan.fields.minor==expected_minor[reason]&&plan.fields.reason44==0&&plan.fields.location118==10&&plan.fields.previous11c==10&&plan.fields.stable120==10);
        check(plan.cache_rebuilt==(reason==1));check(plan.pending.back()==StoryMapEntryPending::RevealBlack32);
        if(reason==1)check(plan.fields.bound_event_row==1&&plan.fields.cache.selected_event_id==3&&plan.fields.cache.selection_count==1); //cache built BEFORE location fallback, bound row separate
        if(reason==2)check(plan.fields.portrait_a0c==255&&plan.fields.list_status9cc==before.list_status9cc);
        if(reason==4)check(plan.fields.event_status9f0==2&&plan.pending[plan.pending.size()-3]==StoryMapEntryPending::RequestActor3&&plan.pending[plan.pending.size()-2]==StoryMapEntryPending::ConstructPartnerWidgets);
    }
    auto special=before;special.action=StoryStartAction::Special;special.location_mask114=0;check(story_map_entry_plan(special,profile,progress,history,replay,{true},&ready,&error)&&ready.fields.location118==3&&ready.fields.previous11c==1&&ready.fields.stable120==2);
    auto invalid=before;invalid.reason44=5;auto untouched=ready;check(!story_map_entry_plan(invalid,profile,progress,history,replay,{true},&ready,&error)&&ready.fields==untouched.fields&&ready.pending==untouched.pending);
    invalid=before;invalid.location118=15;check(!story_map_entry_plan(invalid,profile,progress,history,replay,{true},&ready,&error));invalid=before;invalid.phase=2;check(!story_map_entry_plan(invalid,profile,progress,history,replay,{true},&ready,&error));
    gate=before;gate.minor=1;gate.pending_gate9e0=4;check(!story_map_after_entry_plan(gate,profile,progress,history,replay,&ready,&error));
    std::printf("Story Continue/map entry fields: %u owned checks PASS; construction/fade/event binding requests unfulfilled\n",checks);
}
