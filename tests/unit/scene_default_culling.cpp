#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/scene_model_bindings.hpp"
#include "tests/unit/scene_archive_pool_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;hsd_record_animation_fixture::Fixture fixture;fixture.ptr(0x18,0xf80);fixture.word(0x38c,0xc0000003);const auto bytes=fixture.archive();HsdRecordAnimationBank bank;check(bank.load({bytes.data(),bytes.size()},&error,true));
    auto domain=scene_object_fixture::owner(8);SceneArchivePool archives;SceneArchiveLease archive;check(archives.begin_fresh(scene_archive_fixture::profile(),domain,&error)&&archives.acquire(&archive,&error)==SceneArchiveAcquire::Acquired&&archives.bind_borrowed_bank(archive,bank,&error));
    HsdJointInstanceBank roots;const std::array<u16,2> records{0,0};check(roots.construct_normal_records(bank,records,&error));std::array<SceneDescriptorLease,2> descriptors;
    for(unsigned i=0;i<2;++i){HsdJointInstanceOwner root;check(domain->acquire(&descriptors[i],&error)==SceneDescriptorAcquire::Acquired&&roots.take_instance(i,&root,&error)&&SceneModelBindings::attach_normal(*domain,descriptors[i],archive,0,std::move(root),&error));}
    const auto before=*domain->root(descriptors[0])->state();const auto pools=*domain->snapshot();check(SceneModelBindings::disable_fresh_default_culling(*domain,descriptors[0],&error));
    const auto& state=*domain->root(descriptors[0])->state();check(state.joints.size()==3&&!state.world_evaluated&&!state.animation_requested&&!state.side_style_applied&&!state.stage_shadow_applied);
    for(unsigned i=0;i<state.mesh_properties.size();++i){const auto& m=state.mesh_properties[i].second;const auto& a=before.mesh_properties[i].second;check(m.pobj_flags==u16(a.pobj_flags&~0xc000u)&&!m.cull_backface&&!m.cull_frontface&&m.texture==a.texture&&m.material_alpha==a.material_alpha&&m.render_mode==a.render_mode&&std::equal(std::begin(m.material_colour),std::end(m.material_colour),std::begin(a.material_colour)));check(domain->root(descriptors[1])->state()->mesh_properties[i].second.pobj_flags==a.pobj_flags);check(state.joints[i].joint.flags==before.joints[i].joint.flags&&!state.joints[i].world);}
    check(*domain->snapshot()==pools);check(domain->release(descriptors[0],&error)&&!SceneModelBindings::disable_fresh_default_culling(*domain,descriptors[0],&error));
    std::printf("default hierarchical PObj cull-only mutation: %u checks PASS\n",checks);
}
