#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/scene_model_bindings.hpp"
#include "tests/unit/scene_archive_pool_fixture.hpp"
#include "tests/unit/attachment_alternate_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"actor model binding: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;const auto bytes=scene_archive_fixture::bank();HsdRecordAnimationBank bank;check(bank.load({bytes.data(),bytes.size()},&error,true),"shared owned bank");
    const auto profile_bytes=attachment_alternate_fixture::profile();AttachmentAlternateProfile profile;check(attachment_alternate_load({profile_bytes.data(),profile_bytes.size()},&profile,&error),"qualified alternate recipe");
    HsdJointInstanceBank prepared;check(prepared.construct(bank,profile,&error)&&prepared.instances()==2,"normal and alternate roots");
    auto domain=scene_object_fixture::owner(16);SceneArchivePool archives;SceneArchiveLease source;check(archives.begin_fresh(scene_archive_fixture::profile(),domain,&error)&&archives.acquire(&source,&error)==SceneArchiveAcquire::Acquired&&archives.bind_borrowed_bank(source,bank,&error),"actual shared domain/archive");
    std::array<SceneDescriptorLease,2> descriptors;std::array<HsdJointInstanceReceipt,2> receipts;auto context=std::make_shared<SceneModelMaterialContext>();
    for(unsigned i=0;i<2;++i){
        check(domain->acquire(&descriptors[i],&error)==SceneDescriptorAcquire::Acquired,"real model descriptor");
        receipts[i]=prepared.receipt(i);HsdJointInstanceOwner root;check(prepared.take_instance(i,&root,&error)&&!prepared.state(i)&&!prepared.receipt(i).current()&&receipts[i].current(),"exclusive transfer retains original identity");
        if(i)check(!SceneModelBindings::attach_normal(*domain,descriptors[i],source,0,std::move(root),&error)&&root.current(),"normal-only guard remains unchanged");
        auto bad=std::make_shared<SceneModelMaterialContext>(std::array<u32,2>{0,0});
        const auto style=i?HsdJointInstanceStyle::Alternate:HsdJointInstanceStyle::Normal;
        auto colour=std::make_shared<SceneModelMaterialContext>(std::array<u32,2>{255,0},SceneModelMaterialKind::ColourWrapper);
        check(!SceneModelBindings::attach_actor(*domain,descriptors[i],source,0,style,std::move(root),colour,&error)&&root.current(),"equal cache words cannot substitute a070E4 colour-wrapper class");
        check(!SceneModelBindings::attach_actor(*domain,descriptors[i],source,0,style,std::move(root),bad,&error)&&root.current()&&!domain->root(descriptors[i]),"bad initial context refuses before consuming root");
        check(SceneModelBindings::attach_actor(*domain,descriptors[i],source,0,style,std::move(root),context,&error)&&!root.current(),"scoped contextual root attached");
        check(domain->root(descriptors[i])->receipt().same_instance(receipts[i])&&domain->root(descriptors[i])->state()->style==style&&domain->material_context(descriptors[i])==context.get()&&domain->inspect(descriptors[i])->material_context_required,"same identity/style/context survives typed0C ownership");
    }
    context.reset();for(unsigned i=0;i<2;++i){check(!domain->root(descriptors[i])&&!domain->material_context(descriptors[i])&&receipts[i].current(),"expired context closes contextual view, metadata graph still owned");check(domain->release(descriptors[i],&error)&&!receipts[i].current(),"unrequested metadata teardown remains qualified");}
    check(domain->snapshot()->free_count==16,"each actual root released once");
    std::printf("scene actor model bindings: %u checks PASS\n",checks);
}
