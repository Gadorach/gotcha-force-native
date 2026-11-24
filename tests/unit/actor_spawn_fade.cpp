#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/actor_fade.hpp"
#include "tests/unit/actor_fade_fixture.hpp"
#include "engine/assets/battle_actor_spawn_entry.hpp"
namespace {unsigned checks=0;void check(bool b){++checks;if(!b)std::abort();}}
int main(){
    using namespace gf;std::string error;auto old=actor_fade_fixture::profile();ActorFadeProfile legacy;check(actor_fade_load({old.data(),old.size()},&legacy,&error));
    ActorFadeState initial,next;check(actor_fade_initialize(legacy,&initial)&&!actor_fade_request(legacy,initial,0,1,&next)&&!actor_fade_request(legacy,initial,255,1,&next));
    std::vector<u8> raw(old.begin(),old.end());raw.resize(108);store_be16(raw.data()+4,2);store_be16(raw.data()+6,3);store_be32(raw.data()+8,108);store_be32(raw.data()+88,0xffffffff);store_be_f32(raw.data()+96,80);store_be32(raw.data()+100,255);store_be32(raw.data()+104,0xf0000000);
    ActorFadeProfile profile;check(!actor_fade_load({raw.data(),raw.size()},&profile,&error));check(actor_fade_load_spawn({raw.data(),raw.size()},&profile,&error)&&profile.spawn_curve);
    check(actor_fade_initialize(profile,&initial)&&actor_fade_request(profile,initial,0,1,&next)&&next.channels[1].time==0&&next.channels[1].rgba==255&&actor_fade_update(profile,next,1,&next)&&next.channels[1].time==1&&next.material_rgba==0xffffffff&&next.channels[0].curve==255&&!next.channels[0].active);
    check(!actor_fade_request(profile,next,255,1,&initial));
    for(unsigned at=80;at<108;++at){auto bad=raw;bad[at]^=1;check(!actor_fade_load_spawn({bad.data(),bad.size()},&profile,&error)&&!profile.loaded);}
    std::array<u8,80> recipe{};const std::array<u32,20> words{0x47465356,1,80,0,0x80054070,0x8005475c,0x802d3274,0x800547dc,0x800061a8,0x80045204,0x800465fc,0x80006284,0x41a00000,0x3dcccccd,0x3f666666,0x46800000,0x41980000,0x3f800000,0x802c347c,0};
    for(unsigned i=0;i<words.size();++i){store_be32(recipe.data()+4*i,words[i]);}BattleActorSpawnEntryProfile spawn;check(spawn.load({recipe.data(),recipe.size()},&error));
    for(unsigned i=0;i<recipe.size();++i){auto bad=recipe;bad[i]^=1;check(!spawn.load({bad.data(),bad.size()},&error)&&!spawn.ready());}
    std::printf("actor spawn fade: %u checks PASS\n",checks);
}
