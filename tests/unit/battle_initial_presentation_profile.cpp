#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/assets/battle_initial_presentation.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,112> bytes{};
    const std::array<u32,28> words{0x47465052,1,112,0,0x800286d8,0x8002aaac,0x8008cec8,0x8008cbd4,0x8002acd0,0x8008c440,0x80005668,0x80009c4c,0x800058d0,0x80009cbc,0x8002ab70,0x8008c3f0,640,448,224,6,0,1,0,4,7,0,0x100000,0x8c000};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleInitialPresentationProfile profile;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready());
    for(unsigned i=0;i<words.size();++i){auto bad=bytes;bad[4*i]^=1;check(!profile.load({bad.data(),bad.size()},&error)&&!profile.ready());}
    for(unsigned n=0;n<bytes.size();++n)check(!profile.load({bytes.data(),n},&error));
    auto extra=std::vector<u8>(bytes.begin(),bytes.end());extra.push_back(0);check(!profile.load({extra.data(),extra.size()},&error));
    std::printf("initial presentation profile: %u checks PASS\n",checks);
}
