#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/scene_model_bindings.hpp"
#include "tests/unit/scene_archive_pool_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"HUD model binding: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;
    for(unsigned scenario=0;scenario<4;++scenario){
        hsd_record_animation_fixture::Fixture f;f.ptr(0x18,0xe80);
        auto clear=[&](unsigned at){f.word(at,0);std::erase(f.fields,at);};
        clear(0x108);clear(0x34);clear(0x38);f.word(0x104,scenario==2?0x20280080:0x20080080);
        if(scenario==1){f.ptr(0x38,0x60);clear(0x60);} //nonnull empty material table
        if(scenario==3)f.ptr(0x38,0x60); //material animation is authored but not attached
        const auto raw=f.archive();HsdRecordAnimationBank bank;check(bank.load({raw.data(),raw.size()},&error),"owned bank");
        HsdJointInstanceBank roots;const std::array<u16,1> records{0};check(roots.construct_normal_records(bank,records,&error),"one fresh default root");
        auto domain=scene_object_fixture::owner(8);SceneArchivePool archives;SceneArchiveLease archive;
        check(archives.begin_fresh(scene_archive_fixture::profile(),domain,&error)&&archives.acquire(&archive,&error)==SceneArchiveAcquire::Acquired&&archives.bind_borrowed_bank(archive,bank,&error),"actual archive/domain");
        SceneDescriptorLease descriptor;check(domain->acquire(&descriptor,&error)==SceneDescriptorAcquire::Acquired,"actual descriptor");
        const auto identity=roots.receipt(0);HsdJointInstanceOwner root;
        check(roots.take_instance(0,&root,&error)&&SceneModelBindings::attach_normal(*domain,descriptor,archive,0,std::move(root),&error),"normal root transfer");
        const auto before=*domain->root(descriptor)->state();const auto descriptors=*domain->snapshot();
        check(SceneModelBindings::finish_fresh_empty_material_animation(*domain,descriptor,archive,&error)==(scenario==0),"null table/no-op flags ONLY; nonnull empty is not absence");
        const auto* state=domain->root(descriptor)->state();
        check(!state->animation_requested&&!state->world_evaluated&&!state->joints[0].world&&state->joints[0].joint.flags==before.joints[0].joint.flags&&*domain->snapshot()==descriptors,"no request, clocks, flags, worlds or descriptor stores");
        check(SceneModelBindings::set_fresh_diffuse_rgb(*domain,descriptor,{0,255,0},&error),"narrow diffuse RGB mutation");
        state=domain->root(descriptor)->state();
        for(unsigned i=0;i<state->mesh_properties.size();++i){
            const auto& a=before.mesh_properties[i].second;const auto& b=state->mesh_properties[i].second;
            check(b.material_colour[0]==0&&b.material_colour[1]==255&&b.material_colour[2]==0&&b.material_colour[3]==a.material_colour[3]&&b.material_alpha==a.material_alpha&&b.texture==a.texture&&b.render_mode==a.render_mode&&b.pobj_flags==a.pobj_flags,"only diffuse RGB; preserve alpha, texture and render/geometry flags");
        }
        check(state->dobjs[0].material.alpha==before.dobjs[0].material.alpha&&state->dobjs[0].flags==before.dobjs[0].flags&&!state->side_style_applied&&!state->stage_shadow_applied&&!state->child_translucent_applied,"no shadow/side/alternate styling");
        check(domain->root(descriptor)->receipt().same_instance(identity),"same mutable owner");
        archives.retire();check(!SceneModelBindings::finish_fresh_empty_material_animation(*domain,descriptor,archive,&error),"stale source archive refuses");
        check(domain->release(descriptor,&error)&&!identity.current(),"qualified sole metadata root cleanup");
        check(!SceneModelBindings::set_fresh_diffuse_rgb(*domain,descriptor,{255,0,0},&error),"stale descriptor cannot mutate");
    }
    std::printf("initial HUD empty-animation/diffuse bindings: %u checks PASS\n",checks);
}
