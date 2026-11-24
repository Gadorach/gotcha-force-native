#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_actor_first_update.hpp"
namespace {unsigned checks=0;void check(bool b){++checks;if(!b)std::abort();}}
int main(){
    using namespace gf;std::array<u8,64> raw{};const std::array<u32,16> words{0x47464655,1,64,0,0x80054070,0x8005685c,0x802d3264,0x800541ac,0x8008b480,0x8005a580,0x800465fc,0x80006284,0x41a00000,0,0,0};
    for(unsigned i=0;i<words.size();++i)store_be32(raw.data()+4*i,words[i]);
    std::string error;BattleActorFirstUpdateProfile profile;check(profile.load({raw.data(),raw.size()},&error)&&profile.ready()&&profile.bound_margin()==20);
    for(unsigned i=0;i<raw.size();++i){auto bad=raw;bad[i]^=1;check(!profile.load({bad.data(),bad.size()},&error)&&!profile.ready());}
    check(!profile.load({nullptr,64},&error)&&!profile.load({raw.data(),63},&error));
    std::printf("battle first update profile: %u checks PASS\n",checks);
}
