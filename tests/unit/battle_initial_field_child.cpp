#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/battle_initial_field_child.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,128> bytes{};
    const std::array<u32,32> words{0x47464649,1,128,0,0x80089a94,0x8008997c,0x800c6394,0x800660e8,0x800c63e8,0x800c6544,0x80046588,0x80045204,0x80089100,0x80007030,0x80007c54,0x80007cac,0x803052fc,0x0001000c,0,0x41f00000,0,0,0x0000ffff,0,0,0x0000ffff,0x3ecccccd,0x070b,0x1e,0x41,1,0};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleInitialFieldChildProfile profile;check(profile.load({bytes.data(),bytes.size()},&error));
    Mat4 bone=mat4_translation(std::array<f32,3>{10,20,30}.data());bone.at(0,0)=2;bone.at(1,1)=3;bone.at(2,2)=4;
    BattleInitialFieldChildNumbers out;const std::array<s16,3> angles{-32768,123,32767};
    check(battle_initial_field_child_numbers(profile,bone,angles,2,1,&out,&error));
    check(out.initial20==std::array<f32,3>{10,110,30}&&out.smoothed148==out.initial20&&out.position20==out.initial20&&out.angles70==angles&&out.scale58==std::array<f32,3>{2,2,2}&&!out.angle154);
    for(unsigned flag:{0u,2u,0x81u,0xffu})check(!battle_initial_field_child_numbers(profile,bone,angles,2,u8(flag),&out,&error));
    for(float scale:{0.0f,-1.0f,10001.0f,std::numeric_limits<float>::infinity()})check(!battle_initial_field_child_numbers(profile,bone,angles,scale,1,&out,&error));
    bone.m[15]=2;check(!battle_initial_field_child_numbers(profile,bone,angles,1,1,&out,&error));bone.m[15]=1;
    bone.m[0]=std::numeric_limits<float>::quiet_NaN();check(!battle_initial_field_child_numbers(profile,bone,angles,1,1,&out,&error));
    for(unsigned i=0;i<words.size();++i){auto bad=bytes;bad[4*i]^=1;BattleInitialFieldChildProfile p;check(!p.load({bad.data(),bad.size()},&error));}
    for(unsigned n=0;n<bytes.size();++n){BattleInitialFieldChildProfile p;check(!p.load({bytes.data(),n},&error));}
    std::printf("initial field child profile/numbers: %u checks PASS\n",checks);
}
