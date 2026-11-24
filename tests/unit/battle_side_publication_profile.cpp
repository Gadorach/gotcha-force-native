#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_side_publication.hpp"
namespace {unsigned checks=0;void check(bool b){++checks;if(!b)std::abort();}}
int main(){
 using namespace gf;std::array<u8,80> bytes{};std::string error;
 const std::array<u32,20> words{0x47465350,1,80,0,0x80089eb4,0x800820c8,0x800076d0,0x80007cac,0x800097b4,0x8008b290,0x3f800000,0x3f000000,0x3e800000,0x3bf5c28f,7,0x0f,0x47,1,0xff,0};
 for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
 BattleSidePublicationProfile p;check(p.load({bytes.data(),bytes.size()},&error)&&p.ready());
 for(unsigned i=0;i<words.size();++i){auto bad=bytes;bad[4*i]^=1;check(!p.load({bad.data(),bad.size()},&error)&&!p.ready());}
 check(!p.load({},&error));std::printf("side callback source profile: %u checks PASS\n",checks);
}
