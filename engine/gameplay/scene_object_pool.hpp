#pragma once
#include "engine/assets/scene_object_pool.hpp"
#include "engine/gameplay/scene_descriptor_pool.hpp"
#include <optional>
namespace gf {
struct SceneObjectParentFields {u8 kind83=0; s8 slot3e4=0;}; //declared numeric fields ONLY
struct SceneObjectHeader {u8 kind10=0,type11=0,argument12=0;u32 update0c_key=0,draw10c_key=0;bool operator==(const SceneObjectHeader&)const=default;};
struct SceneObjectRecord {
    s32 link4=-1,link8=-1;s16 group01=0;u8 active00=0,type02=2,pool03=2,kind83=0,registration85=0,descriptor_count99=0;
    bool allocated=false;u64 generation=0;std::array<u16,8> descriptor_indices{};
    SceneObjectHeader header;bool header_installed=false;
    bool operator==(const SceneObjectRecord&)const=default;
};
struct SceneObjectPoolSnapshot {
    s32 free_head=-1,free_tail=-1;u16 free_count=0;std::array<s32,64> group_heads,group_tails;std::vector<SceneObjectRecord> records;
    SceneObjectPoolSnapshot(){group_heads.fill(-1);group_tails.fill(-1);}
    bool operator==(const SceneObjectPoolSnapshot&)const=default;
};
namespace scene_object_pool_detail {
struct Storage {
    bool live=true,battle_widgets_claimed=false;SceneObjectPoolProfile profile;std::shared_ptr<SceneDescriptorPool> descriptors;const SceneDescriptorPoolSnapshot* descriptor_identity=nullptr;
    SceneObjectPoolSnapshot state;std::vector<std::array<SceneDescriptorLease,8>> leases;
    bool current()const{return live&&descriptors&&descriptors->ready()&&descriptors->snapshot()==descriptor_identity;}
};
}
class SceneObjectPool;
class SceneObjectLease {
public:
    bool current()const{const auto s=owner_.lock();return s&&s->current()&&index_<s->state.records.size()&&s->state.records[index_].allocated&&s->state.records[index_].generation==generation_;}
    u16 index()const{return index_;}u64 generation()const{return generation_;}
private:friend class SceneObjectPool;std::weak_ptr<scene_object_pool_detail::Storage> owner_;u16 index_=65535;u64 generation_=0;
};
enum class SceneObjectAcquire {Allocated,NoDescriptors,NoObjects,Refused};
//Fresh rootless class2 owner:8853C/88D98/88AA0/88E50/89814. Owns ACTUAL
//descriptor leases from ONE shared descriptor owner. No child constructor,
//renderer callback registration, source pointer bits, current scene or actor ACK.
class SceneObjectPool {
public:
    SceneObjectPool()=default;SceneObjectPool(const SceneObjectPool&)=delete;SceneObjectPool& operator=(const SceneObjectPool&)=delete;
    SceneObjectPool(SceneObjectPool&&)=default;SceneObjectPool& operator=(SceneObjectPool&&)=default;~SceneObjectPool(){retire();}
    bool begin_fresh(const SceneObjectPoolProfile& profile,const std::shared_ptr<SceneDescriptorPool>& descriptors,std::string* error){
        if(attempted_||!profile.ready()||!descriptors||!descriptors->ready()||descriptors->storage_->class2_claimed){if(error)*error="scene object pool: fresh recipe/unclaimed shared descriptors required";return false;}
        attempted_=true;descriptors->storage_->class2_claimed=true; //serialized, never reset after uncertainty/retire
        try{auto s=std::make_shared<scene_object_pool_detail::Storage>();s->profile=profile;s->descriptors=descriptors;s->descriptor_identity=descriptors->snapshot();s->state.records.resize(profile.capacity());s->leases.resize(profile.capacity());s->state.free_head=0;s->state.free_tail=profile.capacity()-1;s->state.free_count=profile.capacity();
            for(unsigned i=0;i<profile.capacity();++i){auto& r=s->state.records[i];r.link4=i+1<profile.capacity()?s32(i+1):-1;r.descriptor_indices.fill(65535);}storage_=std::move(s);if(error)error->clear();return true;
        }catch(const std::exception&){if(error)*error="scene object pool: uncertain fresh allocation; no replay";return false;}
    }
    SceneObjectAcquire acquire(s16 group,bool append,u8 descriptors,const std::optional<SceneObjectParentFields>& parent,SceneObjectLease* out,std::string* error){
        if(out)*out={};
        auto refuse=[&](const char* why){if(error)*error=std::string("scene object pool: ")+why;return SceneObjectAcquire::Refused;};
        if(!out||!ready()||descriptors>8||group< -1||group>=64)return refuse("owner/request/group domain");
        s16 selected=group;if(group>=0&&group<56&&!(group&7)&&parent&&parent->kind83==0){if(parent->slot3e4<0||parent->slot3e4>=6)return refuse("ordinary physical parent slot required");selected=s16(group+1+parent->slot3e4);}
        auto& state=storage_->state;auto& pool=*storage_->descriptors;
        //88AA0 checks shared free count BEFORE touching the class2 free list,
        //even for group−1, where it later returns without descriptor creation.
        if(pool.snapshot()->free_count<descriptors){if(error)error->clear();return SceneObjectAcquire::NoDescriptors;}
        if(!state.free_count){if(error)error->clear();return SceneObjectAcquire::NoObjects;}
        const auto index=u16(state.free_head);auto& old=state.records[index];if(old.generation==std::numeric_limits<u64>::max())return refuse("native generation overflow");
        //Fixed06F60 hook cannot fail/reenter; preflight native generation guards
        //before any source work. No fake failed allocations or descriptor debt.
        if(selected!=-1){s32 next=pool.snapshot()->free_head;for(unsigned i=0;i<descriptors;++i){if(next<0||std::size_t(next)>=pool.snapshot()->records.size())return refuse("inconsistent descriptor chain");const auto& d=pool.snapshot()->records[unsigned(next)];if(d.allocated09||d.generation==std::numeric_limits<u64>::max())return refuse("descriptor generation/state");next=d.link0;}}
        const auto generation=old.generation+1;state.free_head=old.link4;if(state.free_head<0)state.free_tail=-1;--state.free_count;
        SceneObjectRecord r;r.descriptor_indices.fill(65535);r.generation=generation;r.allocated=true;r.group01=selected;r.registration85=255; //88D98 clear452 then restore02/03,88AA0 sets01/85
        if(selected!=-1){r.kind83=storage_->profile.kind(unsigned(selected));const unsigned g=unsigned(selected);
            if(!append){r.link4=-1;r.link8=state.group_heads[g];if(r.link8>=0)state.records[unsigned(r.link8)].link4=index;else state.group_tails[g]=index;state.group_heads[g]=index;}
            else{r.link8=-1;r.link4=state.group_tails[g];if(r.link4>=0)state.records[unsigned(r.link4)].link8=index;else state.group_heads[g]=index;state.group_tails[g]=index;}
            for(unsigned i=0;i<descriptors;++i){const auto result=pool.acquire(&storage_->leases[index][i],error);if(result!=SceneDescriptorAcquire::Acquired){storage_->live=false;return refuse("uncertain descriptor acquisition; no replay");}r.descriptor_indices[i]=storage_->leases[index][i].index();}r.descriptor_count99=descriptors;
        }
        state.records[index]=r;out->owner_=storage_;out->index_=index;out->generation_=generation;if(error)error->clear();return SceneObjectAcquire::Allocated;
    }
    bool release(const SceneObjectLease& lease,std::string* error){
        if(!owns(lease)){if(error)*error="scene object pool: foreign/stale lease";return false;}
        auto& s=storage_->state;auto& r=s.records[lease.index_];auto& pool=*storage_->descriptors;
        if(r.registration85!=255){if(error)*error="scene object pool: renderer unregister required";return false;}
        for(unsigned i=0;i<r.descriptor_count99;++i){if(!pool.can_release(storage_->leases[lease.index_][i])){if(error)*error="scene object pool: current attached descriptor cleanup required";return false;}}
        if(r.group01!=-1){const unsigned group=unsigned(r.group01);if(r.link4>=0)s.records[unsigned(r.link4)].link8=r.link8;else s.group_heads[group]=r.link8;if(r.link8>=0)s.records[unsigned(r.link8)].link4=r.link4;else s.group_tails[group]=r.link4;
            for(unsigned i=0;i<r.descriptor_count99;++i){if(!pool.release(storage_->leases[lease.index_][i],error)){storage_->live=false;return false;}}
        }
        if(s.free_tail>=0)s.records[unsigned(s.free_tail)].link4=lease.index_;else s.free_head=lease.index_;s.free_tail=lease.index_;r.link4=-1;r.active00=0;r.allocated=false;++s.free_count; //89814 FIFO; other data/link8 retained
        if(error){error->clear();}return true;
    }
    //Source constructor header stores, not callback execution/renderer binding.
    bool install_header(const SceneObjectLease& lease,const SceneObjectHeader& fields){
        if(!owns(lease)||!fields.update0c_key||!fields.draw10c_key||(fields.update0c_key&3)||(fields.draw10c_key&3))return false;
        auto& r=storage_->state.records[lease.index_];if(r.header_installed||r.active00)return false;
        r.active00=1;r.header.kind10=fields.kind10;r.header.update0c_key=fields.update0c_key;r.header.draw10c_key=fields.draw10c_key;r.header.type11=fields.type11;r.header.argument12=fields.argument12;r.header_installed=true;return true;
    }
    bool set_declared_descriptor_fields(const SceneObjectLease& lease,unsigned index,const SceneDescriptorFields& fields){return owns(lease)&&index<storage_->state.records[lease.index_].descriptor_count99&&storage_->descriptors->set_declared_fields(storage_->leases[lease.index_][index],fields);}
    const SceneObjectRecord* inspect(const SceneObjectLease& lease)const{return owns(lease)?&storage_->state.records[lease.index_]:nullptr;}
    const SceneObjectPoolSnapshot* snapshot()const{return ready()?&storage_->state:nullptr;}
    bool ready()const{return storage_&&storage_->current();}bool requires_scene_allocation_join()const{return true;}
    void retire(){if(storage_)storage_->live=false;storage_.reset();attempted_=true;} //native invalidation, not source bulk cleanup/descriptors-free ACK
private:
    friend class BattlePreparationWidgets;
    friend class BattlePreparationModels;
    friend class SceneModelBindings;
    bool owns(const SceneObjectLease& lease)const{return ready()&&lease.owner_.lock()==storage_&&lease.current();}
    std::shared_ptr<scene_object_pool_detail::Storage> storage_;bool attempted_=false;
};
} // namespace gf
