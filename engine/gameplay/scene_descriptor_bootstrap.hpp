#pragma once
#include "engine/assets/scene_descriptor_bootstrap.hpp"
#include "engine/gameplay/scene_archive_pool.hpp"
#include "engine/gameplay/scene_scratch_buffer.hpp"
namespace gf {
//Typed native fresh storage, not a guest image. Pool2 hooks remain unexposed;
//pool3's bounded06DC8 allocation/06F1C cleanup share these ACTUAL records.
class SceneDescriptorBootstrap {
public:
    SceneDescriptorBootstrap()=default;SceneDescriptorBootstrap(const SceneDescriptorBootstrap&)=delete;SceneDescriptorBootstrap& operator=(const SceneDescriptorBootstrap&)=delete;
    SceneDescriptorBootstrap(SceneDescriptorBootstrap&&)=default;SceneDescriptorBootstrap& operator=(SceneDescriptorBootstrap&&)=default;~SceneDescriptorBootstrap(){retire();}
    bool begin_fresh(const SceneDescriptorBootstrapProfile& profile,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("descriptor bootstrap: ")+why;return false;};if(attempted_||!profile.ready())return reject("fresh one-shot owner/profile required");attempted_=true;
        try{auto next=std::make_unique<Storage>();next->profile=profile;next->models=std::make_shared<SceneDescriptorPool>();next->archives=std::make_shared<SceneArchivePool>();
            if(!next->models->begin_fresh(profile.model_profile(),error)||!next->archives->begin_fresh(profile.archive_profile(),next->models,error))return false;
            for(unsigned i=2;i<4;++i){auto pool=std::make_shared<scene_small_descriptor_detail::Storage>();pool->pool=u8(i);pool->domain=next->models;pool->identity=next->models->snapshot();auto& s=pool->state;const auto& row=*profile.row(i);const auto count=row.bytes/row.stride;s.free_head=0;s.free_count=s16(count);s.records.resize(count);pool->buffers.resize(count);for(unsigned j=0;j<count;++j){auto& r=s.records[j];r.pool08=u8(i);r.link0=j+1<count?s32(j+1):-1;}next->small[i-2]=std::move(pool);}
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain native fresh allocation; no retry or source reset");}
    }
    bool current()const{return storage_&&storage_->models&&storage_->models->ready()&&storage_->archives&&storage_->archives->matches_descriptor_owner(storage_->models)&&storage_->small[0]&&storage_->small[0]->current()&&storage_->small[1]&&storage_->small[1]->current();}
    std::shared_ptr<SceneDescriptorPool> model_pool()const{return current()?storage_->models:nullptr;}
    std::shared_ptr<SceneArchivePool> archive_pool()const{return current()?storage_->archives:nullptr;}
    const SceneSmallDescriptorPoolSnapshot* small_pool(unsigned index)const{return current()&&index>=2&&index<4?&storage_->small[index-2]->state:nullptr;}
    SceneScratchAcquire acquire_scratch(u32 bytes,ISceneScratchHeap& heap,SceneScratchLease* out,std::string* error){
        if(out)*out={};
        auto fail=[&](const char* why){if(error)*error=std::string("scene scratch: ")+why;return SceneScratchAcquire::Refused;};
        if(!out||!current()||!bytes||bytes>(16u<<20))return fail("positive bounded06DC8 request/current owner required");
        const auto scope=storage_->small[1];auto& s=scope->state;if(!scope->current())return fail("retired source pool3");
        if(s.free_count<=0){if(error)error->clear();return SceneScratchAcquire::PoolExhausted;}
        u16 index=0;if(!scene_pool_links::take(s,&index,"scene scratch: free-list inconsistency","scene scratch: generation/state overflow",error))return SceneScratchAcquire::Refused;
        auto& r=s.records[index];r.requested_bytes=bytes;r.rounded_bytes=(bytes+31u)&~31u;r.initialized_bytes=0;r.consumers=0;
        try{
            auto block=heap.allocate(r.rounded_bytes);
            if(!scope->current())return fail("owner retired/replaced during heap callback; consumed prefix retained");
            r.header0c_nonnull=block.allocated();r.buffer_owned=block.allocated();
            if(!block.allocated()){scope->buffers[index].reset();scene_pool_links::put(s,index);if(error)error->clear();return SceneScratchAcquire::HeapFailure;}
            if(block.bytes()!=r.rounded_bytes||!block.aligned()){scope->live=false;return fail("heap contract mismatch; no replay");}
            scope->buffers[index]=std::move(block);out->owner_=scope;out->index_=index;out->generation_=r.generation;
            if(error){error->clear();}return SceneScratchAcquire::Allocated;
        }catch(const std::exception&){scope->live=false;return fail("uncertain heap effect; consumed prefix retained");}
    }
    bool release_scratch(const SceneScratchLease& lease,std::string* error){
        const auto scope=lease.owner_.lock();
        if(!current()||!scope||scope!=storage_->small[1]||!lease.current()){if(error)*error="scene scratch: foreign/stale lease";return false;}
        auto& r=scope->state.records[lease.index_];if(r.consumers){if(error)*error="scene scratch: actual consumer cleanup/unbind required";return false;}
        scope->buffers[lease.index_].reset();r.buffer_owned=false; //source0C retained, not cleared by06F1C
        scene_pool_links::put(scope->state,lease.index_);if(error)error->clear();return true;
    }
    const SceneDescriptorBootstrapProfile* recipe()const{return current()?&storage_->profile:nullptr;}
    //The next88398 call has no intervening descriptor consumer. Capacity alone
    //cannot prove freshness after allocate/free or a claimed class2 owner.
    bool pristine_for_physical_join()const{
        if(!current()||storage_->models->storage_->class2_claimed){return false;}const auto* m=storage_->models->snapshot();const auto* a=storage_->archives->snapshot();
        if(m->free_count!=1489||m->free_head!=0||m->live_head!=-1||a->free_count!=32||a->free_head!=0||a->live_head!=-1)return false;
        for(unsigned i=0;i<m->records.size();++i){const auto& r=m->records[i];if(r.generation||r.allocated09||r.native_root0c||r.link0!=(i+1<m->records.size()?s32(i+1):-1)||r.link4!=-1)return false;}
        for(unsigned i=0;i<a->records.size();++i){const auto& r=a->records[i];if(r.generation||r.allocated09||r.header_owned||r.link0!=(i+1<a->records.size()?s32(i+1):-1)||r.link4!=-1)return false;}
        for(const auto& pool:storage_->small){const auto& s=pool->state;if(s.free_head!=0||s.live_head!=-1||s.free_count!=s16(s.records.size()))return false;for(unsigned i=0;i<s.records.size();++i){const auto& r=s.records[i];if(r.generation||r.allocated09||r.header0c_nonnull||r.buffer_owned||r.consumers||r.link0!=(i+1<s.records.size()?s32(i+1):-1)||r.link4!=-1)return false;}}
        return true;
    }
    bool small_pool_allocation_consumers_pending()const{return true;}
    void retire(){storage_.reset();attempted_=true;} //native lifetime invalidation, not SDK bulk cleanup
private:
    friend class BattlePreparationBootstrap;
    bool claim_scratch_consumer(const SceneScratchLease& lease){const auto s=lease.owner_.lock();if(!current()||!s||s!=storage_->small[1]||!lease.current()||s->state.records[lease.index_].consumers)return false;++s->state.records[lease.index_].consumers;return true;}
    struct Storage {SceneDescriptorBootstrapProfile profile;std::shared_ptr<SceneDescriptorPool> models;std::shared_ptr<SceneArchivePool> archives;std::array<std::shared_ptr<scene_small_descriptor_detail::Storage>,2> small;~Storage(){for(auto& s:small)if(s)s->live=false;if(archives)archives->retire();if(models)models->retire();}};std::unique_ptr<Storage> storage_;bool attempted_=false;
};
} // namespace gf
