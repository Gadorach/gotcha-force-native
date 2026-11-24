#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_initial_bar_child.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,96> bytes{};
    const std::array<u32,24> words{0x4746424b,1,96,0,0x8013be88,0x8013bec4,0x80006dc8,0x80197a0c,0x8025afa4,0x80197ad8,0x80006f1c,0x80089100,0x8013c144,0x8013c264,0x80029434,0x80089898,0x615,3,5,0x24,180,192,1,3};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleInitialBarChildProfile profile;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready());
    for(unsigned at=0;at<bytes.size();at+=4){auto bad=bytes;bad[at]^=1;check(!profile.load({bad.data(),bad.size()},&error)&&!profile.ready());}
    for(unsigned n=0;n<bytes.size();++n)check(!profile.load({bytes.data(),n},&error));
    std::printf("initial count5 G-Red bar source profile: %u checks PASS\n",checks);
}
