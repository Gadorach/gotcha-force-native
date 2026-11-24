#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/scene_model_bindings.hpp"
#include "tests/unit/scene_archive_pool_fixture.hpp"
#include "tests/unit/hsd_record_texture_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"scene material animation: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;
    for(unsigned scenario=0;scenario<5;++scenario){
        auto f=hsd_record_texture_fixture::fixture(scenario==4?12:2);
        auto clear=[&](unsigned at){f.word(at,0);std::erase(f.fields,at);};
        clear(0x108);clear(0x800);if(scenario!=1)clear(0x34);if(scenario!=2)clear(0x904);
        if(scenario==3)clear(0x38);
        f.ptr(0x18,0xf80);const auto raw=f.archive();HsdRecordAnimationBank bank;check(bank.load({raw.data(),raw.size()},&error,true),"owned bank");
        auto domain=scene_object_fixture::owner(8);SceneArchivePool archives;SceneArchiveLease archive;
        check(archives.begin_fresh(scene_archive_fixture::profile(),domain,&error)&&archives.acquire(&archive,&error)==SceneArchiveAcquire::Acquired&&archives.bind_borrowed_bank(archive,bank,&error),"same archive/domain");
        HsdJointInstanceBank prepared;const std::array<u16,1> records{0};check(prepared.construct_normal_records(bank,records,&error),"fresh one-joint instance");
        HsdJointInstanceOwner root;const auto identity=prepared.receipt(0);SceneDescriptorLease descriptor;
        check(domain->acquire(&descriptor,&error)==SceneDescriptorAcquire::Acquired&&prepared.take_instance(0,&root,&error)&&SceneModelBindings::attach_normal(*domain,descriptor,archive,0,std::move(root),&error),"actual descriptor ownership");
        const auto before=*domain->root(descriptor)->state();const auto descriptors=*domain->snapshot();SceneMaterialAnimationOwner animation;
        check(SceneModelBindings::request_fresh_material_texture_zero(*domain,descriptor,archive,&animation,&error)==(scenario==0),"only nonempty texture-only material table is admitted");
        const auto* owned=domain->root(descriptor);const auto* state=owned->state();
        check(!state->world_evaluated&&!owned->world_current()&&!state->joints[0].world&&state->joints[0].joint.flags==before.joints[0].joint.flags&&state->animation_requested==(scenario==0)&&*domain->snapshot()==descriptors,"no descriptor stores, root flag change or fabricated world");
        if(scenario){check(!animation.current()&&!animation.pose()&&animation.trace().empty(),"failed selection grants no retained runtime");check(domain->release(descriptor,&error)&&!identity.current(),"untouched metadata cleanup remains qualified");continue;}
        check(animation.current()&&animation.same_root(*owned)&&animation.pose()->materials.size()==1&&animation.trace().size()==1,"runtime is bound to the exact descriptor/root");
        check(animation.trace()[0].tracks.size()==1&&animation.trace()[0].tracks[0].callbacks==1&&animation.trace()[0].frame==0,"shared player executes first time0 callback, not a clock tick");
        check(animation.pose()->materials[0].has_texture&&animation.pose()->materials[0].uv_offset[0]==.25f,"genuine initial texture callback result retained separately from authored metadata");
        check(state->mesh_properties[0].second.material_alpha==before.mesh_properties[0].second.material_alpha&&state->mesh_properties[0].second.texture==before.mesh_properties[0].second.texture,"authored material/texture identity unchanged");
        SceneMaterialAnimationOwner second;check(!SceneModelBindings::request_fresh_material_texture_zero(*domain,descriptor,archive,&second,&error)&&!second.current(),"already requested root cannot replay as fresh");
        check(!SceneModelBindings::request_fresh_material_texture_zero(*domain,descriptor,archive,nullptr,&error),"output owner is required");
        SceneMaterialAnimationOwner moved=std::move(animation);check(moved.current()&&!animation.current()&&moved.same_root(*owned),"move preserves, never duplicates runtime ownership");
        check(!domain->release(descriptor,&error),"attached animation cleanup remains unsupported");
        const auto copied_pose=*moved.pose();archives.retire();check(!moved.current()&&!moved.pose()&&moved.trace().empty()&&copied_pose.materials.size()==1&&identity.current(),"copied pose/retained bytes do not keep source archive current");
        domain->retire();check(!identity.current()&&!moved.current(),"descriptor retirement closes retained root/player join");
    }
    std::printf("descriptor-owned fresh material texture animation: %u checks PASS\n",checks);
}
