#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include "engine/gameplay/story_force_editor_bridge.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
#include "tests/unit/story_map_rebuild_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"Story force edges: %s\n",why);std::abort();}}
static_assert(!std::is_copy_constructible_v<StoryForceSelection>&&!std::is_copy_constructible_v<StoryForceEditorBridge>);
const auto profile=story_map_rebuild_fixture::profile();
StoryForceSetupFacts facts(){StoryForceSetupFacts f;f.active_event_row=3;f.source50=1;f.source78=175;check(StoryReplayContext::from_declared_index(7,&f.replay),"replay7");return f;}
StoryForceEvent tick(StoryForceSelection& page,const ForceInventory& owner,u32 raw=0,bool fade=false){std::array<SourceSetupPad,4> pads{};pads[0]={raw,true};StoryForceEvent e;check(page.step(owner,pads,fade,&e),"callback");return e;}
void active(StoryForceSelection& page,const ForceInventory& owner){for(unsigned i=0;i<5;++i)tick(page,owner);}
void finish(StoryForceSelection& page,const ForceInventory& owner){tick(page,owner,0x100);tick(page,owner);for(unsigned i=0;i<33;++i)tick(page,owner);tick(page,owner,0,true);check(tick(page,owner).destination==StoryForceDestination::Stored,"stored destination");}
StoryForceEditorRequest edit(StoryForceSelection& page,const ForceInventory& owner){tick(page,owner,0x400);check(tick(page,owner).request_cover==30,"editor cover");tick(page,owner,0,true);auto event=tick(page,owner);check(event.destination==StoryForceDestination::Editor,"editor result");return event.edit;}
struct Save final:ISaveBackend {unsigned accesses=0;bool load(u32,std::vector<u8>&)override{++accesses;return false;}bool store(u32,const u8*,std::size_t)override{++accesses;return false;}bool erase(u32)override{++accesses;return false;}};
ForceSupplement names(const ForceCatalog& lib){ForceSupplement n;std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;check(n.assign(lib,{},"PART ",{"N","A","G","S","C","B"},digits,{"A"},&error),"names");return n;}
}
int main(){using namespace gf;const auto lib=force_inventory_fixture::catalog();ForceInventory owner;check(owner.create(lib)&&owner.place(0,1,2)==ForceChange::Applied&&owner.place(0,2,1)==ForceChange::Applied,"ordered mixed costs100/75/250");
    StoryForceSelection page;check(page.open_prepared(profile,facts(),owner,0,0,{}),"open prefix");active(page,owner);finish(page,owner);ForceBattleRoster roster;
    check(page.take_roster(owner,&roster)&&roster.cost()==175&&roster.source_slot_prefix()==2&&roster.members().size()==2&&roster.members()[0].box_slot==0&&roster.members()[1].box_slot==2&&roster.members()[1].force_slot==1,"canonical prefix and actual member provenance");check(!page.take_roster(owner,&roster),"one shot");
    check(page.open_prepared(profile,facts(),owner,0,0,{}),"reopen stale");active(page,owner);finish(page,owner);check(owner.rename_force(0,"STALE")==ForceChange::Applied&&!page.take_roster(owner,&roster),"post-confirm revision refuses");
    check(page.open_prepared(profile,facts(),owner,0,0,{}),"foreign query open");active(page,owner);finish(page,owner);ForceInventory foreign;check(foreign.create(lib)&&!page.take_roster(foreign,&roster)&&page.take_roster(owner,&roster),"foreign query no roster leak; rightful candidate retained");
    {
        StoryForceSetupPlan plan;auto f=facts();for(s32 value:{s32(-2147483647-1),s32(-1),s32(0),s32(2147483647)}){f.source78=value;check(story_force_setup_plan(profile,f,&plan)&&plan.energy==value,"signed fallback exact no truncation");f.event_energy_override=0;check(story_force_setup_plan(profile,f,&plan)&&plan.energy==0,"zero override over any fallback");f.event_energy_override=-1;}
        f.action=StoryStartAction(3);check(!story_force_setup_plan(profile,f,&plan),"unknown action");f=facts();f.active_event_row=0;check(!story_force_setup_plan(profile,f,&plan),"missing active row no highlight substitution");
    }
    {
        Save save;ForceInventoryStore store(save);const auto supplement=names(lib);StoryForceEditorRequest request;StoryForceEditorBridge bridge;
        {StoryForceSelection source;check(source.open_prepared(profile,facts(),owner,0,0,{}),"lifetime page");active(source,owner);request=edit(source,owner);check(bridge.begin(request,owner,supplement,store),"claimed lifetime editor");}
        check(!request.valid(owner)&&!bridge.session(owner)&&!bridge.step(owner,{},0,0x80),"destroyed page invalidates claimed bridge");check(!save.accesses,"no store access");
    }
    {
        Save save;ForceInventoryStore store(save);const auto supplement=names(lib);check(page.open_prepared(profile,facts(),owner,0,0,{}),"move editor page");active(page,owner);const auto request=edit(page,owner);StoryForceEditorBridge bridge;check(bridge.begin(request,owner,supplement,store),"move bridge begin");StoryForceSelection moved_page=std::move(page);StoryForceEditorBridge moved_bridge=std::move(bridge);
        check(!bridge.session(owner)&&moved_bridge.session(owner),"moved-from bridge cannot act");check(moved_bridge.step(owner,{},0,0x80),"genuine moved editor finish");check(!moved_bridge.complete(page,owner,{}),"wrong/moved-from page cannot consume return");check(moved_bridge.complete(moved_page,owner,{}),"right page return remains consumable");check(!request.valid(owner)&&moved_page.facts().replay.index()==7&&moved_page.facts().active_event_row==3&&!save.accesses,"one request context retained without saves");
    }
    {
        Save save;ForceInventoryStore store(save);const auto supplement=names(lib);StoryForceSelection source;check(source.open_prepared(profile,facts(),owner,0,0,{}),"reload editor page");active(source,owner);const auto request=edit(source,owner);StoryForceEditorBridge bridge;check(bridge.begin(request,owner,supplement,store)&&owner.create(lib),"same owner reload");check(!request.valid(owner)&&!bridge.step(owner,{},0,0x80)&&!bridge.complete(source,owner,{}),"reload cannot acknowledge old editor");
    }
    std::printf("Story force final ownership edges: %u checks PASS; no upstream/main replay\n",checks);
}
