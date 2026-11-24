#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/scene_model_bindings.hpp"
#include "tests/unit/scene_archive_pool_fixture.hpp"
#include "tests/unit/hsd_record_texture_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"requested diffuse: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;auto f=hsd_record_texture_fixture::fixture();
    auto clear=[&](unsigned at){f.word(at,0);std::erase(f.fields,at);};
    clear(0x108);clear(0x800);clear(0x34);clear(0x904);f.ptr(0x18,0xf80);
    const auto bytes=f.archive();HsdRecordAnimationBank bank;check(bank.load({bytes.data(),bytes.size()},&error,true),"owned texture bank");
    auto domain=scene_object_fixture::owner(8);SceneArchivePool archives;SceneArchiveLease archive;check(archives.begin_fresh(scene_archive_fixture::profile(),domain,&error)&&archives.acquire(&archive,&error)==SceneArchiveAcquire::Acquired&&archives.bind_borrowed_bank(archive,bank,&error),"same source domain");
    HsdJointInstanceBank roots;const std::array<u16,3> records{0,0,0};check(roots.construct_normal_records(bank,records,&error),"independent roots");
    std::array<SceneDescriptorLease,3> descriptors;std::array<SceneMaterialAnimationOwner,2> animations;
    for(unsigned i=0;i<3;++i){HsdJointInstanceOwner root;check(domain->acquire(&descriptors[i],&error)==SceneDescriptorAcquire::Acquired&&roots.take_instance(i,&root,&error)&&SceneModelBindings::attach_normal(*domain,descriptors[i],archive,0,std::move(root),&error),"actual source root");if(i<2)check(SceneModelBindings::request_fresh_material_texture_selector(*domain,descriptors[i],archive,f32(i),&animations[i],&error),"actual requested texture owner");}
    const auto before=*domain->root(descriptors[0])->state();const auto fields=*domain->snapshot();const auto trace=animations[1].trace();const auto uv=animations[1].pose()->materials[0].uv_offset[0];
    check(!SceneModelBindings::set_requested_texture_diffuse_rgb(*domain,descriptors[0],animations[1],{0,0,0},&error),"foreign root/runtime pairing refuses");
    check(!SceneModelBindings::set_requested_texture_diffuse_rgb(*domain,descriptors[2],animations[0],{0,0,0},&error),"unrequested root cannot borrow another runtime");
    check(SceneModelBindings::set_requested_texture_diffuse_rgb(*domain,descriptors[0],animations[0],{255,255,255},&error)&&SceneModelBindings::set_requested_texture_diffuse_rgb(*domain,descriptors[1],animations[1],{0,0,0},&error),"actual source white/black constant writes after selectors");
    for(unsigned i=0;i<2;++i){const auto& mesh=domain->root(descriptors[i])->state()->mesh_properties[0].second;for(unsigned k=0;k<3;++k)check(mesh.material_colour[k]==(i?0:255)&&animations[i].pose()->materials[0].diffuse[k]==(i?0:1),"both retained mutable MObj and player base colour changed");check(mesh.material_colour[3]==before.mesh_properties[0].second.material_colour[3]&&mesh.material_alpha==before.mesh_properties[0].second.material_alpha&&mesh.texture==before.mesh_properties[0].second.texture&&mesh.render_mode==before.mesh_properties[0].second.render_mode,"alpha, texture, category and source geometry remain authored");}
    check(*domain->snapshot()==fields&&animations[1].trace()[0].tracks[0].callbacks==trace[0].tracks[0].callbacks&&animations[1].trace()[0].frame==trace[0].frame&&animations[1].pose()->materials[0].uv_offset[0]==uv,"constant write does not replay, tick or replace UV state");
    HsdRecordAnimationBinding binding;check(bank.select_textures(0,2,0,0,&binding,&error),"shared player binding");
    HsdRecordAnimationPlayer player;check(player.bind(binding,&error)&&player.request(0,1,&error)&&player.set_texture_only_base_diffuse({0,0,0},&error)&&player.advance(1,&error)&&player.pose()->materials[0].diffuse[0]==0&&player.request_again(2,1,&error)&&player.pose()->materials[0].diffuse[0]==0,"constant base survives texture ticks and re-requests");
    check(player.bind(binding,&error)&&player.request(0,1,&error)&&player.pose()->materials[0].diffuse[0]==1,"new binding clears prior-instance override");
    archives.retire();check(!SceneModelBindings::set_requested_texture_diffuse_rgb(*domain,descriptors[1],animations[1],{255,0,0},&error),"stale source runtime refuses mutation");
    std::printf("post-selector default RGB/base-colour ownership: %u checks PASS\n",checks);
}
