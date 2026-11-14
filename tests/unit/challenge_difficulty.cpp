#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/gameplay/challenge_difficulty.hpp"
namespace {
unsigned checks=0;
void check(bool value,const char* message){++checks;if(!value){std::fprintf(stderr,"Challenge difficulty: %s\n",message);std::abort();}}
std::vector<gf::u8> fixture(){
    using namespace gf;std::vector<u8> bytes(40);store_be32(bytes.data(),0x47464344);store_be32(bytes.data()+4,1);
    // OWNED values: actual original energy rows are exercised separately.
    for(unsigned i=0;i<3;++i)store_be32(bytes.data()+8+4*i,101+101*i);
    store_be16(bytes.data()+20,3);store_be16(bytes.data()+22,5);
    store_be16(bytes.data()+24,1);store_be16(bytes.data()+26,2);store_be16(bytes.data()+28,0x180);store_be16(bytes.data()+30,0x200);bytes[32]=8;return bytes;
}
}
int main(){
    using namespace gf;const auto bytes=fixture();std::string error;ChallengeDifficultyProfile profile;
    check(challenge_difficulty_profile_load({bytes.data(),bytes.size()},&profile,&error)&&profile.valid(),"owned bounded profile");
    for(unsigned n=0;n<40;++n){auto bad=bytes;bad[n]^=0x80;ChallengeDifficultyProfile result;
        const bool ok=challenge_difficulty_profile_load({bad.data(),bad.size()},&result,&error);
        if(n<8||n>=24||n==8||n==12||n==16||n==20||n==22)check(!ok&&!result.valid(),"header/domain/reserved/negative/counter corruption refuses and clears");
        else check(!ok||result.valid(),"represented data cannot create invalid profile");}
    ChallengeDifficultyProfile invalid;check(!challenge_difficulty_profile_load({bytes.data(),39},&invalid,&error)&&!challenge_difficulty_profile_load({bytes.data(),40},nullptr,&error),"truncated/null profile refusal");
    ChallengeDifficulty idle;ChallengeDifficultyEvent event;check(!idle.step(0,0,&event)&&!idle.open({}),"unconfigured refuses");
    auto active=[&](unsigned selection){ChallengeDifficulty page;check(page.open(profile),"fresh owner");check(!page.step(0,0,nullptr)&&page.updates()==0,"null output no clock mutation");check(page.step(3,0x380,&event)&&page.phase()==ChallengeDifficultyPhase::Opening&&page.remaining()==3&&event.sound_count==0,"entry ignores held commands");
        for(unsigned i=0;i<3;++i)check(page.step(3,0x380,&event)&&event.sound_count==0&&page.remaining()==2-i,"opening consumes no input outputs");
        check(page.phase()==ChallengeDifficultyPhase::Active&&page.visible_mask()==8&&page.interactive_mask()==8,"opening ends on decrement to zero");
        for(unsigned i=0;i<selection;++i)check(page.step(2,0,&event)&&event.changed&&event.sounds[0]==119,"actual navigation operation");
        return page;};
    for(unsigned initial=0;initial<3;++initial)for(u16 repeated:{u16(0),u16(1),u16(2),u16(3),u16(0xfffc)})for(u16 pressed:{u16(0),u16(0x80),u16(0x100),u16(0x180),u16(0x200),u16(0x380)}){
        auto page=active(initial);check(page.step(repeated,pressed,&event),"active step");
        const unsigned expected=(repeated&1)?(initial?initial-1:0):((repeated&2)&&initial<2?initial+1:initial);
        const bool changed=expected!=initial;check(page.selection()==expected&&event.changed==changed,"left priority and clamped nonwrap navigation");
        if(changed)check(event.sounds[0]==119,"navigation precedes accepted action sound");
        if(pressed&0x200){check(page.phase()==ChallengeDifficultyPhase::BackRequested&&event.back_requested&&!event.players_requested&&event.committed_energy==0&&event.sounds[changed?1:0]==123,"B priority without committing energy/faking return");}
        else if(pressed&0x180){check(page.phase()==ChallengeDifficultyPhase::Closing&&page.remaining()==5&&page.visible_mask()==8&&page.interactive_mask()==0&&page.closing_mask()==8&&event.sounds[changed?1:0]==122&&event.committed_energy==0,"A or Start requests close, not immediate energy write");
            for(unsigned i=0;i<5;++i){check(page.step(3,0x380,&event)&&event.sound_count==0,"closing ignores input and sound replay");check(event.players_requested==(i==4),"players request exactly at close completion");}
            check(page.visible_mask()==0&&event.committed_energy==profile.energy[expected]&&page.phase()==ChallengeDifficultyPhase::PlayersRequested,"original selected energy commits on close completion");}
        else check(page.phase()==ChallengeDifficultyPhase::Active&&event.sound_count==unsigned(changed)&&event.committed_energy==0,"non-action updates never commit");
        if(page.phase()!=ChallengeDifficultyPhase::Active){const auto count=page.updates();check(!page.step(0,0,&event)&&!event.sound_count&&!event.players_requested&&!event.back_requested&&page.updates()==count,"terminal request emitted once, not replayed or counted as next page");}
    }
    auto page=active(2);check(!page.open({})&&page.phase()==ChallengeDifficultyPhase::Closed&&page.selected_energy()==0,"failed reload cannot replay previous page");
    std::printf("{\"challenge_difficulty_owned_checks\":%u,\"all_passed\":true,\"normalized_input_page_only\":true}\n",checks);
}
