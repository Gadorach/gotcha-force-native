#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/scene_model_bindings.hpp"
#include "tests/unit/scene_archive_pool_fixture.hpp"
#include "tests/unit/hsd_record_texture_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"material selector: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;auto f=hsd_record_texture_fixture::fixture();
    auto clear=[&](unsigned at){f.word(at,0);std::erase(f.fields,at);};
    clear(0x108);clear(0x800);clear(0x34);clear(0x904);f.ptr(0x18,0xf80);
    const auto raw=f.archive();HsdRecordAnimationBank bank;check(bank.load({raw.data(),raw.size()},&error,true),"owned texture-only single-joint record");
    auto domain=scene_object_fixture::owner(8);SceneArchivePool archives;SceneArchiveLease archive;
    check(archives.begin_fresh(scene_archive_fixture::profile(),domain,&error)&&archives.acquire(&archive,&error)==SceneArchiveAcquire::Acquired&&archives.bind_borrowed_bank(archive,bank,&error),"same archive/domain");
    HsdJointInstanceBank prepared;const std::array<u16,3> records{0,0,0};check(prepared.construct_normal_records(bank,records,&error),"three independent roots");
    std::array<SceneDescriptorLease,3> descriptors;
    for(unsigned i=0;i<3;++i){HsdJointInstanceOwner root;check(domain->acquire(&descriptors[i],&error)==SceneDescriptorAcquire::Acquired&&prepared.take_instance(i,&root,&error)&&SceneModelBindings::attach_normal(*domain,descriptors[i],archive,0,std::move(root),&error),"actual descriptor root");}
    const auto hidden_identity=domain->root(descriptors[0])->receipt();const auto root_flags=domain->root(descriptors[0])->state()->joints[0].joint.flags;
    check(SceneModelBindings::set_fresh_descriptor_visibility(*domain,descriptors[0],0,&error),"hide the bound tens descriptor only");
    check(hidden_identity.current()&&domain->root(descriptors[0])->state()->joints[0].joint.flags==root_flags&&!domain->root(descriptors[0])->state()->animation_requested&&domain->inspect(descriptors[0])->native_root0c&&!domain->inspect(descriptors[0])->fields.payload10_1f[0],"hidden is not unbound, animated or a hidden JObj");
    const auto before=*domain->snapshot();SceneMaterialAnimationOwner invalid;
    for(f32 selector:{-.1f,.5f,10.f,std::numeric_limits<f32>::quiet_NaN()})check(!SceneModelBindings::request_fresh_material_texture_selector(*domain,descriptors[0],archive,selector,&invalid,&error)&&!invalid.current()&&*domain->snapshot()==before,"unsupported selector refuses without root/descriptor effects");
    check(!SceneModelBindings::set_fresh_descriptor_visibility(*domain,descriptors[0],2,&error)&&*domain->snapshot()==before,"visibility is exactly0/1");
    SceneMaterialAnimationOwner zero,one;
    check(SceneModelBindings::request_fresh_material_texture_zero(*domain,descriptors[1],archive,&zero,&error),"legacy zero entry still requests time0");
    check(SceneModelBindings::request_fresh_material_texture_selector(*domain,descriptors[2],archive,1,&one,&error),"explicit selector requests the same shared player at time1");
    check(zero.trace()[0].frame==0&&one.trace()[0].frame==1&&zero.trace()[0].tracks[0].callbacks==1&&one.trace()[0].tracks[0].callbacks==1,"one initial callback each, not replay/ticks");
    check(zero.pose()->materials[0].uv_offset[0]==.25f&&one.pose()->materials[0].uv_offset[0]==.3125f,"real nonzero selector sample differs from time0");
    check(zero.same_root(*domain->root(descriptors[1]))&&one.same_root(*domain->root(descriptors[2]))&&!one.same_root(*domain->root(descriptors[0])),"runtime remains attached to exact independent roots");
    check(!SceneModelBindings::set_fresh_descriptor_visibility(*domain,descriptors[2],0,&error)&&domain->inspect(descriptors[2])->fields.payload10_1f[0]==1,"fresh visibility helper refuses requested roots");
    for(unsigned i=0;i<3;++i)check(!domain->root(descriptors[i])->world_current()&&!domain->root(descriptors[i])->state()->joints[0].world,"no world matrix or renderer authority");
    archives.retire();check(!zero.current()&&!one.current()&&!zero.pose()&&!one.pose(),"stale source closes both selector owners");
    check(domain->release(descriptors[0],&error)&&!hidden_identity.current(),"unrequested hidden root keeps ordinary sole-metadata cleanup");
    std::printf("descriptor visibility and retained integer material selectors: %u checks PASS\n",checks);
}
