#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_initial_briefing_input.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,128> bytes{};
    const std::array<u32,32> words{
        0x47464249,1,128,0,0x80029434,0x80089898,0x80199c0c,0x80199c74,
        0x8019b030,0x8019b094,0x80006fb4,0x800086b8,0x80007d40,0x80007c30,
        0x8024ef10,0x8024ebc8,0x22e678b4,77059,27,4,3,18,16,17,
        57,58,0x12,180,179,0x3dcccccd,0x426c0000,0x803c7204};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleInitialBriefingWidgetProfile profile,bad;check(profile.load({bytes.data(),bytes.size()},&error));
    BattleInitialBriefingWidgetPrefix prefix;check(battle_initial_briefing_widget_prefix(profile,3,&prefix,&error));
    const auto before=prefix;const std::array<u32,8> bits{0x10,0x20,0x40,0x100,0x200,0x400,0x800,0x1000};
    for(unsigned subset=0;subset<256;++subset){
        u32 pressed=0;for(unsigned bit=0;bit<8;++bit)if(subset&(1u<<bit))pressed|=bits[bit];
        BattleInitialBriefingInputResult result;check(battle_initial_briefing_input(profile,prefix,pressed,&result,&error));
        auto expected=prefix;expected.subphase19=pressed?1:0;expected.awaiting_input=false;expected.update_complete=true;
        check(result.state==expected&&result.decrement_counter1653==bool(pressed)&&prefix==before);
        check(!battle_initial_briefing_input(profile,result.state,pressed,&result,&error));
    }
    BattleInitialBriefingInputResult result;check(!battle_initial_briefing_input(bad,prefix,0,&result,&error)&&!battle_initial_briefing_input(profile,prefix,1,&result,&error)&&!battle_initial_briefing_input(profile,prefix,0x10000,&result,&error)&&!battle_initial_briefing_input(profile,prefix,0,nullptr,&error));
    auto altered=prefix;--altered.timer1c;check(!battle_initial_briefing_input(profile,altered,0,&result,&error));
    std::printf("first briefing pressed-branch one-shot source arithmetic: %u checks PASS\n",checks);
}
