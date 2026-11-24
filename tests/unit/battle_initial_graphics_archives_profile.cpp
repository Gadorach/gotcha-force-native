#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_initial_graphics_archives.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,96> bytes{};
    const std::array<u32,24> words{0x47464741,1,96,0,0x800286d8,0x800425b8,0x80042954,0x80197a0c,0x80040eb4,0x80006c88,0x80008b90,0x80008c40,0x80245f20,0x80043414,0x80042300,0x8004be08,1,2708,0x2fbc80c9,2,0x6dd5d3b2,118,157,4};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleInitialGraphicsArchivesProfile profile;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready());
    for(unsigned at=0;at<bytes.size();at+=4){auto bad=bytes;bad[at]^=1;check(!profile.load({bad.data(),bad.size()},&error)&&!profile.ready());}
    for(unsigned n=0;n<bytes.size();++n)check(!profile.load({bytes.data(),n},&error));
    std::printf("source initial graphics archive fragment profile: %u checks PASS\n",checks);
}
