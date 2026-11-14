#pragma once
#include "engine/assets/scene_descriptor_pool.hpp"
#include "engine/gameplay/scene_pool_links.hpp"
#include "engine/assets/hsd_joint_instances.hpp"
#include <array>
#include <limits>
#include <memory>
#include <vector>
namespace gf {
//Native typed relationships, NOT source pointer encodings/44-byte images. The
//source field0 is free-next OR live-previous. Field4 remains stale after release.
struct SceneDescriptorFields {u32 unresolved_root_key0c=0;std::array<u8,16> payload10_1f{};std::array<u32,3> words20_24_28{};u8 flag0b=0;bool operator==(const SceneDescriptorFields&)const=default;};
struct SceneDescriptorRecord {s32 link0=-1,link4=-1;u8 pool08=0,allocated09=0,flag0a=0;u64 generation=0;SceneDescriptorFields fields;bool native_root0c=false;bool operator==(const SceneDescriptorRecord&)const=default;};
struct SceneDescriptorPoolSnapshot {s32 free_head=-1,live_head=-1;s16 free_count=0;std::vector<SceneDescriptorRecord> records;bool operator==(const SceneDescriptorPoolSnapshot&)const=default;};
namespace scene_descriptor_pool_detail {struct Storage {bool live=true,class2_claimed=false,archive_pool1_claimed=false;SceneDescriptorPoolSnapshot state;std::vector<HsdJointInstanceOwner> roots;};}
class SceneDescriptorPool;
class SceneDescriptorLease {
public:
    bool current()const{const auto s=owner_.lock();return s&&s->live&&index_<s->state.records.size()&&s->state.records[index_].allocated09&&s->state.records[index_].generation==generation_;}
    u16 index()const{return index_;}u64 generation()const{return generation_;}
private:friend class SceneDescriptorPool;std::weak_ptr<scene_descriptor_pool_detail::Storage> owner_;u16 index_=0xffff;u64 generation_=0;
};
enum class SceneDescriptorAcquire {Acquired,Exhausted,Refused};
//Serialized fresh/rootless pool owner for97950→197A0C/06D4C→06D80/197AD8.
//No live battle allocation capability: preparation widgets/other users have NOT
//reserved these records, and attached HSD roots/renderer cleanup remain refused.
class SceneDescriptorPool {
public:
    SceneDescriptorPool()=default;SceneDescriptorPool(const SceneDescriptorPool&)=delete;SceneDescriptorPool& operator=(const SceneDescriptorPool&)=delete;
    SceneDescriptorPool(SceneDescriptorPool&&)=default;SceneDescriptorPool& operator=(SceneDescriptorPool&&)=default;~SceneDescriptorPool(){retire();}
    bool begin_fresh(const SceneDescriptorPoolProfile& profile,std::string* error){
        if(attempted_||!profile.ready()){if(error)*error="scene descriptor pool: fresh owner/qualified recipe required";return false;}
        attempted_=true;try{auto next=std::make_shared<scene_descriptor_pool_detail::Storage>();auto& s=next->state;s.records.resize(profile.capacity());next->roots.resize(profile.capacity());s.free_count=s16(profile.capacity());s.free_head=0;
            for(unsigned i=0;i<s.records.size();++i)s.records[i].link0=i+1<s.records.size()?s32(i+1):-1;
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){if(error)*error="scene descriptor pool: uncertain allocation; no replay";return false;}
    }
    SceneDescriptorAcquire acquire(SceneDescriptorLease* out,std::string* error){
        if(out)*out={};
        if(!out||!ready()){if(error)*error="scene descriptor pool: current owner/output required";return SceneDescriptorAcquire::Refused;}
        auto& s=storage_->state;if(s.free_count<=0){if(error)error->clear();return SceneDescriptorAcquire::Exhausted;}
        u16 index=0;if(!scene_pool_links::take(s,&index,"scene descriptor pool: inconsistent free list","scene descriptor pool: stale allocation/generation overflow",error))return SceneDescriptorAcquire::Refused;auto& r=s.records[index];
        //06F60 initializer succeeds (returns0);06D4C then clears root0C only.
        r.fields.words20_24_28.fill(0);r.fields.unresolved_root_key0c=0;
        out->owner_=storage_;out->index_=index;out->generation_=r.generation;if(error)error->clear();return SceneDescriptorAcquire::Acquired;
    }
    bool release(const SceneDescriptorLease& lease,std::string* error){
        if(!owns(lease)){if(error)*error="scene descriptor pool: foreign/stale lease";return false;}
        auto& s=storage_->state;auto& r=s.records[lease.index_];
        //Both06D80 and06ED8 would call HSD release for a nonnull root. Never
        //clear it to fabricate successful cleanup or dereference a source key.
        if(r.fields.unresolved_root_key0c){if(error)*error="scene descriptor pool: attached HSD root cleanup required";return false;}
        if(r.native_root0c){if(!can_release(lease)){if(error)*error="scene descriptor pool: non-metadata root cleanup required";return false;}storage_->roots[lease.index_].retire();r.native_root0c=false;}
        scene_pool_links::put(s,lease.index_);if(error)error->clear();return true;
    }
    //Declared field seam for source consumers/tests, not an HSD binding or actor
    //receipt. Nonzero root keys deliberately prevent release without that owner.
    bool set_declared_fields(const SceneDescriptorLease& lease,const SceneDescriptorFields& fields){if(!owns(lease)||storage_->state.records[lease.index_].native_root0c)return false;auto& current=storage_->state.records[lease.index_].fields;if(current.unresolved_root_key0c&&current.unresolved_root_key0c!=fields.unresolved_root_key0c)return false;current=fields;return true;}
    bool can_release(const SceneDescriptorLease& lease)const{if(!owns(lease)||storage_->state.records[lease.index_].fields.unresolved_root_key0c)return false;const auto& r=storage_->state.records[lease.index_];const auto* root=storage_->roots[lease.index_].state();return !r.native_root0c||(root&&!root->animation_requested&&!root->world_evaluated);}
    const HsdJointInstanceOwner* root(const SceneDescriptorLease& lease)const{return owns(lease)&&storage_->state.records[lease.index_].native_root0c?&storage_->roots[lease.index_]:nullptr;}
    const SceneDescriptorRecord* inspect(const SceneDescriptorLease& lease)const{return owns(lease)?&storage_->state.records[lease.index_]:nullptr;}
    const SceneDescriptorPoolSnapshot* snapshot()const{return ready()?&storage_->state:nullptr;}
    bool ready()const{return storage_&&storage_->live;}bool requires_scene_allocation_join()const{return true;}
    void retire(){if(storage_)storage_->live=false;storage_.reset();attempted_=true;} //native invalidation, NOT a source HSD cleanup ACK
private:
    friend class SceneObjectPool;
    friend class SceneArchivePool;
    friend class SceneModelBindings;
    friend class SceneDescriptorBootstrap;
    bool owns(const SceneDescriptorLease& lease)const{return ready()&&lease.owner_.lock()==storage_&&lease.current();}
    std::shared_ptr<scene_descriptor_pool_detail::Storage> storage_;bool attempted_=false;
};
} // namespace gf
