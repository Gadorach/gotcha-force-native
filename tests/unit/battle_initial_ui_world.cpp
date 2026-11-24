#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_initial_ui_world.hpp"
#include "engine/gameplay/battle_initial_post_world.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;const std::array<u32,44> words{0x47465557,1,176,0,0x80036e24,0x8008972c,0x80089658,0x80047aa4,0x80080f88,0x800818cc,0x8006ae60,0x800537a8,0x80029434,0x8002945c,0x80029488,0x7d2,1,1,13,38,8,0x20,0x21,1,0x11,0x19,0x64,0x58,0x74,0x38c90fdb,0x1e00,2,57,82,62,0x2fbc80c9,0x22e678b4,0x87d2f859,4,5,0x80053a94,0x80089898,59,0x8000bda4};
    std::array<u8,176> raw{};for(unsigned i=0;i<words.size();++i)store_be32(raw.data()+4*i,words[i]);
    BattleInitialUiWorldProfile profile,bad;check(profile.load({raw.data(),raw.size()},&error));
    for(unsigned i=0;i<raw.size();++i){auto edited=raw;edited[i]^=1;check(!bad.load({edited.data(),edited.size()},&error));check(!bad.load({raw.data(),i},&error));}
    for(unsigned flags=0;flags<256;++flags)for(unsigned phase=0;phase<4;++phase)check(battle_initial_hud_world_inactive(u8(flags),u8(phase))==(!(flags&1)||phase!=1));
    BattleInitialPostWorldState caller;caller.phase=BattleInitialPostWorldPhase::UiWorlds;check(caller.next_source()==0x80089658);caller.phase=BattleInitialPostWorldPhase::AwaitActorLate;check(caller.next_source()==0x80053a94);
    std::printf("initial UI source profile, HUD mask/phase refusal and caller boundary: %u checks PASS\n",checks);
}
