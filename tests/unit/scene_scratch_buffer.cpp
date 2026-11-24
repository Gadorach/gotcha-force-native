#include <cstdio>
#include <cstdlib>
#include "tests/unit/scene_descriptor_bootstrap_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}
struct Heap:gf::ISceneScratchHeap {bool fail=false;unsigned calls=0;gf::u32 last=0;gf::SceneScratchAllocation allocate(gf::u32 n)override{++calls;last=n;return fail?gf::SceneScratchAllocation{}:gf::SceneScratchAllocation::allocate(n);}};
struct RetireHeap:gf::ISceneScratchHeap {gf::SceneDescriptorBootstrap* owner=nullptr;gf::SceneScratchAllocation allocate(gf::u32 n)override{owner->retire();return gf::SceneScratchAllocation::allocate(n);}};
}
int main(){
    using namespace gf;std::string error;const auto raw=descriptor_bootstrap_fixture::bytes();SceneDescriptorBootstrapProfile profile;check(profile.load({raw.data(),raw.size()},&error));
    SceneDescriptorBootstrap b;check(b.begin_fresh(profile,&error)&&b.pristine_for_physical_join());Heap heap;SceneScratchLease first;
    const auto pool2=*b.small_pool(2);const auto models=*b.model_pool()->snapshot();const auto archives=*b.archive_pool()->snapshot();
    check(b.acquire_scratch(180,heap,&first,&error)==SceneScratchAcquire::Allocated&&first.current()&&first.index()==0&&first.generation()==1&&first.requested_bytes()==180&&first.rounded_bytes()==192&&heap.last==192);
    const auto& record=b.small_pool(3)->records[0];check(record.header0c_nonnull&&record.buffer_owned&&record.initialized_bytes==0&&!record.consumers&&record.requested_bytes==180&&record.rounded_bytes==192);
    check(!b.pristine_for_physical_join()&&*b.small_pool(2)==pool2&&*b.model_pool()->snapshot()==models&&*b.archive_pool()->snapshot()==archives);
    const auto copy=first;check(b.release_scratch(first,&error)&&!first.current()&&!copy.current()&&!b.small_pool(3)->records[0].buffer_owned&&b.small_pool(3)->records[0].header0c_nonnull);
    SceneScratchLease second;check(b.acquire_scratch(1,heap,&second,&error)==SceneScratchAcquire::Allocated&&second.index()==0&&second.generation()==2&&!first.current()&&heap.last==32);
    check(b.release_scratch(second,&error));heap.fail=true;SceneScratchLease failed;const auto before=*b.small_pool(3);
    check(b.acquire_scratch(180,heap,&failed,&error)==SceneScratchAcquire::HeapFailure&&!failed.current()&&heap.last==192);
    const auto& after=*b.small_pool(3);check(after.free_count==before.free_count&&after.free_head==before.free_head&&after.live_head==before.live_head&&after.records[0].generation==3&&!after.records[0].header0c_nonnull&&!after.records[0].allocated09);
    check(!b.pristine_for_physical_join());heap.fail=false;
    for(u32 n:{0u,0xffffffffu,(16u<<20)+1}){const auto snap=*b.small_pool(3);check(b.acquire_scratch(n,heap,&failed,&error)==SceneScratchAcquire::Refused&&*b.small_pool(3)==snap);}
    std::vector<SceneScratchLease> all;
    for(unsigned i=0;i<512;++i){SceneScratchLease lease;check(b.acquire_scratch(32,heap,&lease,&error)==SceneScratchAcquire::Allocated);all.push_back(lease);}
    const auto calls=heap.calls;check(b.acquire_scratch(180,heap,&failed,&error)==SceneScratchAcquire::PoolExhausted&&heap.calls==calls&&!failed.current());
    SceneDescriptorBootstrap moved=std::move(b);check(all[0].current()&&moved.current()&&!b.current());moved.retire();for(const auto& lease:all)check(!lease.current());
    SceneDescriptorBootstrap reentrant;check(reentrant.begin_fresh(profile,&error));RetireHeap retire;retire.owner=&reentrant;
    check(reentrant.acquire_scratch(180,retire,&failed,&error)==SceneScratchAcquire::Refused&&!reentrant.current()&&!failed.current());
    std::printf("source pool3 scratch ownership/rounding/failures: %u checks PASS\n",checks);
}
