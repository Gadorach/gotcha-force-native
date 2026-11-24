#include <cstdio>
#include <cstdlib>
#include "tests/unit/briefing_type2_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;const auto bytes=briefing_type2_fixture::bytes();BattleInitialBriefingType2Profile profile,bad;check(profile.load({bytes.data(),bytes.size()},&error));
    const auto facts=briefing_type2_fixture::facts();unsigned active=0,inactive=0;
    for(unsigned argument=0;argument<18;++argument){
        BattleInitialBriefingType2State s;check(battle_initial_briefing_type2(profile,u8(argument),facts,&s,&error));
        check(s.slot96==argument%6&&s.variant12==argument/6&&s.initialized&&s.generation==1&&s.records[2]==0);
        if(argument%6>=4){++inactive;check(!s.active&&s.phase18==2&&!s.render82&&!s.timer1c&&s.records[0]==-1&&s.records[1]==-1&&s.team88==255);continue;}
        ++active;check(s.active&&s.phase18==1&&s.render82==0x11&&s.flags84==1&&s.timer1c==15);
        check(s.ordinal13==(s.slot96/2)&&s.team88==u8(facts.teams[s.slot96]));
        check(s.records[0]==(s.variant12==0?14:s.variant12==1?(s.slot96&1?s.slot96+4:15):s.team88==1?23:22));
        check(s.records[1]==(s.variant12? -1:14)&&f32_to_bits(s.field44)==(s.team88?0x80000000u:0u));
        check(s.values58_60[0]==s.field38&&s.rate4c==f32(s.field38/15.f));
    }
    check(active==12&&inactive==6);
    auto missing=facts;missing.slots[0].fill(-1);BattleInitialBriefingType2State fallback;check(battle_initial_briefing_type2(profile,3,missing,&fallback,&error)&&fallback.ordinal13==0);
    for(unsigned at=0;at<bytes.size();++at){auto edited=bytes;edited[at]^=1;check(!bad.load({edited.data(),edited.size()},&error)&&!bad.ready());}
    for(unsigned n=0;n<bytes.size();++n)check(!bad.load({bytes.data(),n},&error));
    for(unsigned argument=18;argument<256;++argument)check(!battle_initial_briefing_type2(profile,u8(argument),facts,&fallback,&error));
    std::printf("type2 actual18-node active/inactive/ordinal/source arithmetic: %u checks PASS\n",checks);
}
