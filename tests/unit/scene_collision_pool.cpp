#include <cstdio>
#include <cstdlib>
#include "tests/unit/scene_physical_pools_fixture.hpp"
#include "tests/unit/scene_object_pool_fixture.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"collision pool: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;SceneDescriptorBootstrap descriptors;check(descriptors.begin_fresh(physical_pools_fixture::descriptor_profile(),&error),"descriptor owner");
    auto raw=physical_pools_fixture::bytes();ScenePhysicalPoolsProfile profile;check(profile.load({raw.data(),raw.size()},&error),"physical profile");ScenePhysicalPools physical;check(physical.begin_fresh(profile,descriptors,&error),"fresh physical owner");
    auto pool=physical.class2_pool();check(physical.bank(4)==pool->collision_snapshot(),"class4 snapshot is the actual single bank");
    SceneObjectLease a,b;SceneCollisionLease x,y;
    check(pool->acquire(50,false,1,std::nullopt,&a,&error)==SceneObjectAcquire::Allocated,"class2 head");
    check(pool->acquire_collision(48,false,SceneObjectParentFields{0,1},&x,&error)==SceneObjectAcquire::Allocated,"class4 parent-biased prepend");
    check(pool->acquire(50,true,1,std::nullopt,&b,&error)==SceneObjectAcquire::Allocated,"class2 append behind class4");
    check(pool->acquire_collision(50,true,std::nullopt,&y,&error)==SceneObjectAcquire::Allocated,"class4 append");
    check(a.index()==x.index()&&b.index()==y.index()&&ScenePhysicalLink{2,a.index()}!=ScenePhysicalLink{4,x.index()},"equal arena-local indices have distinct typed identities");
    check(pool->snapshot()->group_heads[50]==ScenePhysicalLink{4,x.index()}&&pool->snapshot()->group_tails[50]==ScenePhysicalLink{4,y.index()},"one mixed-class head/tail store");
    check(pool->inspect(x)->group_next8==ScenePhysicalLink{2,a.index()}&&pool->inspect(a)->link4==ScenePhysicalLink{4,x.index()}&&pool->inspect(a)->link8==ScenePhysicalLink{2,b.index()}&&pool->inspect(b)->link8==ScenePhysicalLink{4,y.index()}&&pool->inspect(y)->free_next4==ScenePhysicalLink{2,b.index()},"cross-class intrusive source links");
    check(descriptors.model_pool()->snapshot()->free_count==1487&&pool->collision_snapshot()->free_count==382,"class4 consumes no model descriptors");
    check(pool->release(a,&error)&&pool->inspect(x)->group_next8==ScenePhysicalLink{2,b.index()}&&pool->inspect(b)->link4==ScenePhysicalLink{4,x.index()},"class2 middle unlink patches class4 neighbor");
    check(pool->release(x,&error)&&pool->snapshot()->group_heads[50]==ScenePhysicalLink{2,b.index()}&&!pool->inspect(b)->link4.present(),"class4 head unlink patches class2 neighbor");
    check(pool->release(y,&error)&&pool->snapshot()->group_tails[50]==ScenePhysicalLink{2,b.index()}&&!pool->inspect(b)->link8.present(),"class4 tail unlink patches class2 neighbor");
    check(pool->release(b,&error)&&!pool->snapshot()->group_heads[50].present()&&!pool->snapshot()->group_tails[50].present(),"mixed group drained");
    check(!x.current()&&!pool->release(x,&error)&&pool->collision_snapshot()->free_tail==y.index(),"stale lease refused, class4 FIFO tail preserved");
    std::vector<SceneCollisionLease> leases;for(unsigned i=0;i<384;++i){SceneCollisionLease lease;check(pool->acquire_collision(-1,false,std::nullopt,&lease,&error)==SceneObjectAcquire::Allocated,"real class4 exhaustion");leases.push_back(lease);}
    check(leases[382].index()==x.index()&&leases[383].index()==y.index()&&leases[382].generation()>x.generation(),"class4 FIFO reuses old slots after untouched ones");
    const auto before=*pool->snapshot();const auto models=*descriptors.model_pool()->snapshot();SceneCollisionLease exhausted;
    check(pool->acquire_collision(50,false,std::nullopt,&exhausted,&error)==SceneObjectAcquire::NoObjects&&!exhausted.current()&&*pool->snapshot()==before&&*descriptors.model_pool()->snapshot()==models,"class4 exhaustion does not consume class2/model pools or mutate groups");
    SceneObjectPool standalone;auto other=scene_object_fixture::owner(4);check(standalone.begin_fresh(profile.class2_profile(),other,&error)&&standalone.acquire_collision(48,false,{},&exhausted,&error)==SceneObjectAcquire::Refused,"standalone class2 cannot invent a physical class4 bank");
    ScenePhysicalPools moved=std::move(physical);check(moved.current()&&leases[0].current(),"shared physical move keeps class4 leases");moved.retire();check(!leases[0].current()&&!pool->collision_snapshot()&&!pool->ready(),"physical retirement invalidates both classes without source cleanup");
    std::printf("scene collision pool: %u checks PASS\n",checks);
}
