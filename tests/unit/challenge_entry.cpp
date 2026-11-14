#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/gameplay/challenge_difficulty_flow.hpp"
namespace {
unsigned checks=0;void check(bool value,const char* message){++checks;if(!value){std::fprintf(stderr,"Challenge entry: %s\n",message);std::abort();}}
std::vector<gf::u8> entry_bytes(){using namespace gf;std::vector<u8> out(32);store_be32(out.data(),0x47464345);store_be32(out.data()+4,1);store_be32(out.data()+8,0xe0e0e000);store_be16(out.data()+12,3);store_be16(out.data()+14,4);out[20]=32;out[21]=27;return out;}
}
int main(){
    using namespace gf;std::string error;auto bytes=entry_bytes();ChallengeEntryProfile entry;ChallengeDifficultyProfile difficulty{{101,202,303},3,5};ChallengeEntryEvents event;
    check(challenge_entry_profile_load({bytes.data(),bytes.size()},&entry,&error)&&entry.bgm_member==0,"member0 is represented, not missing audio");
    for(unsigned i:{0u,4u,20u,21u,22u,24u,28u}){auto bad=bytes;bad[i]^=1;ChallengeEntryProfile result;check(!challenge_entry_profile_load({bad.data(),bad.size()},&result,&error)&&!result.valid(),"profile domain/reserved failure clears output");}
    check(!challenge_entry_profile_load({},&entry,&error)&&!challenge_entry_profile_load({bytes.data(),31},&entry,&error),"missing/truncated profile");
    check(challenge_entry_profile_load({bytes.data(),bytes.size()},&entry,&error),"restore profile");
    for(u32 bits=0;bits<65536;++bits){const u16 expected=u16((bits&0xf7f)|((bits&0x1000)>>5));check(challenge_setup_fold_buttons(bits)==expected,"all low-word button folding combinations");}
    for(unsigned bit=0;bit<32;++bit){const u16 expected=bit<=6?u16(1u<<bit):bit>=8&&bit<=11?u16(1u<<bit):bit==12?0x80:bit>=16&&bit<=19?u16(1u<<(bit-16)):bit>=20&&bit<=23?u16(1u<<(bit-8)):0;check(challenge_setup_fold_buttons(u32(1u)<<bit)==expected,"independent high-direction aliases and dropped bits");}
    ChallengeSetupButtons history;
    for(unsigned frame=1;frame<=72;++frame){challenge_setup_input_step({0x10002,true},&history);check(history.held==3&&history.pressed==(frame==1?3:0),"combined physical/high-direction held and edge");check(history.repeated==((frame==1||(frame>=32&&(frame-32)%5==0))?3:0),"source repeat32/reload27, not MAIN's cadence");}
    challenge_setup_input_step({0xffffffffu,false},&history);check(!history.held&&!history.repeated&&!history.pressed&&history.counters==std::array<u8,16>{},"disconnect resets held/counters without inventing a press");
    challenge_setup_input_step({0x1100,true},&history);check(history.pressed==0x180,"Start folds to80, A remains100");
    history.counters[0]=255;history.held=1;challenge_setup_input_step({1,true},&history);check(!history.counters[0]&&!history.pressed&&!history.repeated,"source byte store wraps before repeat test");
    GamePadState digital;digital.a=digital.start=digital.dpad_left=digital.r=true;const auto sample=challenge_setup_digital_pad(digital,true);check(sample.source_buttons==0x1121&&sample.connected,"native digital adapter retains physical R and raw Start");
    digital.stick_x=127;check(challenge_setup_digital_pad(digital,true).source_buttons==sample.source_buttons,"no invented source analogue threshold");
    std::array<ChallengeSetupPad,4> pads{};ChallengeDifficultyFlow flow;
    check(!flow.step(true,pads,&event)&&!flow.open(entry,difficulty,0)&&!flow.open(entry,difficulty,16),"closed/invalid participant masks refuse");
    check(flow.open(entry,difficulty,10)&&!flow.step(true,pads,nullptr)&&flow.updates()==0,"fresh flow, null output cannot advance");
    check(flow.step(false,pads,&event)&&event.request_resources&&!event.request_bgm_start,"resource request once, no fake load ACK");
    for(unsigned i=0;i<7;++i)check(flow.step(false,pads,&event)&&flow.phase()==ChallengeEntryPhase::ResourceWait&&!event.request_resources&&!event.request_bgm_start,"busy gate stays pending, never forced");
    check(flow.step(true,pads,&event)&&event.request_bgm_start&&event.bgm_member==0&&flow.port()==1&&flow.participants()==2&&flow.fade().remaining()==2,"native resource observation selects first participant and requests member0/reveal");
    check(flow.step(true,pads,&event)&&event.build_widgets&&flow.fade().remaining()==1,"widgets constructed on next callback, fade advances after");
    check(flow.step(true,pads,&event)&&flow.phase()==ChallengeEntryPhase::RevealWait&&!flow.fade().remaining(),"reveal completion waits until next scene callback");
    check(flow.step(true,pads,&event)&&flow.phase()==ChallengeEntryPhase::Page&&!flow.fade().draw(),"reveal cleared before difficulty entry");
    pads[0]={0x1303,true};pads[3]=pads[0];
    check(flow.step(true,pads,&event)&&flow.page().phase()==ChallengeDifficultyPhase::Opening&&flow.page().remaining()==3,"difficulty enter callback does not consume input");
    for(unsigned i=0;i<3;++i)check(flow.step(true,pads,&event)&&event.sound_count==0&&flow.input().held==0,"opening doesn't sample page input or other ports");
    check(flow.step(true,pads,&event)&&flow.page().phase()==ChallengeDifficultyPhase::Active&&flow.page().selection()==0&&!event.sound_count,"unselected ports cannot confirm or navigate");
    pads[1]={0x102,true};check(flow.step(true,pads,&event)&&event.sound_count==2&&event.sounds[0]==119&&event.sounds[1]==122&&flow.page().selection()==1&&event.committed_energy==0,"selected pad navigates then confirms, not early energy commit");
    for(unsigned i=0;i<5;++i){check(flow.step(true,pads,&event)&&event.sound_count==0,"closing no repeated actions");check(event.players_requested==(i==4),"player request only after exact close countdown");}
    check(flow.phase()==ChallengeEntryPhase::PlayersRequested&&event.port==1&&event.difficulty==1&&event.committed_energy==202&&!flow.step(true,pads,&event)&&!event.players_requested,"one-shot energy/player request, not completed next page");
    //Fresh initial-context Back skips modal, but retains dispatch/cover/cleanup callbacks.
    check(flow.open(entry,difficulty,1),"fresh Back flow");pads={};
    while(flow.phase()!=ChallengeEntryPhase::Page||flow.page().phase()!=ChallengeDifficultyPhase::Active)check(flow.step(true,pads,&event),"reach active source page");
    pads[0]={0x300,true};check(flow.step(true,pads,&event)&&flow.phase()==ChallengeEntryPhase::BackDispatch&&event.sounds[0]==123&&!event.committed_energy,"B wins simultaneous A, no energy commit");
    check(flow.step(true,pads,&event)&&flow.phase()==ChallengeEntryPhase::ExitStart&&!event.request_bgm_fade,"B34 skips popup on its own callback");
    check(flow.step(true,pads,&event)&&flow.phase()==ChallengeEntryPhase::ExitFadeWait&&event.request_bgm_fade&&event.bgm_fade_updates==4&&flow.fade().remaining()==3,"D44 requests cover/music fade without audio ACK");
    for(unsigned i=0;i<3;++i)check(flow.step(true,pads,&event)&&flow.phase()==ChallengeEntryPhase::ExitFadeWait&&!event.main_requested,"cover countdown not prematurely completed");
    check(flow.step(true,pads,&event)&&flow.phase()==ChallengeEntryPhase::ReturnCleanup&&flow.fade().draw()&&!event.retire_widgets,"DA8 retains final opaque cover through dispatch");
    check(flow.step(true,pads,&event)&&flow.phase()==ChallengeEntryPhase::MainRequested&&event.main_requested&&event.retire_widgets&&!flow.fade().draw(),"E90 cleanup/return request once");
    check(!flow.step(true,pads,&event)&&!event.main_requested,"terminal return cannot replay");
    check(flow.open(entry,difficulty,1)&&flow.step(false,pads,&event),"native budget fixture");
    for(unsigned i=1;i<8192;++i)if(!flow.step(false,pads,&event))check(false,"allowed busy prefix");
    check(!flow.step(false,pads,&event)&&flow.phase()==ChallengeEntryPhase::Failed&&!event.main_requested&&!event.players_requested,"native work bound refuses, never forces source readiness or completion");
    std::printf("{\"challenge_entry_owned_checks\":%u,\"numeric_low_word_cases\":65536,\"all_passed\":true,\"no_device_or_original_clock_proof\":true}\n",checks);
}
