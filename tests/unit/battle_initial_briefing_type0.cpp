#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_initial_briefing_type0.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,128> raw{};
    const std::array<u32,32> words{
        0x47464230,1,128,0,0x80029434,0x80089898,0x80199c0c,0x80199c74,
        0x80199dc0,0x80199ea8,0x80006fb4,0x80007cac,0x800091e4,0x802437c8,
        0x80250298,0x80199d30,0x22e678b4,27,0,0,2,2,3,0x11,10,0,
        0xc2960000,0x3f800000,0xc3968000,0x3f000000,0xc3960000,0x3e4ccccd};
    for(unsigned i=0;i<words.size();++i)store_be32(raw.data()+4*i,words[i]);
    BattleInitialBriefingType0Profile profile,bad;check(profile.load({raw.data(),raw.size()},&error)&&profile.ready());
    BattleInitialBriefingType0State state;check(battle_initial_briefing_type0(profile,&state,&error));
    check(state.phase18==1&&!state.subphase19&&state.render82==0x11&&state.timer1e==10&&state.initialized&&state.generation==1);
    const std::array<f32,13> fields{0,0,-75,0,0,-301,1,1,1,0,0,1,0};
    check(state.fields144_174==fields&&f32_to_bits(state.rate3c)==0x3e4ccccd&&f32_to_bits(state.rate40)==0xc1b40000&&!state.rate38);
    const auto saved=state;check(!battle_initial_briefing_type0(bad,&state,&error)&&state==saved&&!battle_initial_briefing_type0(profile,nullptr,&error));
    for(unsigned at=0;at<raw.size();++at){auto bytes=raw;bytes[at]^=1;check(!bad.load({bytes.data(),bytes.size()},&error)&&!bad.ready()&&!battle_initial_briefing_type0(bad,&state,&error)&&state==saved);}
    for(unsigned n=0;n<raw.size();++n)check(!bad.load({raw.data(),n},&error));
    std::printf("fresh briefing type0 field/rate source initializer: %u checks PASS\n",checks);
}
