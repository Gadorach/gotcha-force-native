#pragma once
#include "engine/assets/scene_archive_pool.hpp"
#include "engine/assets/hsd_record_animation.hpp"
#include "engine/gameplay/scene_descriptor_pool.hpp"
#include <new>
namespace gf {
struct SceneArchiveRecord {
    s32 link0=-1,link4=-1;u8 pool08=1,allocated09=0,flag0a=0;u64 generation=0;
    bool header0c_nonnull=false,header_owned=false,bank_loaded=false,borrowed_header=false,data14_present=false;
    u32 scene10_source_key=0,data14_hash=0,bank_hash=0;u8 cameras18=0,fog19=0;u16 models1a=0;
    bool operator==(const SceneArchiveRecord&)const=default;
};
struct SceneArchivePoolSnapshot {s32 free_head=-1,live_head=-1;s16 free_count=0;std::vector<SceneArchiveRecord> records;bool operator==(const SceneArchivePoolSnapshot&)const=default;};
namespace scene_archive_pool_detail {
struct Storage {
    bool live=true;std::shared_ptr<SceneDescriptorPool> domain;const SceneDescriptorPoolSnapshot* domain_identity=nullptr;SceneArchivePoolSnapshot state;
    std::vector<std::shared_ptr<HsdRecordAnimationBank>> headers;
    bool current()const{return live&&domain&&domain->ready()&&domain->snapshot()==domain_identity;}
};
}
class SceneArchivePool;
class SceneArchiveLease {
public:
    bool current()const{const auto s=owner_.lock();return s&&s->current()&&index_<s->state.records.size()&&s->state.records[index_].allocated09&&s->state.records[index_].generation==generation_;}
    const HsdRecordAnimationBank* bank()const{const auto s=owner_.lock();return current()&&s->state.records[index_].bank_loaded?s->headers[index_].get():nullptr;}
    const SceneArchiveRecord* inspect()const{const auto s=owner_.lock();return current()?&s->state.records[index_]:nullptr;}
    bool belongs_to(const SceneDescriptorPool& domain)const{const auto s=owner_.lock();return current()&&s->domain.get()==&domain&&domain.snapshot()==s->domain_identity;}
    u16 index()const{return index_;}u64 generation()const{return generation_;}
private:friend class SceneArchivePool;std::weak_ptr<scene_archive_pool_detail::Storage> owner_;u16 index_=65535;u64 generation_=0;
};
enum class SceneArchiveAcquire {Acquired,Exhausted,HeapFailure,Refused};
//Profile1 pool1, 06E3C blank header and06E8C borrowed-buffer cleanup ONLY.
//Native parsed-bank ownership replaces the source68B heap header; it is not a
//binary-layout/allocator-hook/DMA or global scene-occupancy acknowledgement.
class SceneArchivePool {
public:
    SceneArchivePool()=default;SceneArchivePool(const SceneArchivePool&)=delete;SceneArchivePool& operator=(const SceneArchivePool&)=delete;
    SceneArchivePool(SceneArchivePool&&)=default;SceneArchivePool& operator=(SceneArchivePool&&)=default;~SceneArchivePool(){retire();}
    bool begin_fresh(const SceneArchivePoolProfile& profile,const std::shared_ptr<SceneDescriptorPool>& domain,std::string* error){
        if(attempted_||!profile.ready()||!domain||!domain->ready()||domain->storage_->archive_pool1_claimed){if(error)*error="scene archive pool: fresh recipe/unclaimed live domain required";return false;}
        attempted_=true;domain->storage_->archive_pool1_claimed=true;
        try{auto next=std::make_shared<scene_archive_pool_detail::Storage>();next->domain=domain;next->domain_identity=domain->snapshot();auto& s=next->state;s.records.resize(profile.capacity());next->headers.resize(profile.capacity());s.free_count=s16(profile.capacity());s.free_head=0;for(unsigned i=0;i<s.records.size();++i)s.records[i].link0=i+1<s.records.size()?s32(i+1):-1;storage_=std::move(next);if(error)error->clear();return true;}
        catch(const std::exception&){if(error)*error="scene archive pool: uncertain initialization; no replay";return false;}
    }
    SceneArchiveAcquire acquire(SceneArchiveLease* out,std::string* error){
        if(out)*out={};
        if(!out||!ready()){if(error)*error="scene archive pool: current owner/output required";return SceneArchiveAcquire::Refused;}
        auto& s=storage_->state;if(s.free_count<=0){if(error)error->clear();return SceneArchiveAcquire::Exhausted;}
        u16 index=0;if(!scene_pool_links::take(s,&index,"scene archive pool: inconsistent free list","scene archive pool: native generation/state overflow",error))return SceneArchiveAcquire::Refused;auto& r=s.records[index];
        r.header0c_nonnull=false;r.header_owned=false;r.bank_loaded=false;r.borrowed_header=false;r.bank_hash=0;
        try{storage_->headers[index]=std::make_shared<HsdRecordAnimationBank>();}
        catch(const std::bad_alloc&){scene_pool_links::put(s,index);if(error)*error="scene archive pool: native header allocation failed";return SceneArchiveAcquire::HeapFailure;}
        r.header0c_nonnull=r.header_owned=true;out->owner_=storage_;out->index_=index;out->generation_=r.generation;if(error)error->clear();return SceneArchiveAcquire::Acquired;
    }
    bool bind_borrowed_bank(const SceneArchiveLease& lease,const HsdRecordAnimationBank& bank,std::string* error){
        if(!owns(lease)||storage_->state.records[lease.index_].bank_loaded){if(error)*error="scene archive pool: current uninitialized header required";return false;}
        HsdArchiveSceneMetadata metadata;if(!bank.archive_scene_metadata(&metadata,error))return false;
        auto& r=storage_->state.records[lease.index_];*storage_->headers[lease.index_]=bank; //same immutable bytes, NO second decode/relocation
        r.data14_present=true;r.data14_hash=bank.hash();r.scene10_source_key=metadata.scene_key;
        if(metadata.models)r.models1a=u16(metadata.models);
        if(metadata.cameras)r.cameras18=u8(metadata.cameras);
        if(metadata.fog_present)r.fog19=1;
        r.flag0a=1;r.borrowed_header=true;r.bank_loaded=true;r.bank_hash=bank.hash();if(error)error->clear();return true;
    }
    bool release(const SceneArchiveLease& lease,std::string* error){
        if(!owns(lease)){if(error)*error="scene archive pool: foreign/stale lease";return false;}
        auto& s=storage_->state;auto& r=s.records[lease.index_];
        //Loaded06C88 headers have flags3C&1:8025B254 frees only the header, not
        //borrowed buffer/tables.06E8C does NOT clear descriptor0C/10/14/18/19/1A.
        storage_->headers[lease.index_].reset();r.header_owned=false;r.bank_loaded=false;r.borrowed_header=false;r.bank_hash=0;scene_pool_links::put(s,lease.index_);if(error)error->clear();return true;
    }
    bool matches_descriptor_owner(const std::shared_ptr<SceneDescriptorPool>& owner)const{return ready()&&owner&&owner->ready()&&owner==storage_->domain&&owner->snapshot()==storage_->domain_identity;}
    const SceneArchivePoolSnapshot* snapshot()const{return ready()?&storage_->state:nullptr;}
    bool ready()const{return storage_&&storage_->current();}bool requires_scene_allocation_join()const{return true;}
    void retire(){if(storage_)storage_->live=false;storage_.reset();attempted_=true;} //native invalidation, not source bulk/global scene cleanup
private:
    bool owns(const SceneArchiveLease& lease)const{return ready()&&lease.owner_.lock()==storage_&&lease.current();}
    std::shared_ptr<scene_archive_pool_detail::Storage> storage_;bool attempted_=false;
};
} // namespace gf
