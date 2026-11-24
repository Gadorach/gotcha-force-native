#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_initial_auxiliary_world.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;const std::array<u32,24> words{0x47465754,1,96,0,0x800c79f8,0x800c7a0c,0x800c7a1c,0x800c7a24,0x800c7c6c,0x800537a8,0x8005685c,0x80089554,0x80089658,0x8008972c,0xa8,6,3,0x615,41,2,40,0,0x144,2};
    std::array<u8,96> raw{};for(unsigned i=0;i<words.size();++i)store_be32(raw.data()+4*i,words[i]);
    BattleInitialWorldTailProfile profile,bad;check(profile.load({raw.data(),raw.size()},&error));
    for(unsigned i=0;i<raw.size();++i){auto corrupt=raw;corrupt[i]^=1;check(!bad.load({corrupt.data(),corrupt.size()},&error));check(!bad.load({raw.data(),i},&error));}
    StartupAuxiliaryState state;state.allocated=true;state.captured94=0x615;startup_auxiliary_detail::initialize_inline_two(state);for(auto& m:state.matrices)std::fill_n(m.m,16,0.f);
    const auto original=state;
    for(u8 channel:{u8(0),u8(2)}){
        for(unsigned count=0;count<256;++count){std::array<u8,6> counters{};counters[channel]=u8(count);check(battle_initial_inactive_auxiliary_world(profile,channel,counters,state,&error)==!count);check(state==original);}
        for(unsigned other=0;other<6;++other)if(other!=channel){std::array<u8,6> counters{};counters[other]=255;check(battle_initial_inactive_auxiliary_world(profile,channel,counters,state,&error));} //only the source-selected byte is read
    }
    for(unsigned channel=0;channel<256;++channel)check(battle_initial_inactive_auxiliary_world(profile,u8(channel),{},state,&error)==(channel==0||channel==2));
    check(!battle_initial_inactive_auxiliary_world(bad,0,{},state,&error));
    for(unsigned edit=0;edit<10;++edit){
        auto changed=state;
        switch(edit){case 0:changed.allocated=false;break;case 1:changed.captured94=10;break;case 2:changed.phase=2;break;case 3:changed.counter=1;break;case 4:changed.render_group=0;break;case 5:changed.cached_a8=1;break;case 6:changed.previous_a9=1;break;case 7:changed.matrices_valid=true;break;case 8:changed.matrices[0].at(0,0)=1;break;case 9:changed.history[1][2]=1;break;}
        const auto before=changed;check(!battle_initial_inactive_auxiliary_world(profile,0,{},changed,&error)&&changed==before);
    }
    std::printf("inactive C79F8 source channel/no-write/refusal profile: %u checks PASS\n",checks);
}
