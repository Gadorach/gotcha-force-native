#pragma once
#include <algorithm>
#include "engine/assets/battle_initial_briefing_type1.hpp"
namespace gf {
struct BattleInitialBriefingType1State {
    u8 phase18=0,render82=0,flags84=0,digit_count13=0;s16 timer1e=0;
    std::array<bool,2> digit_visible{};std::array<f32,2> selectors{};
    std::array<f32,3> target64_6c{};f32 rate40=0;
    u64 generation=0;bool initialized=false;
    bool operator==(const BattleInitialBriefingType1State&)const=default;
};
inline bool battle_initial_briefing_type1(const BattleInitialBriefingType1Profile& profile,u8 common32,u8 common64,s16 wave46,BattleInitialBriefingType1State* out,std::string* error){
    if(!profile.ready()||!out||common32!=1||common64||wave46!=0){if(error)*error="briefing type1: owned normal Challenge mode1/first wave0 required";return false;}
    BattleInitialBriefingType1State next;next.phase18=1;next.render82=0x11;next.flags84=1;next.timer1e=15;
    const int number=std::min(int(wave46)+1,99);bool leading=false;
    for(unsigned index=0,divisor=10;divisor;divisor/=10,++index){
        const int digit=(number/int(divisor))%10;
        if(digit||divisor==1)leading=true;
        if(leading){next.digit_visible[index]=true;next.selectors[index]=f32(digit);++next.digit_count13;}
    }
    next.target64_6c={bits_to_f32(0xc2ffb22d),bits_to_f32(0x42b40ee6),bits_to_f32(0xc3390000)};
    next.rate40=f32(f32(bits_to_f32(0xc3960000)-next.target64_6c[2])/f32(next.timer1e));
    next.generation=1;next.initialized=true;*out=next;if(error){error->clear();}return true;
}
}
