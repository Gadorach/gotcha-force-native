#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/challenge_setup_flow.hpp"
#include "engine/gameplay/challenge_force_notices.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"retained Force: %s\n",why);std::abort();}}
struct Save final:gf::ISaveBackend {unsigned reads=0,writes=0;bool load(gf::u32,std::vector<gf::u8>&)override{++reads;return false;}bool store(gf::u32,const gf::u8*,std::size_t)override{++writes;return true;}bool erase(gf::u32)override{return false;}};
}
int main(){
    using namespace gf;ChallengeForceNoticesArt art;art.bank_hash=1;art.prompt_record=4;art.initial_tick=31;art.team={ChallengeForceNoticePart{5,{0,0,0}},ChallengeForceNoticePart{-1,{1,0,0}},ChallengeForceNoticePart{-1,{2,0,0}}};art.alpha_step=.25f;art.active_blink=2;art.closing_blink=32;art.port_record_base=10;check(art.valid(),"owned notices");
    ChallengeForceNotices notices;check(notices.open(art)&&notices.step({4,0,0,true,1,{2,-1}})&&notices.level()==.25f&&notices.draws().size()==2,"type14 opens in first box, not only Force active");
    check(notices.draws()[1].record==12&&notices.draws()[0].authored_alpha,"source controller record and original panel material alpha");
    for(unsigned i=0;i<3;++i)check(notices.step({4,4,0,true,1,{2,3}}),"team reveal callback");
    check(notices.level()==1&&notices.team_tick()==0,"reveal clamps/reset tick until fully open");
    for(unsigned i=0;i<32;++i)check(notices.step({4,4,0,true,1,{2,3}}),"source team blink callback");
    check(notices.team_tick()==32&&notices.draws().back().material_time==0,"unselected second controller does not blink");
    check(notices.step({4,0,4,true,2,{2,3}})&&notices.level()==2&&notices.team_tick()==0,"second context changes panel selector and resets blink clock");
    check(notices.step({0,0,0,false,0,{-1,-1}})&&notices.draws().empty()&&notices.prompt_tick()==31,"inactive prompt/team indicators disappear");
    check(!notices.step({0,0,0,true,3,{0,1}})&&!notices.ready()&&notices.draws().empty(),"invalid context refuses");
    ChallengeExitDialogArt exit;exit.bank_hash=1;exit.depth=-260;exit.panel_end=42;exit.close_start=8;exit.choice_step=56;exit.open_wait=1;exit.cancel_wait=1;exit.accept_wait=2;
    const auto catalog=force_inventory_fixture::catalog();ForceInventory owner;check(owner.create(catalog)&&owner.place(19,0,1)==ForceChange::Applied,"shared inventory");
    ChallengeEntryProfile entry{0xe0e0e000,0,1,2,true};ChallengeDifficultyProfile difficulty{{1000,2000,3000},1,1};ChallengeSetupPagesProfile pages{1,1,1,1,1};ChallengeForceSelectionProfile force{1,2};
    ChallengeSetupFlow flow;ChallengeSetupEvent event;std::array<ChallengeSetupPad,4> pads{};
    auto step=[&](){check(flow.step(true,true,pads,nullptr,&event,&owner),"retained setup callback");};
    auto reach=[&](bool team){pads={};check(flow.open(entry,difficulty,pages,1)&&!flow.enable_force_selection(force)&&flow.enable_exit_dialog(exit)&&flow.enable_force_selection(force),"explicit dialogs/force opt-in");
        for(unsigned i=0;i<100&&flow.entry().page().phase()!=ChallengeDifficultyPhase::Active;++i)step();
        pads[0]={0x100,true};step();while(flow.phase()!=ChallengeSetupPhase::Players)step();pads={};while(flow.players().phase()!=ChallengePlayersPhase::Active)step();step();if(team){pads[0]={2,true};step();}pads[0]={0x100,true};step();while(flow.phase()!=ChallengeSetupPhase::Box)step();pads={};while(flow.box().phase()!=ChallengeBoxPhase::Active)step();pads[0]={0x1000,true};step();while(flow.phase()!=ChallengeSetupPhase::SelectionRequested)step();
        check(event.selection_requested&&flow.accept_selection_inventory(owner)&&flow.phase()==ChallengeSetupPhase::Force,"inventory handoff not a terminal screen ACK in explicit mode");
        while(flow.force_selection().phase()!=ChallengeForcePhase::Active)step();
    };
    reach(false);pads[0]={0x200,true};step();check(flow.phase()==ChallengeSetupPhase::ExitConfirmationRequested&&event.force.exit_requested,"Force B enters same original dialog reducer");step();pads={};while(flow.exit_dialog().phase()!=ChallengeExitPhase::Active)step();step();pads[0]={0x100,true};step();while(flow.phase()==ChallengeSetupPhase::ExitConfirmationRequested)step();check(event.exit_cancelled&&flow.phase()==ChallengeSetupPhase::Force&&flow.force_selection().cursor()==0,"No returns to same force/cursor");step();check(flow.force_selection().phase()==ChallengeForcePhase::Active&&!event.force.finish_requested,"modal-held A cannot accept force");
    pads={};step();pads[0]={0x100,true};step();while(flow.phase()==ChallengeSetupPhase::Force)step();ForceBattleRoster roster;check(flow.phase()==ChallengeSetupPhase::ForceFinished&&flow.take_force_roster(owner,&roster)&&owner.current(roster)&&roster.force()==0,"ordinary immutable roster publication");
    check(!flow.take_force_roster(owner,&roster)&&!flow.step(true,true,pads,nullptr,&event,&owner)&&flow.phase()==ChallengeSetupPhase::ForceFinished,"one-shot transfer and terminal callback");
    reach(true);check(flow.phase()==ChallengeSetupPhase::Force&&!event.force.second_box_requested&&flow.box().context()==1,"TEAM UP has not admitted second box before Force acceptance");pads={};step();pads[0]={0x100,true};step();check(flow.force_selection().phase()==ChallengeForcePhase::Closing&&!event.force.second_box_requested,"accept still waits for close");step();check(flow.phase()==ChallengeSetupPhase::Force&&!event.force.second_box_requested,"first close callback remains same first force");step();check(flow.phase()==ChallengeSetupPhase::SecondBoxRequested&&event.force.second_box_requested&&flow.box().context()==1&&flow.participants()==1,"only after accepted close emits second-box REQUEST, not a joined human");
    reach(false);pads={};step();pads[0]={1,true};step();check(flow.force_selection().cursor()==-1,"Random distinct row");pads[0]={0x100,true};step();while(flow.phase()==ChallengeSetupPhase::Force)step();ChallengeForceRequest random;check(!flow.take_force_roster(owner,&roster)&&flow.take_random_request(owner,&random)&&random.current(owner),"Random yields actual scoped request, not stored force0");const auto retained=random;check(!flow.take_random_request(owner,&random),"Random request transferred once");
    reach(false);check(!retained.valid(owner),"old request expires on new selection owner");pads[0]={0x400,true};step();check(flow.phase()==ChallengeSetupPhase::ForceEditorRequested&&event.force.editor_requested,"real editor request boundary");
    ForceSupplement names;std::array<std::string,6> variants={"N","A","G","S","C","B"};std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;check(names.assign(catalog,{},"PART ",variants,digits,{"A"},&error),"owned editor text");Save backend;ForceInventoryStore store(backend);ChallengeForceEditorBridge bridge;
    check(bridge.begin(event.force.edit,owner,names,store)&&!flow.complete_force_editor(bridge,owner,{}),"no forged editor return before real session publishes one");
    check(bridge.choose_force(owner,-1)&&bridge.step(owner,{},0,0x80),"real editor selection return");ChallengeSetupButtons history;challenge_setup_input_step({0x1000,true},&history);check(flow.complete_force_editor(bridge,owner,history)&&flow.force_selection().selected_force()==19&&!backend.reads&&!backend.writes,"genuine return retains force20 and no save");pads[0]={0x1000,true};while(flow.force_selection().phase()!=ChallengeForcePhase::Active)step();step();check(flow.phase()==ChallengeSetupPhase::Force&&!event.force.finish_requested,"return-held Start consumed without new quarantine");
    pads={};step();pads[0]={0x200,true};step();pads={};while(flow.exit_dialog().phase()!=ChallengeExitPhase::Active)step();step();pads[0]={0x101,true};step();while(flow.phase()==ChallengeSetupPhase::ExitConfirmationRequested)step();check(event.exit_accepted&&flow.phase()==ChallengeSetupPhase::ExitStart,"Force Yes enters shared cover cleanup");while(flow.phase()!=ChallengeSetupPhase::MainRequested)step();check(event.entry.main_requested&&event.entry.retire_widgets,"main REQUEST only after cover/cleanup");
    std::printf("retained Force: %u owned checks PASS\n",checks);
}
