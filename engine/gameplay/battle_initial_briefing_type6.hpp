#pragma once
#include "engine/assets/battle_initial_briefing_type6.hpp"
namespace gf {
struct BattleInitialBriefingType6State {
    u8 phase18=0,render82=0,flags84=0;s16 angle74=0,rate80=0,timer1c=0,timer1e=0;
    std::array<f32,3> values58_60{},target64_6c{};u16 record=0;
    u64 generation=0;bool initialized=false;
    bool operator==(const BattleInitialBriefingType6State&)const=default;
};
inline bool battle_initial_briefing_type6(const BattleInitialBriefingType6Profile& profile,u8 argument,BattleInitialBriefingType6State* out,std::string* error){
    if(!profile.ready()||argument||!out){if(error)*error="briefing type6: original fresh argument0 required; random branch unjoined";return false;}
    BattleInitialBriefingType6State next;next.phase18=1;next.flags84=0x19;next.render82=0x12;next.rate80=0x444;next.timer1e=8;next.record=21;
    next.values58_60={bits_to_f32(0x3f4ccccd),bits_to_f32(0x3f4ccccd),1};next.target64_6c={0,0,-400};next.generation=1;next.initialized=true;*out=next;if(error){error->clear();}return true;
}
}
