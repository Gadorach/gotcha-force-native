#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_initial_shape_child.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,116> bytes{};
    const std::array<u32,29> words{0x47465353,1,116,0,0x8001c42c,0x8001c468,0x8001c524,0x80089100,0x80008a94,0x80245f20,0x80040e94,0x802442dc,0x80245a8c,0x80245acc,0x8020bab8,0x8020bb14,0x070b,0xbdb1d4f4,0xbdf8e542,2,0,0,19,0x3f800000,0x3a83126f,0x38100000,0,0x10,1};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleInitialShapeChildProfile profile;check(profile.load({bytes.data(),bytes.size()},&error));
    HsdInitialLightConstructor lights;lights.generation=1;BattleInitialShapeNumbers out;
    check(battle_initial_shape_numbers(profile,lights,&out,&error)&&out.direction9c==std::array<f32,3>{0,0,1}&&!out.light_direction_read&&!out.selected_light);
    lights.nodes.resize(1);lights.nodes[0].light.flags=13;lights.nodes[0].light.position[0]=3;lights.nodes[0].light.position[1]=4;lights.primary_infinite=0;
    check(battle_initial_shape_numbers(profile,lights,&out,&error)&&out.light_direction_read&&out.selected_light==0&&!out.normalization_fallback&&std::abs(out.direction9c[0]+.6f)<1e-6f&&std::abs(out.direction9c[1]+.8f)<1e-6f&&out.visibility144==1);
    for(unsigned k=0;k<3;++k)lights.nodes[0].light.interest[k]=lights.nodes[0].light.position[k];
    check(battle_initial_shape_numbers(profile,lights,&out,&error)&&out.normalization_fallback&&out.direction9c==std::array<f32,3>{0,0,1});
    lights.nodes[0].light.position[0]=1e-30f;lights.nodes[0].light.position[1]=0;std::fill_n(lights.nodes[0].light.interest,3,0.0f);
    check(!battle_initial_shape_numbers(profile,lights,&out,&error)); //nonzero underflow is not repaired to Z
    lights.nodes[0].light.position[0]=3;lights.primary_infinite=1;check(!battle_initial_shape_numbers(profile,lights,&out,&error));lights.primary_infinite=0;
    lights.nodes[0].light.flags=4;check(!battle_initial_shape_numbers(profile,lights,&out,&error));lights.nodes[0].light.flags=13;
    lights.animation_requested=true;check(!battle_initial_shape_numbers(profile,lights,&out,&error));lights.animation_requested=false;lights.generation=2;check(!battle_initial_shape_numbers(profile,lights,&out,&error));
    for(unsigned at=0;at<bytes.size();at+=4){auto bad=bytes;bad[at]^=1;BattleInitialShapeChildProfile p;check(!p.load({bad.data(),bad.size()},&error)&&!p.ready());}
    for(unsigned n=0;n<bytes.size();++n){BattleInitialShapeChildProfile p;check(!p.load({bytes.data(),n},&error));}
    std::printf("initial bone19 shape source profile/light math: %u checks PASS\n",checks);
}
