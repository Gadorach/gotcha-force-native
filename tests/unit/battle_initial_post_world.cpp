#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_initial_post_world.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;const std::array<u32,32> words{0x47465043,1,128,0,0x80029434,0x800294c8,0x80089554,0x800294cc,0x800294d0,0x800294d4,0x800294d8,0x800294e0,0xb0,0x66,0,1,0x8008a28c,0x80089aec,0x8002bb14,0x80036e24,0x8008972c,0x80089658,0x808d8394,0x806400b0,0x38030001,0x900400b0,0x88030066,0,2,3,4,5};
    std::array<u8,128> raw{};for(unsigned i=0;i<words.size();++i)store_be32(raw.data()+4*i,words[i]);
    BattleInitialPostWorldProfile profile,bad;check(profile.load({raw.data(),raw.size()},&error));
    for(unsigned i=0;i<raw.size();++i){auto edited=raw;edited[i]^=1;check(!bad.load({edited.data(),edited.size()},&error));check(!bad.load({raw.data(),i},&error));}
    u32 out=99;check(!battle_initial_post_world_counter(bad,0,&out,&error)&&out==99);check(!battle_initial_post_world_counter(profile,0,nullptr,&error));
    for(u32 value:{0u,1u,0x7ffffffeu,0x7fffffffu,0x80000000u,0xfffffffeu,0xffffffffu}){check(battle_initial_post_world_counter(profile,value,&out,&error));check(u64(out)==((u64(value)+1)&0xffffffffu));}
    u32 value=0x32740befu;for(unsigned i=0;i<65536;++i){value=u32(u64(value)*1664525u+1013904223u);check(battle_initial_post_world_counter(profile,value,&out,&error)&&u64(out)==((u64(value)+1)&0xffffffffu));}
    BattleInitialPostWorldState state;const std::array<u32,4> next{0x8008a28c,0x8002bb14,0x8002d35c,0x80036e24};
    for(unsigned i=0;i<4;++i){state.phase=BattleInitialPostWorldPhase(i);check(state.next_source()==next[i]);}
    std::printf("first29434 counter/source profile/unsigned wrap and caller phases: %u checks PASS\n",checks);
}
