#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_initial_hud_children.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,128> bytes{};
    const std::array<u32,32> words{
        0x47464849,1,128,0,0x800813a8,0x80081438,0x80081478,0x800818cc,
        0x800807e4,0x80080874,0x800808b4,0x80080f88,0x80089100,0x80006fb4,
        0x800086b8,0x80007d40,0x80007c30,0x8024ef10,0x8024ebc8,0x80007cd0,
        2708,0x2fbc80c9,118,56,81,50,0x21,5,1,6,0xff00,0xff0000};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleInitialHudChildrenProfile profile;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready());
    for(unsigned at=0;at<bytes.size();++at){auto bad=bytes;bad[at]^=1;check(!profile.load({bad.data(),bad.size()},&error)&&!profile.ready());}
    for(unsigned n=0;n<bytes.size();++n)check(!profile.load({bytes.data(),n},&error));
    std::printf("initial HUD source profile: %u checks PASS\n",checks);
}
