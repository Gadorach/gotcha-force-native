#include <cstdio>
#include <cstdlib>
#include "tests/unit/scene_physical_pools_fixture.hpp"
#include "tests/unit/scene_object_pool_fixture.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"class3 pool: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;SceneDescriptorBootstrap descriptors;check(descriptors.begin_fresh(physical_pools_fixture::descriptor_profile(),&error),"descriptor owner");
    auto raw=physical_pools_fixture::bytes();ScenePhysicalPoolsProfile profile;check(profile.load({raw.data(),raw.size()},&error),"physical profile");ScenePhysicalPools physical;check(physical.begin_fresh(profile,descriptors,&error),"physical owner");
    auto pool=physical.class2_pool();auto models=descriptors.model_pool();check(physical.bank(3)==pool->class3_snapshot(),"one actual class3 bank");
    SceneObjectLease a;SceneClass3Lease b,c;SceneCollisionLease d;
    check(pool->acquire(40,false,1,{},&a,&error)==SceneObjectAcquire::Allocated,"occupied class2 prefix");
    const auto first=models->snapshot()->free_head;
    check(pool->acquire_class3(40,false,2,&b,&error)==SceneObjectAcquire::Allocated,"class3 prepends to occupied shared group");
    check(pool->class3_models(b)->descriptor_count99==2&&pool->class3_models(b)->descriptor_indices[0]==first&&pool->class3_models(b)->descriptor_indices[1]==first+1,"descriptors follow actual existing occupancy");
    check(pool->inspect(b)->class02==3&&pool->inspect(b)->physical03==3&&pool->inspect(b)->registration85==255&&!pool->inspect(b)->active00,"rootless factory allocation");
    check(pool->acquire_collision(40,true,{},&d,&error)==SceneObjectAcquire::Allocated&&pool->acquire_class3(40,true,2,&c,&error)==SceneObjectAcquire::Allocated,"class4 then class3 append");
    check(pool->snapshot()->group_heads[40]==ScenePhysicalLink{3,b.index()}&&pool->snapshot()->group_tails[40]==ScenePhysicalLink{3,c.index()},"one head/tail store for all three classes");
    check(pool->inspect(b)->group_next8==ScenePhysicalLink{2,a.index()}&&pool->inspect(a)->link4==ScenePhysicalLink{3,b.index()}&&pool->inspect(a)->link8==ScenePhysicalLink{4,d.index()}&&pool->inspect(d)->group_next8==ScenePhysicalLink{3,c.index()}&&pool->inspect(c)->free_next4==ScenePhysicalLink{4,d.index()},"typed cross-class links");
    check(pool->release(a,&error)&&pool->inspect(b)->group_next8==ScenePhysicalLink{4,d.index()}&&pool->inspect(d)->free_next4==ScenePhysicalLink{3,b.index()},"class2 unlink patches class3/class4");
    check(pool->release(d,&error)&&pool->inspect(b)->group_next8==ScenePhysicalLink{3,c.index()}&&pool->inspect(c)->free_next4==ScenePhysicalLink{3,b.index()},"class4 unlink patches class3");
    check(pool->release(b,&error)&&pool->snapshot()->group_heads[40]==ScenePhysicalLink{3,c.index()}&&!pool->inspect(c)->free_next4.present(),"class3 head release");
    check(pool->release(c,&error)&&!pool->snapshot()->group_heads[40].present()&&!pool->snapshot()->group_tails[40].present(),"class3 final release");
    check(!b.current()&&!pool->release(b,&error),"stale class3 lease cannot release a second time");
    std::vector<SceneClass3Lease> all;for(unsigned i=0;i<512;++i){SceneClass3Lease lease;check(pool->acquire_class3(-1,false,8,&lease,&error)==SceneObjectAcquire::Allocated,"unlinked class3 allocations");all.push_back(lease);}
    check(all[510].index()==b.index()&&all[511].index()==c.index()&&all[510].generation()>b.generation()&&!b.current(),"class3 FIFO reuse preserves lease generations");
    check(pool->class3_models(all[0])->descriptor_count99==0&&models->snapshot()->free_count==1489,"group minus one checks capacity without creating descriptors");
    SceneClass3Lease exhausted;const auto full=*pool->class3_snapshot();
    check(pool->acquire_class3(40,false,2,&exhausted,&error)==SceneObjectAcquire::NoObjects&&!exhausted.current(),"actual class3 exhaustion");
    std::vector<SceneDescriptorLease> held;while(models->snapshot()->free_count){SceneDescriptorLease lease;check(models->acquire(&lease,&error)==SceneDescriptorAcquire::Acquired,"consume shared descriptors independently");held.push_back(lease);}
    const auto objects=*pool->snapshot();const auto collision=*pool->collision_snapshot();const auto empty_models=*models->snapshot();
    check(pool->acquire_class3(40,false,2,&exhausted,&error)==SceneObjectAcquire::NoDescriptors&&*pool->class3_snapshot()==full&&*pool->snapshot()==objects&&*pool->collision_snapshot()==collision&&*models->snapshot()==empty_models,"descriptor refusal precedes object exhaustion without writes");
    check(pool->release(all[0],&error)&&pool->acquire_class3(-1,false,1,&exhausted,&error)==SceneObjectAcquire::NoDescriptors,"unlinked allocation still checks descriptor count first");
    check(pool->acquire_class3(-1,false,0,&exhausted,&error)==SceneObjectAcquire::Allocated,"zero-descriptor unlinked allocation remains valid");
    for(const auto& lease:held)check(models->release(lease,&error),"release independently held descriptors");
    check(pool->release(all[1],&error)&&pool->acquire_class3(40,false,2,&b,&error)==SceneObjectAcquire::Allocated,"reused class3 model allocation");
    SceneDescriptorFields fields;fields.unresolved_root_key0c=0x1234;check(pool->set_declared_descriptor_fields(b,1,fields),"declared unresolved cleanup dependency");
    const auto before=*pool->class3_snapshot();const auto before_models=*models->snapshot();const auto before_groups=*pool->snapshot();
    check(!pool->release(b,&error)&&b.current()&&*pool->class3_snapshot()==before&&*models->snapshot()==before_models&&*pool->snapshot()==before_groups,"cleanup preflight refuses before unlink or partial descriptor release");
    SceneObjectPool standalone;auto other=scene_object_fixture::owner(4);check(standalone.begin_fresh(profile.class2_profile(),other,&error)&&standalone.acquire_class3(40,false,2,&c,&error)==SceneObjectAcquire::Refused&&!standalone.release(b,&error),"standalone/foreign owners cannot use class3");
    check(pool->acquire_class3(64,false,0,&c,&error)==SceneObjectAcquire::Refused&&pool->acquire_class3(-2,false,0,&c,&error)==SceneObjectAcquire::Refused&&pool->acquire_class3(40,false,9,&c,&error)==SceneObjectAcquire::Refused,"bounded group/descriptor domain");
    ScenePhysicalPools moved=std::move(physical);check(moved.current()&&b.current()&&exhausted.current(),"physical move retains leases");
    moved.retire();check(!b.current()&&!exhausted.current()&&!pool->class3_snapshot()&&!pool->ready(),"physical retirement invalidates all classes");
    std::printf("scene class3 pool: %u checks PASS\n",checks);
}
