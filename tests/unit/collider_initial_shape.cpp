#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/collider_initial_shape.hpp"
#include "tests/unit/collider_plane_fixture.hpp"
namespace {
unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"collider initial shape: %s\n",why);std::abort();}}
gf::ColliderInitialShapeProfile profile(){
    using namespace gf;std::array<u8,208> bytes{};
    const std::array<u32,12> keys{0x47464353,1,208,0,0x8003a4b8,0x8003a8fc,0x8003b170,0x8003c3d8,0x8008a65c,0x8008aa40,0x8008a2bc,0x80089aec};
    for(unsigned i=0;i<keys.size();++i)store_be32(bytes.data()+4*i,keys[i]);
    const auto p=collider_plane_fixture::profile();for(unsigned i=0;i<8;++i)store_be32(bytes.data()+48+4*i,p.words[i]);
    const std::array<f32,24> corners{1,1,-1,1,1,1,-1,1,-1,-1,1,1,1,-1,-1,1,-1,1,-1,-1,-1,-1,-1,1};
    for(unsigned i=0;i<24;++i)store_be_f32(bytes.data()+80+4*i,corners[i]);
    const std::array<u8,24> faces{0,2,3,1,1,3,7,5,2,6,7,3,4,5,7,6,6,2,0,4,5,4,0,1};std::copy(faces.begin(),faces.end(),bytes.begin()+176);store_be32(bytes.data()+200,1);
    ColliderInitialShapeProfile out;std::string error;if(!out.load({bytes.data(),bytes.size()},&error))std::abort();return out;
}
}
int main(){
    using namespace gf;auto p=profile();std::string error;ColliderPlaneLifecycleOwnerFields owner;owner.lifecycle_declared=true;owner.registration={true,true,1,22,0,0,0,0,0};
    CollisionRegistrationPeerFields peer{true,true,22,0};Mat4 frame;frame.at(0,3)=100;frame.at(1,3)=20;frame.at(2,3)=-50;
    auto d=collider_plane_fixture::descriptor();d.shape=0;d.flags2=0x14;d.local_point={2,3,4};store_be_f32(d.source.data()+24,7);
    ColliderInitialPlan plan;check(collider_initial_plan(p,d,owner,peer,frame,2,&plan,&error),error.c_str());
    check(plan.fields.center64==std::array<f32,3>{102,23,-46}&&plan.fields.current30==plan.fields.previous3c&&plan.fields.center64==plan.fields.previous_center70,"sphere source center and history");
    check(plan.fields.sphere.radius88==14&&plan.fields.sphere.previous_radius8c==14&&plan.fields.broad_radius7c==100,"flag10 broad radius uses50*C0 without changing sphere radius");
    check(plan.membership.contains(CollisionRegistrationList::L3B7C)&&plan.membership.contains(CollisionRegistrationList::L297C),"shared registration mask uses source descriptor");
    d.shape=2;d.flags2=4;store_be_f32(d.source.data()+24,3);store_be_f32(d.source.data()+28,4);store_be_f32(d.source.data()+32,0);store_be_f32(d.source.data()+36,2);
    check(collider_initial_plan(p,d,owner,peer,frame,2,&plan,&error)&&plan.fields.broad_radius7c==14&&plan.fields.capsule.radius88==4,"capsule radius uses descriptor extent length and C0");
    check(plan.fields.capsule.endpoints[0]==std::array<f32,3>{99,19,-46}&&plan.fields.capsule.endpoints[1]==std::array<f32,3>{105,27,-46},"capsule endpoints use actual matrix, not scalar radius scale");
    for(unsigned at:{24u,28u,32u})store_be_f32(d.source.data()+at,0);
    check(collider_initial_plan(p,d,owner,peer,frame,2,&plan,&error)&&!plan.fields.conditional_complete&&plan.fields.capsule.radius88==0&&plan.fields.broad_radius7c==0&&plan.membership.lists,"zero capsule direction retains cleared conditional fields; no invented sphere fallback");
    d.shape=1;for(unsigned at:{24u,28u,32u})store_be_f32(d.source.data()+at,1);
    check(collider_initial_plan(p,d,owner,peer,frame,100,&plan,&error)&&plan.fields.box.normals_written==6&&plan.fields.conditional_complete&&std::abs(plan.fields.broad_radius7c-std::sqrt(3.f))<1e-6f,"box uses transformed corners, not owner C0 scaling");
    check(plan.fields.box.normals[0]==std::array<f32,3>{0,1,0}&&plan.fields.box.previous_normals==plan.fields.box.normals,"source box face ordering and history");
    store_be_f32(d.source.data()+24,0);check(collider_initial_plan(p,d,owner,peer,frame,1,&plan,&error)&&!plan.fields.conditional_complete&&plan.fields.box.normals_written==1&&plan.fields.previous_center70==std::array<f32,3>{},"box first-face degeneracy returns before history/radius suffix");
    d=collider_plane_fixture::descriptor();owner.freeze281=1;check(collider_initial_plan(p,d,owner,peer,frame,1,&plan,&error)&&plan.fields.phase10==2&&plan.fields.plane.initialized,"type4 reuses shared plane initialization and source freeze");
    frame.at(0,0)=.005f;check(collider_initial_plan(p,d,owner,peer,frame,1,&plan,&error)&&plan.degenerate&&!plan.membership.lists&&plan.fields.plane.initialized,"column-length rejection occurs after shape initialization");
    d.flags2|=1;check(!collider_initial_plan(p,d,owner,peer,frame,1,&plan,&error),"parameter-effect branch remains closed");d.flags2&=u16(~1u);
    d.start6=1;check(!collider_initial_plan(p,d,owner,peer,frame,1,&plan,&error),"unjoined delayed initial clock");d.start6=0;d.shape=3;check(!collider_initial_plan(p,d,owner,peer,frame,1,&plan,&error),"unjoined shape3 initializer");
    std::printf("collider initial shapes: %u checks PASS\n",checks);
}
