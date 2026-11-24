#pragma once
#include "engine/assets/battle_initial_briefing_type5.hpp"
#include "engine/gameplay/battle_initial_briefing_type2.hpp"
namespace gf {
struct BattleInitialBriefingType5State {
    u8 phase18=0,render82=0,flags84=0,slot96=0,page12=0,team88=0,ordinal13=0,count1b=0;
    s16 timer1c=0;u8 member_begin=0,force_count=0;
    std::array<s16,5> records{-1,-1,-1,-1,-1};std::array<u16,5> models{};
    f32 field38=0,field44=0,rate4c=0;std::array<f32,3> target64_6c{},values58_60{};
    u64 generation=0;bool active=false,initialized=false;
    bool operator==(const BattleInitialBriefingType5State&)const=default;
};
inline bool battle_initial_briefing_type5(const BattleInitialBriefingType5Profile& profile,u8 argument,const BattleInitialBriefingType2Facts& facts,const std::array<u8,6>& counts,BattleInitialBriefingType5State* out,std::string* error){
    if(!profile.ready()||!out||argument>=36||facts.active!=15||facts.humans!=10||facts.teams!=std::array<s8,6>{1,0,1,0,-1,-1}||facts.counts!=std::array<u8,2>{2,2}){if(error)*error="briefing type5: actual first-party card source fields required";return false;}
    for(auto count:counts)if(count>30){if(error)*error="briefing type5: bounded original force count required";return false;}
    BattleInitialBriefingType5State next;next.slot96=argument/6;next.page12=argument;next.active=(facts.active&(1u<<next.slot96))!=0;next.generation=1;next.initialized=true;
    if(!next.active){next.phase18=2;*out=next;if(error)error->clear();return true;}
    next.phase18=1;next.render82=next.flags84=0x11;next.page12=argument%6;next.team88=u8(facts.teams[next.slot96]);next.ordinal13=briefing_source_team_ordinal(facts,next.slot96);next.timer1c=15;next.force_count=counts[next.slot96];next.member_begin=u8(next.page12*5);
    next.count1b=u8(std::min(5,std::max(0,int(next.force_count)-next.member_begin)));
    const f32 base=next.force_count<6?f32(f32(bits_to_f32(0x41a60000)*f32(std::max(0,int(next.force_count)-1)))*0.5f):bits_to_f32(0x42260000);
    next.field44=f32(f32(bits_to_f32(0xc197999a)*f32(next.page12))+base);
    next.field38=-200.f;next.rate4c=f32(next.field38/f32(next.timer1c));next.values58_60={bits_to_f32(0x3ecccccd),bits_to_f32(0x3ecccccd),1};
    next.target64_6c=*profile.target(next.team88,next.ordinal13);*out=next;if(error){error->clear();}return true;
}
}
