#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_model_publication.hpp"
namespace {unsigned checks=0;void check(bool b){++checks;if(!b)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,80> bytes{};const std::array<u32,20> words{0x47464250,1,80,0,0x8008a16c,0x80053928,0x8008c098,0x8008c0dc,0x8008a0a8,0x80009790,0x80056aa8,0x800097b4,0x8004fcfc,0x8004fdf0,0x8004fe60,0x800076d0,0x47,0,0,0};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleModelPublicationProfile profile;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready());
    for(unsigned i=0;i<bytes.size();++i){auto bad=bytes;bad[i]^=1;check(!profile.load({bad.data(),bad.size()},&error)&&!profile.ready());}
    check(!profile.load({nullptr,80},&error)&&!profile.load({bytes.data(),79},&error));
    std::printf("battle model publication profile: %u checks PASS\n",checks);
}
