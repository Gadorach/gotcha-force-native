#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/story_force_editor_bridge.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
#include "tests/unit/story_map_rebuild_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"Story force: %s\n",why);std::abort();}}
const auto profile=story_map_rebuild_fixture::profile();
StoryForceSetupFacts facts(s32 energy=250){StoryForceSetupFacts f;f.active_event_row=2;f.source50=1;f.source78=energy;check(StoryReplayContext::from_declared_index(5,&f.replay),"declared replay");return f;}
void inventory(ForceInventory& owner,const ForceCatalog& lib){check(owner.create(lib)&&owner.place(7,22,1)==ForceChange::Applied&&owner.place(19,29,2)==ForceChange::Applied,"sparse inventory");}
StoryForceEvent tick(StoryForceSelection& page,const ForceInventory& owner,u32 word=0,bool fade=false,u8 port=0){std::array<SourceSetupPad,4> pads{};pads[port]={word,true};StoryForceEvent e;check(page.step(owner,pads,fade,&e),"controller callback");return e;}
void active(StoryForceSelection& page,const ForceInventory& owner,u32 held=0,u8 port=0){tick(page,owner,held,false,port);check(page.remaining()==4&&page.visible_mask()==4&&!page.interactive_mask(),"opening enter");for(unsigned i=0;i<4;++i)tick(page,owner,held,false,port);check(page.phase()==StoryForcePhase::Active&&page.interactive_mask()==4,"four opening callbacks");}
StoryForceEvent finish_cover(StoryForceSelection& page,const ForceInventory& owner,u16 duration){auto e=tick(page,owner,0,true);check(e.request_cover==duration&&page.phase()==StoryForcePhase::CoverWait,"new cover cannot use same-callback completion");tick(page,owner);check(page.phase()==StoryForcePhase::CoverWait,"no invented fade completion");tick(page,owner,0,true);check(page.phase()==StoryForcePhase::Result,"fade changes dispatch, not result same callback");return tick(page,owner);}
struct Save final:ISaveBackend {
    unsigned reads=0,writes=0;bool load(u32,std::vector<u8>&)override{++reads;return false;}
    bool store(u32,const u8*,std::size_t)override{++writes;return false;}bool erase(u32)override{return false;}
};
ForceSupplement names(const ForceCatalog& lib){ForceSupplement n;std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;check(n.assign(lib,{},"PART ",{"N","A","G","S","C","B"},digits,{"A"},&error),"owned names");return n;}
void owned(){const auto lib=force_inventory_fixture::catalog();ForceInventory owner;inventory(owner,lib);
    auto f=facts();StoryForceSetupPlan plan;check(story_force_setup_plan(profile,f,&plan)&&plan.selector_enabled&&plan.energy==250,"normal source78");
    f.source50=0;check(story_force_setup_plan(profile,f,&plan)&&!plan.selector_enabled,"selector source50 gate");f.action=StoryStartAction::Special;f.source78=-1;check(story_force_setup_plan(profile,f,&plan)&&plan.selector_enabled&&plan.energy==2200,"SPECIAL enable+2200 independent of negative unused78");
    f.event_energy_override=0;check(story_force_setup_plan(profile,f,&plan)&&plan.energy==0,"zero event override preserved");f.event_energy_override=333;check(story_force_setup_plan(profile,f,&plan)&&plan.energy==333,"event override wins SPECIAL");f.active_event_row=5;check(!story_force_setup_plan(profile,f,&plan)&&plan.energy==333,"invalid row no partial plan");
    f=facts();f.source78=-1;check(story_force_setup_plan(profile,f,&plan)&&plan.energy==-1,"source signed unlimited fallback preserved");f=facts();f.replay={};check(!story_force_setup_plan(profile,f,&plan),"missing replay refuses");
    for(unsigned requested=0;requested<20;++requested){StoryForceSelection page;check(page.open_prepared(profile,facts(),owner,requested,0,{}),"prepared open");check(page.selected_force()==(requested==7?7:requested==19?19:0)&&page.slots().size()==3,"requested actual slot to nonempty row");}
    {
        StoryForceSelection page;check(page.open_prepared(profile,facts(),owner,19,2,{}),"port2 open");active(page,owner,0x1100,2);check(page.input().held==0x180,"Start+A held before activation");tick(page,owner,0x1100,false,2);check(page.phase()==StoryForcePhase::Active,"held confirm consumed during opening");tick(page,owner,0,false,2);
        auto e=tick(page,owner,3,false,2);check(e.changed&&page.selected_force()==7&&e.sounds[0]==119,"left before right and row != slot");tick(page,owner,0,false,2);e=tick(page,owner,0x700,false,2);check(page.phase()==StoryForcePhase::CoverEnter&&e.sounds[0]==123,"B before X before A");
        //All callbacks above explicitly use port2; finish on that same port.
        e=tick(page,owner,0,true,2);check(e.request_cover==30,"Back cover30");tick(page,owner,0,true,2);e=tick(page,owner,0,false,2);check(e.destination==StoryForceDestination::Back,"Back route only after cover dispatch");ForceBattleRoster r;check(!page.take_roster(owner,&r),"Back no roster");
    }
    {
        StoryForceSelection page;check(page.open_prepared(profile,facts(),owner,19,0,{}),"accept open");active(page,owner);auto e=tick(page,owner,0x1000);check(page.phase()==StoryForcePhase::CloseEnter&&e.sounds[0]==122&&page.closing_mask()==4&&!page.visible_mask(),"Start accepted with separate close-enter callback");
        tick(page,owner);check(page.remaining()==32,"close initial32");for(unsigned i=0;i<32;++i){e=tick(page,owner);check(!e.request_cover&&page.phase()==StoryForcePhase::Closing,"zero still waits");}
        check(page.remaining()==0,"decrement-store zero");e=tick(page,owner,0,true);check(e.request_cover==60&&page.remaining()==-1&&page.phase()==StoryForcePhase::CoverWait,"negative gate not zero");ForceBattleRoster r;check(!page.take_roster(owner,&r),"cover request not result authority");tick(page,owner,0,true);e=tick(page,owner);check(e.destination==StoryForceDestination::Stored&&page.facts()==facts(),"captured event+replay retained");
        check(page.take_roster(owner,&r)&&r.force()==19&&r.members().size()==1&&r.cost()==75&&!page.take_roster(owner,&r),"real shared one-shot roster");
    }
    for(s32 energy:{-1,0,74,75}){StoryForceSelection page;check(page.open_prepared(profile,facts(energy),owner,19,0,{}),"unlimited/edge budget open");active(page,owner);auto e=tick(page,owner,0x100);const bool rejected=energy>0&&energy<75;check(e.rejected==rejected&&page.phase()==(rejected?StoryForcePhase::Active:StoryForcePhase::CloseEnter),"budget equality/unlimited rule not positive default");}
    {
        ForceInventory empty;check(empty.create(lib,false),"empty owner");StoryForceSelection page;check(!page.open_prepared(profile,facts(),empty,0,0,{})&&page.phase()==StoryForcePhase::Closed,"all empty refuses no Random fallback");
        auto disabled=facts();disabled.source50=0;check(!page.open_prepared(profile,disabled,owner,0,0,{}),"disabled selector cannot be forced");check(!page.open_prepared(profile,facts(),owner,20,0,{})&&!page.open_prepared(profile,facts(),owner,0,4,{}),"slot/port bounds");
    }
    {
        ForceInventory edited;inventory(edited,lib);CheatSettings cheats;cheats.borg_editor=true;check(edited.edit_borg(cheats,false,2,edited.revision(),BorgEditField::HP,99)==ForceChange::Applied,"edited Borg fixture");StoryForceSelection page;check(page.open_prepared(profile,facts(),edited,19,0,{}),"edited page opens");active(page,edited);check(tick(page,edited,0x100).rejected,"shared edited roster guard unchanged, not Story policy proof");
    }
    {
        ForceInventory changed;inventory(changed,lib);StoryForceSelection page;check(page.open_prepared(profile,facts(),changed,0,0,{}),"revision guard open");active(page,changed);check(changed.rename_force(0,"CHANGED")==ForceChange::Applied,"external revision");StoryForceEvent e;std::array<SourceSetupPad,4> pads{};check(!page.step(changed,pads,false,&e)&&page.phase()==StoryForcePhase::Failed&&!e.sound_count,"stale owner refuses before output");
        check(page.open_prepared(profile,facts(),changed,0,0,{})&&changed.create(lib),"reload fixture");check(!page.step(changed,pads,false,&e),"same object reload different scope");
    }
    {
        ForceInventory edit_owner;inventory(edit_owner,lib);StoryForceSelection page;check(page.open_prepared(profile,facts(),edit_owner,7,0,{}),"editor open");active(page,edit_owner);check(tick(page,edit_owner,0x500).sounds[0]==122,"X before A");const auto event=finish_cover(page,edit_owner,30);check(event.destination==StoryForceDestination::Editor&&event.edit.force()==7&&event.edit.current(edit_owner),"post-cover scoped editor request");
        Save save;ForceInventoryStore store(save);const auto supplement=names(lib);StoryForceEditorBridge bridge,duplicate;check(bridge.begin(event.edit,edit_owner,supplement,store)&&!duplicate.begin(event.edit,edit_owner,supplement,store),"one claim genuine editor");check(bridge.session(edit_owner)->energy_limit()==250&&bridge.session(edit_owner)->cost_mode()==ForceCostMode::Full,"same budget/mode");check(!bridge.complete(page,edit_owner,{}),"no caller success without actual return");
        check(bridge.choose_force(edit_owner,1)&&bridge.session(edit_owner)->force()==8,"real editor can choose empty actual slot");check(bridge.step(edit_owner,{},0,0x80),"real Start finish edit");check(bridge.session(edit_owner)->view()==ForceEditorView::Selection&&bridge.session(edit_owner)->force()==0,"shared first nonempty fallback, not row as slot");
        SourceSetupButtons history;source_setup_input_step({0x1100,true},&history);check(bridge.complete(page,edit_owner,history)&&!event.edit.valid(edit_owner)&&!bridge.complete(page,edit_owner,history),"one-shot genuine editor return invalidates token");check(page.selected_force()==0&&page.facts()==facts()&&page.input().held==0x180,"rebuild and captured context/carried input");active(page,edit_owner,0x1100);tick(page,edit_owner,0x1100);check(page.phase()==StoryForcePhase::Active&&!save.reads&&!save.writes,"held-return confirm not replayed; no local save");
        StoryForceSelection moved=std::move(page);StoryForceEvent e;std::array<SourceSetupPad,4> pads{};check(!page.step(edit_owner,pads,false,&e)&&moved.step(edit_owner,pads,false,&e),"move leaves one live controller");
    }
    {
        ForceInventory guard;inventory(guard,lib);StoryForceSelection page;check(page.open_prepared(profile,facts(),guard,7,0,{}),"request lifetime open");active(page,guard);tick(page,guard,0x400);const auto event=finish_cover(page,guard,30);Save save;ForceInventoryStore store(save);const auto supplement=names(lib);StoryForceEditorBridge bridge;ForceInventory foreign;inventory(foreign,lib);
        check(!bridge.begin(event.edit,foreign,supplement,store),"foreign inventory request refuses");check(guard.rename_force(7,"LATER")==ForceChange::Applied&&!bridge.begin(event.edit,guard,supplement,store),"unclaimed stale revision refuses");check(!page.open_prepared(profile,facts(),guard,20,0,{})&&!event.edit.valid(guard),"failed reopen invalidates earlier token");
    }
    {
        StoryForceSelection page;check(page.open_prepared(profile,facts(),owner,0,0,{}),"budget guard open");std::array<SourceSetupPad,4> pads{};StoryForceEvent event;for(unsigned i=0;i<8192;++i)check(page.step(owner,pads,false,&event),"owned native update budget prefix");check(!page.step(owner,pads,false,&event)&&page.phase()==StoryForcePhase::Failed,"budget never forces finish");
    }
}
//Declared OWNED inventory/controller corpus, not original gameplay or rendering.
//Explicit test wire below is neither a checkpoint format nor a source image.
void corpus(const char* path){FILE* file=std::fopen(path,"wx");check(file,"exclusive test wire");const auto lib=force_inventory_fixture::catalog();unsigned rows=0;
    auto word=[&](u32 value){u8 b[4];store_be32(b,value);check(std::fwrite(b,1,4,file)==4,"wire write");};
    for(unsigned pattern=0;pattern<4;++pattern)for(unsigned requested=0;requested<20;++requested)for(unsigned port=0;port<4;++port)for(unsigned action=0;action<4;++action){
        ForceInventory owner;check(owner.create(lib),"corpus owner");
        if(pattern==0){check(owner.place(7,0,1)==ForceChange::Applied&&owner.place(19,0,2)==ForceChange::Applied,"corpus sparse");}
        else if(pattern==1){check(owner.delete_force(0)==ForceChange::Applied&&owner.place(19,0,2)==ForceChange::Applied,"corpus slot20 only");}
        else if(pattern==2){for(unsigned force=1;force<20;++force)check(owner.place(force,0,force%3)==ForceChange::Applied,"corpus all20");}
        else check(owner.place(7,0,1)==ForceChange::Applied,"corpus zero budget");
        auto f=facts(pattern==3?0:250);f.action=StoryStartAction(action%3);f.active_event_row=action+1;StoryForceSelection page;check(page.open_prepared(profile,f,owner,requested,u8(port),{}),"corpus open");
        for(unsigned tick_index=0;tick_index<56;++tick_index){std::array<SourceSetupPad,4> pads{};u32 raw=tick_index==5?(requested%2?1u:2u):tick_index==7?(action==0?0x100u:action==1?0x1000u:action==2?0x500u:0x700u):tick_index==50?0x200u:0;
            for(unsigned p=0;p<4;++p)pads[p]={p==port?raw:0x703u,true};
            StoryForceEvent event;
            if(!page.terminal()&&!page.suspended())check(page.step(owner,pads,tick_index>=42,&event),"corpus callback");
            for(u32 value:{u32(page.phase()),u32(page.cursor()),u32(page.slots().size()),u32(page.selected_force()),u32(s32(page.remaining())),u32(page.visible_mask()),u32(page.interactive_mask()),u32(page.closing_mask()),u32(page.input().held),u32(page.input().previous),u32(page.input().repeated),u32(page.input().pressed),u32(event.request_cover),u32(event.sound_count),u32(event.sounds[0]),u32(event.sounds[1]),u32(event.destination),u32(event.changed),u32(event.rejected),u32(page.energy()),u32(page.facts().replay.index()),page.facts().active_event_row})word(value);
            check(std::fwrite(page.input().counters.data(),1,16,file)==16,"wire counters");++rows;
        }
    }
    check(std::fclose(file)==0&&rows==71680,"corpus close/row count");std::printf("declared selector rows %u / test bytes %u\n",rows,rows*104);
}
}
int main(int argc,char** argv){owned();std::printf("Story force controller/editor: %u owned checks PASS; prepared values/declared fade only, no map/battle/save authority\n",checks);if(argc==2)corpus(argv[1]);else if(argc!=1)return 2;std::printf("total checks including declared corpus and test-wire I/O: %u\n",checks);}
