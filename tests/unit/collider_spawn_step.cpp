#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/collider_spawn_step.hpp"
#include "tests/unit/collider_plane_fixture.hpp"
namespace {
unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"collider spawn step: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;std::array<u8,80> raw{};
    const std::array<u32,20> words{0x47464355,1,80,0,0x8008a28c,0x80089aec,0x8008a65c,0x80088e50,0x80089814,0x8003a5e4,0x8003b304,0x8003c614,0x8008aa40,0x8008a2bc,48,55,0,1,2,0};
    for(unsigned i=0;i<words.size();++i)store_be32(raw.data()+4*i,words[i]);
    ColliderSpawnSweepProfile recipe;check(recipe.load({raw.data(),raw.size()},&error),"source sweep recipe");
    std::array<u8,208> bytes{};const std::array<u32,12> initial{0x47464353,1,208,0,0x8003a4b8,0x8003a8fc,0x8003b170,0x8003c3d8,0x8008a65c,0x8008aa40,0x8008a2bc,0x80089aec};
    for(unsigned i=0;i<initial.size();++i)store_be32(bytes.data()+4*i,initial[i]);
    const auto geometry=collider_plane_fixture::profile();
    for(unsigned i=0;i<8;++i)store_be32(bytes.data()+48+4*i,geometry.words[i]);
    const std::array<float,24> corners{1,1,-1,1,1,1,-1,1,-1,-1,1,1,1,-1,-1,1,-1,1,-1,-1,-1,-1,-1,1};
    for(unsigned i=0;i<24;++i)store_be_f32(bytes.data()+80+4*i,corners[i]);
    const std::array<u8,24> faces{0,2,3,1,1,3,7,5,2,6,7,3,4,5,7,6,6,2,0,4,5,4,0,1};std::copy(faces.begin(),faces.end(),bytes.begin()+176);store_be32(bytes.data()+200,1);
    ColliderInitialShapeProfile profile;check(profile.load({bytes.data(),bytes.size()},&error),"shared initial geometry recipe");
    auto d=collider_plane_fixture::descriptor();d.shape=0;d.flags2=0x10;store_be_f32(d.source.data()+24,7);
    ColliderPlaneLifecycleOwnerFields owner;owner.lifecycle_declared=true;owner.registration={true,true,1,22,0,0,0,0,0};CollisionRegistrationPeerFields peer{true,true,22,0};
    Mat4 first,second;second.at(0,3)=3;second.at(1,3)=4;ColliderInitialPlan before,after;
    check(collider_initial_plan(profile,d,owner,peer,first,2,&before,&error)&&collider_spawn_step(recipe,profile,d,before,owner,peer,second,3,&after,&error),"first ordinary sphere update");
    check(after.fields.previous3c==before.fields.current30&&after.fields.previous_center70==before.fields.center64&&after.fields.center64==std::array<float,3>{3,9,0},"sphere tracks both lifecycle and shape center histories");
    check(after.fields.sphere.previous_radius8c==14&&after.fields.sphere.radius88==21&&after.fields.broad_radius7c==155,"sphere radius history and travel plus flag10 broad radius");
    d.shape=2;d.flags2=4;store_be_f32(d.source.data()+24,3);store_be_f32(d.source.data()+28,4);store_be_f32(d.source.data()+32,0);store_be_f32(d.source.data()+36,2);
    check(collider_initial_plan(profile,d,owner,peer,first,2,&before,&error)&&collider_spawn_step(recipe,profile,d,before,owner,peer,second,2,&after,&error),"ordinary capsule update");
    check(after.fields.broad_radius7c==361&&after.fields.capsule.radius88==4,"source3B304 squares travel+extent*C0+radius, not a corrected linear bound");
    check(after.fields.capsule.previous_endpoints==before.fields.capsule.endpoints,"capsule endpoint history retained");
    owner.freeze281=1;check(collider_initial_plan(profile,d,owner,peer,first,2,&before,&error)&&collider_spawn_step(recipe,profile,d,before,owner,peer,second,10,&after,&error),"frozen capsule hold");
    check(after.fields.current30==before.fields.current30&&after.fields.center64==before.fields.center64&&after.fields.broad_radius7c==before.fields.broad_radius7c&&after.fields.phase10==2,"hold does not rebuild shape at new owner matrix/scale");
    second.at(0,0)=.005f;check(collider_spawn_step(recipe,profile,d,before,owner,peer,second,2,&after,&error)&&after.degenerate&&!after.membership.lists,"hold still applies current matrix-column registration filter");
    d.flags2|=1;check(!collider_spawn_step(recipe,profile,d,before,owner,peer,second,2,&after,&error),"effect/swept branch remains unsupported");d.flags2&=u16(~1u);
    check(!collider_spawn_step(recipe,profile,d,before,owner,peer,second,2,&before,&error),"cannot alias prior and output");
    raw[19]^=1;check(!recipe.load({raw.data(),raw.size()},&error),"changed source dispatch refuses");
    std::printf("spawn collider ordinary step: %u checks PASS\n",checks);
}
