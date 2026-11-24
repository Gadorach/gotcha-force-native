#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/scene_model_bindings.hpp"
#include "tests/unit/scene_archive_pool_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"default alpha: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;const auto bytes=scene_archive_fixture::bank();HsdRecordAnimationBank bank;
    check(bank.load({bytes.data(),bytes.size()},&error,true),"owned hierarchical bank");
    for(unsigned contextual=0;contextual<2;++contextual){
        auto domain=scene_object_fixture::owner(8);SceneArchivePool archives;SceneArchiveLease archive;
        check(archives.begin_fresh(scene_archive_fixture::profile(),domain,&error)&&archives.acquire(&archive,&error)==SceneArchiveAcquire::Acquired&&archives.bind_borrowed_bank(archive,bank,&error),"same archive/domain");
        HsdJointInstanceBank prepared;const std::array<u16,2> records{0,0};check(prepared.construct_normal_records(bank,records,&error),"independent complete default hierarchies");
        std::array<SceneDescriptorLease,2> descriptors;auto context=std::make_shared<SceneModelMaterialContext>();
        for(unsigned i=0;i<2;++i){
            HsdJointInstanceOwner root;check(domain->acquire(&descriptors[i],&error)==SceneDescriptorAcquire::Acquired&&prepared.take_instance(i,&root,&error),"actual descriptor/root");
            check(contextual?SceneModelBindings::attach_actor(*domain,descriptors[i],archive,0,HsdJointInstanceStyle::Normal,std::move(root),context,&error):SceneModelBindings::attach_normal(*domain,descriptors[i],archive,0,std::move(root),&error),"normal contextual/default binding");
        }
        const auto before=*domain->root(descriptors[0])->state();const auto other=*domain->root(descriptors[1])->state();const auto pool=*domain->snapshot();
        for(f32 alpha:{0.f,-0.f,.4f,1.f}){
            check(SceneModelBindings::set_fresh_material_alpha(*domain,descriptors[0],alpha,&error)==!contextual,"only fresh normal DEFAULT material ownership admitted");
            const auto& state=*domain->root(descriptors[0])->state();
            check(state.joints.size()==3&&state.dobjs.size()==3&&state.mesh_properties.size()==3&&!state.animation_requested&&!state.world_evaluated&&!state.side_style_applied&&!state.stage_shadow_applied&&!state.child_translucent_applied,"complete hierarchy retained without styling/animation/world");
            for(unsigned i=0;i<state.dobjs.size();++i){
                check(f32_to_bits(state.dobjs[i].material.alpha)==f32_to_bits(contextual?before.dobjs[i].material.alpha:alpha)&&state.dobjs[i].flags==before.dobjs[i].flags&&state.dobjs[i].material.render_mode==before.dobjs[i].material.render_mode,"root AND child DObj alpha only");
                const auto& mesh=state.mesh_properties[i].second;const auto& original=before.mesh_properties[i].second;
                check(f32_to_bits(mesh.material_alpha)==f32_to_bits(contextual?original.material_alpha:alpha)&&mesh.render_mode==original.render_mode&&mesh.texture==original.texture&&mesh.blend==original.blend&&mesh.depth_write==original.depth_write&&mesh.pobj_flags==original.pobj_flags&&std::equal(std::begin(mesh.material_colour),std::end(mesh.material_colour),std::begin(original.material_colour)),"preserve authored diffuse alpha, RGB, textures, category and render/geometry flags");
                check(state.joints[i].joint.flags==before.joints[i].joint.flags&&!state.joints[i].world,"no dirty flag or evaluated matrix");
                check(domain->root(descriptors[1])->state()->dobjs[i].material.alpha==other.dobjs[i].material.alpha,"other instance remains independent");
            }
            check(*domain->snapshot()==pool,"no descriptor fields/links/allocation mutation");
        }
        const auto alpha=domain->root(descriptors[0])->state()->dobjs[0].material.alpha;
        for(f32 unsupported:{-.1f,1.1f,std::numeric_limits<f32>::quiet_NaN(),std::numeric_limits<f32>::infinity()}){
            check(!SceneModelBindings::set_fresh_material_alpha(*domain,descriptors[0],unsupported,&error)&&f32_to_bits(domain->root(descriptors[0])->state()->dobjs[0].material.alpha)==f32_to_bits(alpha),"unsupported scalar refuses without partial alpha writes");
        }
        auto foreign=scene_object_fixture::owner(4);check(!SceneModelBindings::set_fresh_material_alpha(*foreign,descriptors[0],0,&error),"foreign descriptor domain refuses");
        const auto receipt=domain->root(descriptors[0])->receipt();check(domain->release(descriptors[0],&error)&&!receipt.current(),"sole-owned unrequested metadata cleanup remains qualified");
        check(!SceneModelBindings::set_fresh_material_alpha(*domain,descriptors[0],0,&error),"stale descriptor cannot mutate retained bytes");
    }
    std::printf("default-context hierarchical direct material alpha: %u checks PASS\n",checks);
}
