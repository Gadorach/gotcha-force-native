#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_initial_world_dispatch.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,160> raw{};const std::array<u32,40> words{0x47465749,1,160,0,0x80089554,0x80029434,0x80047aa4,0x80082060,0x8013c144,0x8001c5ac,0x80089658,0x8008972c,10,12,83,1,0,1,2,3,6,7,8,9,10,11,0x80436290,0x80436294,0x80436280,0x80436284,0x803c6f54,0x803c6f58,0x803c6f5c,0x803c6f60,0x803c6f64,0x803c6f68,4,5,0x54,0x1e};
    for(unsigned i=0;i<words.size();++i)store_be32(raw.data()+4*i,words[i]);
    BattleInitialWorldDispatchProfile profile;check(profile.load({raw.data(),raw.size()},&error));
    for(unsigned at=0;at<raw.size();++at){auto bad=raw;bad[at]^=1;check(!profile.load({bad.data(),bad.size()},&error)&&!profile.ready());}
    for(unsigned n=0;n<raw.size();++n)check(!profile.load({raw.data(),n},&error));
    check(BattleInitialWorldDispatchProfile::source_lists==std::array<u8,10>{0,1,2,3,6,7,8,9,10,11});
    std::printf("initial89554 ordered lists/UI exclusion profile: %u checks PASS\n",checks);
}
