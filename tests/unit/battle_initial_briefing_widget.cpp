#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_initial_briefing_widget.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,128> bytes{};
    const std::array<u32,32> words{
        0x47464249,1,128,0,0x80029434,0x80089898,0x80199c0c,0x80199c74,
        0x8019b030,0x8019b094,0x80006fb4,0x800086b8,0x80007d40,0x80007c30,
        0x8024ef10,0x8024ebc8,0x22e678b4,77059,27,4,3,18,16,17,
        57,58,0x12,180,179,0x3dcccccd,0x426c0000,0x803c7204};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleInitialBriefingWidgetProfile profile;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready());
    BattleInitialBriefingWidgetPrefix result;check(battle_initial_briefing_widget_prefix(profile,3,&result,&error));
    check(result.phase18==1&&result.render82==0x12&&result.timer1c==179&&!result.subphase19&&result.pitch72==0xa0&&result.yaw74==0x80&&result.rate7e==0xa0&&result.rate80==0x80);
    check(f32_to_bits(result.material_time44)==0x3dcccccd&&f32_to_bits(result.material_step4c)==0x3dcccccd&&result.applied&&result.awaiting_input&&!result.update_complete&&result.generation==1);
    const auto saved=result;
    for(unsigned argument=0;argument<256;++argument)if(argument!=3)check(!battle_initial_briefing_widget_prefix(profile,u8(argument),&result,&error)&&result==saved);
    for(unsigned at=0;at<bytes.size();++at){auto bad=bytes;bad[at]^=1;check(!profile.load({bad.data(),bad.size()},&error)&&!profile.ready()&&!battle_initial_briefing_widget_prefix(profile,3,&result,&error)&&result==saved);}
    for(unsigned n=0;n<bytes.size();++n)check(!profile.load({bytes.data(),n},&error));
    std::printf("first briefing type4 pre-input source prefix: %u checks PASS\n",checks);
}
