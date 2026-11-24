#include <cstdio>
#include <cstdlib>
#include <array>
#include "engine/assets/battle_actor_initial_controls.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"initial controls: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::array<u8,32> raw{};store_be32(raw.data(),0x47464943);store_be32(raw.data()+4,1);store_be32(raw.data()+8,32);store_be32(raw.data()+12,123);store_be32(raw.data()+16,0x802d3ff8);store_be16(raw.data()+20,10);raw[24]=raw[25]=1;
    std::string error;BattleActorInitialControlsProfile profile;
    check(profile.load({raw.data(),raw.size()},&error)&&profile.matches(10,123,0x802d3ff8)&&profile.primary()==1&&profile.secondary()==1,"qualified selector0 row");
    check(!profile.matches(0x615,123,0x802d3ff8)&&!profile.matches(10,124,0x802d3ff8)&&!profile.matches(10,123,0x80365cf8),"model/hash/source key all bind");
    for(unsigned i=0;i<raw.size();++i){
        auto bad=raw;bad[i]^=1;const bool loaded=profile.load({bad.data(),bad.size()},&error);
        if(i>=12&&i<16)check(!loaded||!profile.matches(10,123,0x802d3ff8),"changed hash cannot bind original owner");
        else check(!loaded&&!profile.matches(10,123,0x802d3ff8)&&profile.primary()==0,"malformed/unqualified row clears old profile");
    }
    check(!profile.load({nullptr,32},&error)&&!profile.load({raw.data(),31},&error),"null/truncated input refuses");
    std::printf("battle initial controls: %u checks PASS\n",checks);
}
