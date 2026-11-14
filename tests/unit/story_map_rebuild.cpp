#include <cstdio>
#include <cstdlib>
#include "tests/unit/story_map_rebuild_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"Story map rebuild: %s\n",why);std::abort();}}}
int main(){using namespace gf;auto p=story_map_rebuild_fixture::profile();auto bytes=story_map_rebuild_fixture::bytes(p);StoryMapRebuildProfile loaded;std::string error;
    check(story_map_rebuild_load({bytes.data(),bytes.size()},123,&loaded,&error)&&loaded.events.size()==4&&loaded.gates.size()==3,"owned native profile");
    StoryEventProgress progress;progress.events[1][0]=1;progress.events[2][0]=3;progress.events[3][0]=1;progress.events[4][0]=1;
    progress.gates[1][0]=1;progress.gates[3][0]=3;
    StoryMapRebuildState before;for(auto& row:before.event_ids)row.fill(238);for(auto& row:before.annotations)row.fill(177);before.counts.fill(77);before.links.fill(0xffff);before.type67_areas=0xffffffff;before.selection_row=3;before.selection_column=4;
    StoryReplayHistory history;StoryReplayContext invalid,play0,play1;check(StoryReplayContext::from_declared_index(0,&play0)&&StoryReplayContext::from_declared_index(1,&play1),"declared current replays");
    StoryMapRebuildState next;
    check(story_map_rebuild_plan(before,loaded,progress,history,invalid,StoryMapRebuildMode::Normal,10,&next,&error),"normal rebuild does not invent replay-mask filtering");
    check(next.counts[10]==2&&next.event_ids[10][0]==1&&next.event_ids[10][1]==4&&next.event_ids[10][2]==238,"source order, two-bit test and retained unused tail");
    check(next.annotations[10][0]==91&&next.annotations[10][1]==94&&next.annotations[10][2]==177,"annotation byte and retained unused tail");
    check(next.links[10]==(0x8000|4|32)&&next.links[3]==16&&next.type67_areas==((1u<<10)|(1u<<3)),"event type6/7 and post-event gate mask pass");
    check(next.selection_row==0&&next.selection_column==0&&next.selection_count==2&&next.selected_event_id==1,"source cursor fields reset after rebuild");
    check(story_map_rebuild_plan(before,loaded,progress,history,invalid,StoryMapRebuildMode::Normal,14,&next,&error)&&next.selection_count==0&&next.selected_event_id==238,"empty source area retains/read old first byte, not an admitted event");
    history.bytes[0]=u8(1u<<2);history.bytes[32]=u8(1u<<3);
    check(story_map_rebuild_plan(before,loaded,progress,history,play0,StoryMapRebuildMode::Special,10,&next,&error)&&next.counts[10]==1&&next.event_ids[10][0]==2,"SPECIAL uses current play history even for flag02-set event");
    check(next.links==std::array<u16,kStoryMapNodes>{}&&next.type67_areas==0,"SPECIAL bypasses normal event/gate link masks");
    check(story_map_rebuild_plan(before,loaded,progress,history,play1,StoryMapRebuildMode::Special,3,&next,&error)&&next.counts[3]==1&&next.event_ids[3][0]==3,"other play history is independent");
    next=before;check(!story_map_rebuild_plan(before,loaded,progress,history,invalid,StoryMapRebuildMode::Special,10,&next,&error)&&next==before,"missing SPECIAL context no partial output");
    check(!story_map_rebuild_plan(before,loaded,progress,history,play0,StoryMapRebuildMode(2),10,&next,&error)&&next==before,"unknown mode refuses");
    check(!story_map_rebuild_plan(before,loaded,progress,history,play0,StoryMapRebuildMode::Normal,15,&next,&error)&&next==before,"invalid node refuses");
    auto large=loaded;large.events.assign(65,{10,6,1,0,1});for(auto& row:progress.events)row[0]=1;
    check(!story_map_rebuild_plan(before,large,progress,history,play0,StoryMapRebuildMode::Normal,10,&next,&error)&&next==before,"65th selected row refuses whole candidate without overwriting next area");
    large.events.resize(64);check(story_map_rebuild_plan(before,large,progress,history,play0,StoryMapRebuildMode::Normal,10,&next,&error)&&next.counts[10]==64&&next.event_ids[10][63]==64,"exact64-area capacity");
    auto alias=before;check(story_map_rebuild_plan(alias,loaded,progress,history,play0,StoryMapRebuildMode::Normal,10,&alias,&error)&&alias.counts[10]==3,"same logical candidate alias allowed");
    auto bad=bytes;bad[24]=15;check(!story_map_rebuild_load({bad.data(),bad.size()},123,&loaded,&error)&&!loaded.loaded,"payload corruption refuses before domain use");
    check(!story_map_rebuild_load({bytes.data(),bytes.size()},124,&loaded,&error),"foreign map identity");
    bad=bytes;bad[4]=1;check(!story_map_rebuild_load({bad.data(),bad.size()},123,&loaded,&error),"schema version");
    bad=bytes;bad.push_back(0);check(!story_map_rebuild_load({bad.data(),bad.size()},123,&loaded,&error),"extra bytes");
    check(!story_map_rebuild_load({nullptr,bytes.size()},123,&loaded,&error),"missing data");
    check(!story_map_rebuild_load({bytes.data(),bytes.size()},123,nullptr,&error),"missing output");
    bad=story_map_rebuild_fixture::bytes(large);check(story_map_rebuild_load({bad.data(),bad.size()},123,&loaded,&error),"larger bounded profile");
    for(unsigned area:{15u,255u}){auto broken=p;broken.events[0].area=u8(area);bad=story_map_rebuild_fixture::bytes(broken);check(!story_map_rebuild_load({bad.data(),bad.size()},123,&loaded,&error),"signed/out-of-range event area");}
    auto broken=p;broken.gates[0].area=15;bad=story_map_rebuild_fixture::bytes(broken);check(!story_map_rebuild_load({bad.data(),bad.size()},123,&loaded,&error),"bad present gate area");
    broken=p;broken.gates[1].links=1;bad=story_map_rebuild_fixture::bytes(broken);check(!story_map_rebuild_load({bad.data(),bad.size()},123,&loaded,&error),"absent gate cannot carry fabricated mask");
    std::printf("Story map cache reconstruction: %u owned checks PASS; not full event predicates/map rendering/checkpoint load\n",checks);
}
