#pragma once
#include "engine/assets/battle_initial_briefing_type0.hpp"
namespace gf {
struct BattleInitialBriefingType0State {
    u8 phase18=0,subphase19=0,render82=0;s16 timer1e=0;
    std::array<f32,13> fields144_174{};
    f32 rate38=0,rate3c=0,rate40=0;
    u64 generation=0;bool initialized=false;
    bool operator==(const BattleInitialBriefingType0State&)const=default;
};
inline bool battle_initial_briefing_type0(const BattleInitialBriefingType0Profile& profile,BattleInitialBriefingType0State* out,std::string* error){
    if(!profile.ready()||!out){if(error)*error="briefing type0: supported fresh initializer/output required";return false;}
    BattleInitialBriefingType0State next;next.phase18=1;next.render82=0x11;next.timer1e=10;
    next.fields144_174={0,0,-75,0,0,-301,1,1,1,0,0,1,0};
    //Keep the source's separately rounded float product/subtraction/divisions.
    //The signed-integer-to-double conversion is exact for this fresh timer10.
    const f32 timer=f32(next.timer1e);
    next.rate3c=f32(1.f/f32(timer*0.5f));
    next.rate40=f32(f32(-300.f-next.fields144_174[2])/timer);
    next.generation=1;next.initialized=true;*out=next;
    if(error){error->clear();}return true;
}
}
