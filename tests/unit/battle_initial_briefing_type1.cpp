#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_initial_briefing_type1.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,128> raw{};
    const std::array<u32,32> words{
        0x47464231,1,128,0,0x8002821c,0x80195f2c,0x80195fbc,0x801962c4,
        0x80199c0c,0x80199c74,0x8019a108,0x80006fb4,0x800086b8,0x80007c30,
        0x80089100,0x80047aa4,0x22e678b4,27,1,0,1,0x20,4,0x11,15,1,0,
        0xc2ffb22d,0x42b40ee6,0xc3390000,0xc3960000,0xc0f55555};
    for(unsigned i=0;i<words.size();++i)store_be32(raw.data()+4*i,words[i]);
    BattleInitialBriefingType1Profile profile,bad;check(profile.load({raw.data(),raw.size()},&error));
    BattleInitialBriefingType1State state;check(battle_initial_briefing_type1(profile,1,0,0,&state,&error));
    check(state.phase18==1&&state.render82==0x11&&state.flags84==1&&state.timer1e==15&&state.digit_count13==1&&state.initialized&&state.generation==1);
    check(!state.digit_visible[0]&&state.digit_visible[1]&&state.selectors[0]==0&&state.selectors[1]==1&&f32_to_bits(state.rate40)==0xc0f55555);
    const auto saved=state;
    for(unsigned mode=0;mode<256;++mode)if(mode!=1)check(!battle_initial_briefing_type1(profile,u8(mode),0,0,&state,&error)&&state==saved);
    for(s16 wave:{s16(-1),s16(1),s16(98),s16(32767)})check(!battle_initial_briefing_type1(profile,1,0,wave,&state,&error)&&state==saved);
    check(!battle_initial_briefing_type1(profile,1,1,0,&state,&error)&&state==saved);
    for(unsigned at=0;at<raw.size();++at){auto bytes=raw;bytes[at]^=1;check(!bad.load({bytes.data(),bytes.size()},&error)&&!bad.ready());}
    for(unsigned n=0;n<raw.size();++n)check(!bad.load({raw.data(),n},&error));
    std::printf("fresh mode1/wave0 briefing digit initializer: %u checks PASS\n",checks);
}
