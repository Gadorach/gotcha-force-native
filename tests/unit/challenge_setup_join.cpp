#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/challenge_setup_flow.hpp"
#include "tests/unit/challenge_setup_widgets_fixture.hpp"
namespace {unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"Challenge setup join: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;const auto art=challenge_widgets_fixture::art();const auto common=challenge_widgets_fixture::common();ChallengeSetupWidgets w;
    check(w.open(common,art)&&w.draws().size()==9,"correct source constructor context0 leaves player records offscreen, not hidden");
    check(w.step({2,2,0,0,0})&&w.draws().size()>=10,"players render in context0");
    check(w.step({1,0,0,0,1})&&w.draws().size()<=4,"first box context suppresses player panels immediately");
    check(w.step({1,0,0,0,2})&&w.draws().size()<=4,"second box context also suppresses player panels");
    ChallengeDifficultyVisual whole,parts;check(whole.open(common)&&parts.open_widgets(common),"default and widgets-only difficulty constructors");
    for(unsigned tick=0;tick<40;++tick){ChallengeDifficultyVisualInput in{8,u8(tick>=16?8:0),0,0};check(whole.step(in)&&parts.step_widgets(in,0),"shared difficulty kernels");const auto& a=whole.draws();const auto& b=parts.draws();check(a.size()>b.size(),"parts omit chrome");const auto offset=a.size()-b.size();for(unsigned i=0;i<b.size();++i)check(a[offset+i].record==b[i].record&&a[offset+i].alpha==b[i].alpha&&a[offset+i].joint_time==b[i].joint_time&&std::equal(std::begin(a[offset+i].world.m),std::end(a[offset+i].world.m),std::begin(b[i].world.m)),"legacy difficulty packet tail exact");}
    check(parts.step_widgets({0,0,8,0},1)&&parts.draws().size()<=5,"box context suppresses difficulty body/labels with only fading focus permitted");
    ChallengeEntryProfile entry{0xe0e0e000,0,2,3,true};ChallengeDifficultyProfile difficulty{{101,202,303},2,2};ChallengeSetupPagesProfile pages{2,2,2,1,2};ChallengeSetupFlow flow;ChallengeSetupEvent event;std::array<ChallengeSetupPad,4> pads{};
    check(flow.open(entry,difficulty,pages,10),"fresh multi-participant join");
    auto step=[&](bool ready=true){check(flow.step(true,ready,pads,nullptr,&event),"owned setup callback");};
    while(flow.phase()!=ChallengeSetupPhase::Entry||flow.entry().page().phase()!=ChallengeDifficultyPhase::Active)step();
    check(flow.port()==1&&flow.participants()==2,"entry chooses first participant only");pads[1]={0x102,true};step();check(event.sound_count==2,"navigation before difficulty confirm");
    while(flow.phase()!=ChallengeSetupPhase::Players)step();
    check(event.energy==202&&flow.visual_input().context==0,"energy commits only at handoff; players stay context0");
    while(flow.players().phase()!=ChallengePlayersPhase::Active)step();
    step();check(flow.phase()==ChallengeSetupPhase::Players&&flow.players().phase()==ChallengePlayersPhase::Active,"held difficulty A cannot auto-confirm player page");
    pads[1]={0,true};step();pads[1]={0x200,true};step();check(flow.phase()==ChallengeSetupPhase::DifficultyReturn&&flow.visual_input().context==0,"player B requests same-context difficulty without resources");
    pads[1]={0,true};while(flow.return_page().phase()!=ChallengeDifficultyPhase::Active)step();pads[1]={0x100,true};step();while(flow.phase()!=ChallengeSetupPhase::Players)step();
    check(event.energy==101&&!event.entry.request_resources&&!event.entry.build_widgets,"return reducer resets source difficulty selection, not bank/widgets");
    while(flow.players().phase()!=ChallengePlayersPhase::Active)step();
    pads[1]={0,true};step();pads[1]={0x102,true};step();while(flow.phase()!=ChallengeSetupPhase::Box)step();
    check(event.team_up&&flow.visual_input().context==1&&flow.participants()==2,"team choice alone admits no second controller; context1 only after close");
    pads={};step(false);for(unsigned i=0;i<4;++i)step(false);check(flow.box().phase()==ChallengeBoxPhase::Opening&&!flow.box().remaining(),"opening timer cannot fabricate box resource readiness");step();pads[1]={0x200,true};step();check(event.exit_confirmation_requested&&flow.terminal()&&event.dialog_port==1,"later B is exit request, not main-menu ACK");
    check(!flow.step(true,true,pads,nullptr,&event)&&!event.exit_confirmation_requested&&flow.phase()==ChallengeSetupPhase::ExitConfirmationRequested,"terminal request never replays or changes terminal state");
    check(flow.cancel_exit_request()&&!flow.cancel_exit_request(),"external modal cancellation resumes exact same box");step();check(flow.phase()==ChallengeSetupPhase::Box,"held modal B is not new Back");pads[1]={0,true};step();pads[1]={0x1000,true};step();step(false);step(false);check(flow.box().phase()==ChallengeBoxPhase::Closing,"closing timer also waits for readiness");step();check(event.selection_requested&&event.box.skipped==false&&event.energy==101&&event.team_up,"first box requests Force Selection once, not battle acceptance");
    check(!flow.step(true,true,pads,nullptr,&event)&&!event.selection_requested&&flow.phase()==ChallengeSetupPhase::SelectionRequested,"selection terminal stable");
    //Return-page B uses the same source no-popup cover/cleanup sequence.
    check(flow.open(entry,difficulty,pages,1),"return Back fixture");pads={};while(flow.entry().page().phase()!=ChallengeDifficultyPhase::Active)step();pads[0]={0x100,true};step();while(flow.phase()!=ChallengeSetupPhase::Players)step();pads={};while(flow.players().phase()!=ChallengePlayersPhase::Active)step();pads[0]={0x200,true};step();pads={};while(flow.return_page().phase()!=ChallengeDifficultyPhase::Active)step();step();pads[0]={0x200,true};step();check(flow.phase()==ChallengeSetupPhase::ExitDispatch,"return difficulty B dispatch");step();step();check(event.entry.request_bgm_fade&&flow.fade().draw(),"cover/music requirement not delivered audio");while(flow.phase()!=ChallengeSetupPhase::MainRequested)step();check(event.entry.main_requested&&event.entry.retire_widgets&&!flow.fade().draw(),"return difficulty cleanup/main request");
    std::printf("{\"challenge_setup_join_owned_checks\":%u,\"all_passed\":true,\"exit_popup_selection_and_audio_ack_not_claimed\":true}\n",checks);
}
