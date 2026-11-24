#pragma once
#include <algorithm>
#include "engine/assets/battle_initial_briefing_type2.hpp"
namespace gf {
struct BattleInitialBriefingType2Facts {
    u8 active=0,humans=0;std::array<s8,6> teams{};
    std::array<u8,2> counts{};std::array<std::array<s8,6>,2> slots{};
};
inline u8 briefing_source_team_ordinal(const BattleInitialBriefingType2Facts& facts,u8 slot){
    if(slot>=6||facts.teams[slot]<0||facts.teams[slot]>=2)return 0;
    const unsigned team=unsigned(facts.teams[slot]);
    for(unsigned i=0;i<std::min<unsigned>(6,facts.counts[team]);++i)if(facts.slots[team][i]==s8(slot))return u8(i);
    return 0;
}
struct BattleInitialBriefingType2State {
    u8 phase18=0,render82=0,flags84=0,slot96=0,variant12=0,team88=255,ordinal13=0;
    s16 angle70=0,timer1c=0;std::array<s16,3> records{-1,-1,0};
    f32 field38=0,field3c=0,field40=0,field44=0,rate4c=0;
    std::array<f32,3> values58_60{},target64_6c{};
    std::array<u16,2> random{};std::array<u8,2> selectors{};
    u64 generation=0;bool active=false,initialized=false;
    bool operator==(const BattleInitialBriefingType2State&)const=default;
};
inline bool battle_initial_briefing_type2(const BattleInitialBriefingType2Profile& profile,u8 argument,const BattleInitialBriefingType2Facts& facts,BattleInitialBriefingType2State* out,std::string* error){
    const std::array<s8,6> expected{1,0,1,0,-1,-1};
    if(!profile.ready()||!out||argument>=18||facts.active!=0x0f||facts.humans!=0x0a||facts.teams!=expected||facts.counts!=std::array<u8,2>{2,2}){if(error)*error="briefing type2: actual first-party source teams/order required";return false;}
    BattleInitialBriefingType2State next;next.slot96=argument%6;next.variant12=argument/6;next.team88=u8(facts.teams[next.slot96]);next.active=(facts.active&(1u<<next.slot96))!=0;next.generation=1;next.initialized=true;
    if(!next.active){next.phase18=2;*out=next;if(error)error->clear();return true;}
    next.phase18=1;next.render82=0x11;next.flags84=1;next.timer1c=15;
    //19BF18 returns0 if a slot is not found. Do not "repair" the fallback.
    next.ordinal13=briefing_source_team_ordinal(facts,next.slot96);
    if(next.variant12==0)next.records={14,14,0};
    else if(next.variant12==1)next.records[0]=(facts.humans&(1u<<next.slot96))?s16(next.slot96+4):s16(15);
    else next.records[0]=next.team88==1?23:22;
    next.field38=next.variant12==1?-70.f:-200.f;
    if(next.team88==1){next.field38=-next.field38;next.field44=-next.field44;}
    next.values58_60={next.field38,next.field3c,next.field40};next.rate4c=f32(next.field38/f32(next.timer1c));
    next.target64_6c=*profile.target(next.variant12,next.team88,next.ordinal13);
    *out=next;if(error){error->clear();}return true;
}
}
