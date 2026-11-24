#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/battle_initial_point_child.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::array<u8,104> bytes{};std::string error;
    const std::array<u32,26> words{0x47464950,1,104,0,0x80089a94,0x8008997c,0x801fcda8,0x800660e8,0x801fcdfc,0x801fcefc,0x80046588,0x80089100,0x80007030,0x80007c54,0x80007cac,0x80047aa4,0x803a2f88,0x01010308,0x3f800000,0x3ecccccd,0x3e800000,0x070b,0x54,0x31,1,0};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleInitialPointChildProfile profile;check(profile.load({bytes.data(),bytes.size()},&error));
    BattleInitialPointChildNumbers out;check(battle_initial_point_child_numbers(profile,{4,50,-8},{1,-2,3},2,123,1,&out,&error));
    check(out.smoothed14c==std::array<f32,3>{1,12.5f,-2}&&out.position20==std::array<f32,3>{2,10.5f,1}&&out.scale58==std::array<f32,3>{1,1,1}&&out.scale_b4==2&&out.scale_b8==2&&out.angles70==std::array<s16,3>{0,123,0});
    for(unsigned flag:{0u,2u,0x81u,0xffu})check(!battle_initial_point_child_numbers(profile,{},{},1,0,u8(flag),&out,&error));
    for(float scale:{0.0f,-1.0f,10001.0f,std::numeric_limits<float>::infinity()})check(!battle_initial_point_child_numbers(profile,{},{},scale,0,1,&out,&error));
    check(!battle_initial_point_child_numbers(profile,{0,std::numeric_limits<float>::quiet_NaN(),0},{},1,0,1,&out,&error));
    check(!battle_initial_point_child_numbers(profile,{},{10001,0,0},1,0,1,&out,&error));
    for(unsigned i=0;i<words.size();++i){auto bad=bytes;bad[4*i]^=1;BattleInitialPointChildProfile p;check(!p.load({bad.data(),bad.size()},&error));}
    for(unsigned n=0;n<bytes.size();++n){BattleInitialPointChildProfile p;check(!p.load({bytes.data(),n},&error));}
    std::printf("initial point child profile/numbers: %u checks PASS\n",checks);
}
