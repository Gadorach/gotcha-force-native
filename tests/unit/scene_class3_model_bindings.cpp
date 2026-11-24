#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/scene_model_bindings.hpp"
#include "tests/unit/scene_physical_pools_fixture.hpp"
#include "tests/unit/scene_archive_pool_fixture.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"class3 model binding: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;SceneDescriptorBootstrap descriptors;check(descriptors.begin_fresh(physical_pools_fixture::descriptor_profile(),&error),"descriptor owner");
    auto raw=physical_pools_fixture::bytes();ScenePhysicalPoolsProfile profile;check(profile.load({raw.data(),raw.size()},&error),"physical recipe");ScenePhysicalPools physical;check(physical.begin_fresh(profile,descriptors,&error),"physical owner");
    auto pool=physical.class2_pool();const auto bytes=scene_archive_fixture::bank();HsdRecordAnimationBank bank;check(bank.load({bytes.data(),bytes.size()},&error,true),"shared source bank");
    SceneArchiveLease archive;check(descriptors.archive_pool()->acquire(&archive,&error)==SceneArchiveAcquire::Acquired&&descriptors.archive_pool()->bind_borrowed_bank(archive,bank,&error),"same scene archive");
    SceneClass3Lease object;check(pool->acquire_class3(40,false,2,&object,&error)==SceneObjectAcquire::Allocated,"real class3 descriptors");
    HsdJointInstanceBank instances;const std::array<u16,2> records{0,0};check(instances.construct_normal_records(bank,records,&error),"two distinct copies of the same authored record");
    std::array<HsdJointInstanceReceipt,2> receipts;
    for(unsigned i=0;i<2;++i){
        receipts[i]=instances.receipt(i);HsdJointInstanceOwner root;check(instances.take_instance(i,&root,&error),"exclusive root transfer");
        check(!SceneModelBindings::attach_object_descriptor(*pool,object,2,archive,0,std::move(root),&error)&&root.current(),"invalid slot preserves pending root");
        check(SceneModelBindings::attach_object_descriptor(*pool,object,i,archive,0,std::move(root),&error)&&!root.current(),"same-owner class3 root attachment");
        check(SceneModelBindings::object_root(*pool,object,i)->receipt().same_instance(receipts[i])&&!SceneModelBindings::object_root(*pool,object,i)->world_current(),"identity preserved without world authority");
    }
    check(!receipts[0].same_instance(receipts[1]),"repeated record has independent mutable roots");
    const auto before=*descriptors.model_pool()->snapshot();const auto objects=*pool->class3_snapshot();
    HsdJointInstanceBank foreign;check(foreign.construct_normal_records(bank,{records.data(),1},&error),"foreign-source test root");HsdJointInstanceOwner pending;check(foreign.take_instance(0,&pending,&error),"foreign-source root transfer");
    auto other=scene_object_fixture::owner(4);SceneArchivePool other_archives;SceneArchiveLease other_archive;check(other_archives.begin_fresh(scene_archive_fixture::profile(),other,&error)&&other_archives.acquire(&other_archive,&error)==SceneArchiveAcquire::Acquired&&other_archives.bind_borrowed_bank(other_archive,bank,&error),"foreign archive domain with identical bank");
    check(!SceneModelBindings::attach_object_descriptor(*pool,object,0,other_archive,0,std::move(pending),&error)&&pending.current()&&*descriptors.model_pool()->snapshot()==before&&*pool->class3_snapshot()==objects&&receipts[0].current(),"foreign domain cannot replace current root");
    check(pool->release(object,&error)&&!receipts[0].current()&&!receipts[1].current()&&!SceneModelBindings::object_root(*pool,object,0),"qualified metadata cleanup retires both actual roots");
    check(!SceneModelBindings::attach_object_descriptor(*pool,object,0,archive,0,std::move(pending),&error)&&pending.current(),"stale class3 object cannot accept a new root");
    std::printf("scene class3 model bindings: %u checks PASS\n",checks);
}
