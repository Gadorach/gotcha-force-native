#pragma once
#include "engine/assets/scene_object_pool.hpp"
#include "engine/gameplay/scene_descriptor_pool.hpp"
#include "engine/gameplay/scene_physical_links.hpp"
#include <cmath>
#include <optional>
namespace gf {
struct SceneObjectParentFields {u8 kind83=0; s8 slot3e4=0;}; //declared numeric fields ONLY
struct SceneObjectHeader {u8 kind10=0,type11=0,argument12=0;u32 update0c_key=0,draw10c_key=0;bool operator==(const SceneObjectHeader&)const=default;};
struct SceneObjectRecord {
    ScenePhysicalLink link4,link8;s16 group01=0;u8 active00=0,type02=2,pool03=2,kind83=0,registration85=0,descriptor_count99=0;
    bool allocated=false;u64 generation=0;std::array<u16,8> descriptor_indices{};
    SceneObjectHeader header;bool header_installed=false;
    bool operator==(const SceneObjectRecord&)const=default;
};
struct SceneObjectPoolSnapshot {
    s32 free_head=-1,free_tail=-1;u16 free_count=0;std::array<ScenePhysicalLink,64> group_heads{},group_tails{};std::vector<SceneObjectRecord> records;
    bool operator==(const SceneObjectPoolSnapshot&)const=default;
};
struct SceneClass3ModelFields {
    u8 descriptor_count99=0;std::array<u16,8> descriptor_indices{65535,65535,65535,65535,65535,65535,65535,65535};
    SceneObjectHeader header;bool header_installed=false;
    std::optional<std::array<f32,12>> trial_frame114;
    bool operator==(const SceneClass3ModelFields&)const=default;
};
struct SceneTrialBeamShapeFields {
    ScenePhysicalLink carrier20,self5c;
    u64 carrier_generation=0;
    u8 credit_slot24=255,selector14=0,parameter11=0;
    u16 descriptor_offset28=0,carrier_matrix_offset60=0;
    s16 start48=0,end4a=0;
};
namespace scene_object_pool_detail {
struct Storage {
    bool live=true,battle_widgets_claimed=false;SceneObjectPoolProfile profile;std::shared_ptr<SceneDescriptorPool> descriptors;const SceneDescriptorPoolSnapshot* descriptor_identity=nullptr;
    SceneObjectPoolSnapshot state;std::vector<std::array<SceneDescriptorLease,8>> leases;
    ScenePhysicalBankSnapshot class3,collision;
    std::vector<SceneClass3ModelFields> class3_models;
    std::vector<std::array<SceneDescriptorLease,8>> class3_leases;
    std::vector<std::optional<SceneTrialBeamShapeFields>> trial_beam_shapes;
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
class SceneClass3Lease {
public:
    bool current()const{const auto s=owner_.lock();return s&&s->current()&&index_<s->class3.records.size()&&s->class3.records[index_].allocated&&s->class3.records[index_].generation==generation_;}
    u16 index()const{return index_;}u64 generation()const{return generation_;}
private:friend class SceneObjectPool;std::weak_ptr<scene_object_pool_detail::Storage> owner_;u16 index_=65535;u64 generation_=0;
};
class SceneCollisionLease {
public:
    bool current()const{const auto s=owner_.lock();return s&&s->current()&&index_<s->collision.records.size()&&s->collision.records[index_].allocated&&s->collision.records[index_].generation==generation_;}
    u16 index()const{return index_;}u64 generation()const{return generation_;}
private:friend class SceneObjectPool;std::weak_ptr<scene_object_pool_detail::Storage> owner_;u16 index_=65535;u64 generation_=0;
};
enum class SceneObjectAcquire {Allocated,NoDescriptors,NoObjects,Refused};
//8853C/88D98/88AA0/88E50/89814: one intrusive group store and descriptor
//owner. Only the physical bootstrap enables the class3 and class4 banks.
//No class3/class4 constructor, registry or renderer callback is installed here.
class SceneObjectPool {
public:
    SceneObjectPool()=default;SceneObjectPool(const SceneObjectPool&)=delete;SceneObjectPool& operator=(const SceneObjectPool&)=delete;
    SceneObjectPool(SceneObjectPool&&)=default;SceneObjectPool& operator=(SceneObjectPool&&)=default;~SceneObjectPool(){retire();}
    bool begin_fresh(const SceneObjectPoolProfile& profile,const std::shared_ptr<SceneDescriptorPool>& descriptors,std::string* error){
        if(attempted_||!profile.ready()||!descriptors||!descriptors->ready()||descriptors->storage_->class2_claimed){if(error)*error="scene object pool: fresh recipe/unclaimed shared descriptors required";return false;}
        attempted_=true;descriptors->storage_->class2_claimed=true; //serialized, never reset after uncertainty/retire
        try{auto s=std::make_shared<scene_object_pool_detail::Storage>();s->profile=profile;s->descriptors=descriptors;s->descriptor_identity=descriptors->snapshot();s->state.records.resize(profile.capacity());s->leases.resize(profile.capacity());s->state.free_head=0;s->state.free_tail=profile.capacity()-1;s->state.free_count=profile.capacity();
            for(unsigned i=0;i<profile.capacity();++i){auto& r=s->state.records[i];r.link4=i+1<profile.capacity()?ScenePhysicalLink{2,s32(i+1)}:ScenePhysicalLink{};r.descriptor_indices.fill(65535);}storage_=std::move(s);if(error)error->clear();return true;
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
        if(selected!=-1&&!descriptor_preflight(pool,descriptors))return refuse("descriptor chain/generation");
        const auto generation=old.generation+1;state.free_head=old.link4.index;if(state.free_head<0)state.free_tail=-1;--state.free_count;
        SceneObjectRecord r;r.descriptor_indices.fill(65535);r.generation=generation;r.allocated=true;r.group01=selected;r.registration85=255; //88D98 clear452 then restore02/03,88AA0 sets01/85
        if(selected!=-1){r.kind83=storage_->profile.kind(unsigned(selected));
            link_group(unsigned(selected),append,{2,index},r.link4,r.link8);
            for(unsigned i=0;i<descriptors;++i){const auto result=pool.acquire(&storage_->leases[index][i],error);if(result!=SceneDescriptorAcquire::Acquired){storage_->live=false;return refuse("uncertain descriptor acquisition; no replay");}r.descriptor_indices[i]=storage_->leases[index][i].index();}r.descriptor_count99=descriptors;
        }
        state.records[index]=r;out->owner_=storage_;out->index_=index;out->generation_=generation;if(error)error->clear();return SceneObjectAcquire::Allocated;
    }
    bool release(const SceneObjectLease& lease,std::string* error){
        if(!owns(lease)){if(error)*error="scene object pool: foreign/stale lease";return false;}
        auto& s=storage_->state;auto& r=s.records[lease.index_];auto& pool=*storage_->descriptors;
        if(r.registration85!=255){if(error)*error="scene object pool: renderer unregister required";return false;}
        for(unsigned i=0;i<r.descriptor_count99;++i){if(!pool.can_release(storage_->leases[lease.index_][i])){if(error)*error="scene object pool: current attached descriptor cleanup required";return false;}}
        if(r.group01!=-1){unlink_group(unsigned(r.group01),r.link4,r.link8);
            for(unsigned i=0;i<r.descriptor_count99;++i){if(!pool.release(storage_->leases[lease.index_][i],error)){storage_->live=false;return false;}}
        }
        if(s.free_tail>=0)s.records[unsigned(s.free_tail)].link4={2,lease.index_};else s.free_head=lease.index_;s.free_tail=lease.index_;r.link4={};r.active00=0;r.allocated=false;++s.free_count; //89814 FIFO; other data/link8 retained
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
    const ScenePhysicalBankSnapshot* class3_snapshot()const{return ready()&&storage_->class3.runtime_class==3?&storage_->class3:nullptr;}
    const ScenePhysicalRecord* inspect(const SceneClass3Lease& lease)const{return owns(lease)?&storage_->class3.records[lease.index_]:nullptr;}
    const SceneClass3ModelFields* class3_models(const SceneClass3Lease& lease)const{return owns(lease)?&storage_->class3_models[lease.index_]:nullptr;}
    // Caller supplies the resolved group, not an actor pointer. 8893C checks
    // descriptors before the bank even for the unlinked group -1.
    SceneObjectAcquire acquire_class3(s16 group,bool append,u8 descriptors,SceneClass3Lease* out,std::string* error){
        if(out)*out={};
        auto refuse=[&](const char* why){if(error)*error=std::string("scene class3 pool: ")+why;return SceneObjectAcquire::Refused;};
        if(!out||!class3_snapshot()||descriptors>8||group< -1||group>=64)return refuse("current class3 domain/request/group required");
        auto& bank=storage_->class3;auto& pool=*storage_->descriptors;
        if(pool.snapshot()->free_count<descriptors){if(error)error->clear();return SceneObjectAcquire::NoDescriptors;}
        if(!bank.free_count){if(error)error->clear();return SceneObjectAcquire::NoObjects;}
        const auto index=u16(bank.free_head);auto& old=bank.records[index];
        if(old.generation==std::numeric_limits<u64>::max())return refuse("native generation overflow");
        if(group!=-1&&!descriptor_preflight(pool,descriptors))return refuse("descriptor chain/generation");
        auto record=pop_physical(bank,group,append,index);
        SceneClass3ModelFields fields;
        if(group!=-1){
            for(unsigned i=0;i<descriptors;++i){
                auto& lease=storage_->class3_leases[index][i];
                if(pool.acquire(&lease,error)!=SceneDescriptorAcquire::Acquired){storage_->live=false;return refuse("uncertain descriptor acquisition; no replay");}
                fields.descriptor_indices[i]=lease.index();
            }
            fields.descriptor_count99=descriptors;
        }
        old=record;storage_->class3_models[index]=fields;
        out->owner_=storage_;out->index_=index;out->generation_=record.generation;if(error)error->clear();return SceneObjectAcquire::Allocated;
    }
    bool set_declared_descriptor_fields(const SceneClass3Lease& lease,unsigned index,const SceneDescriptorFields& fields){
        return owns(lease)&&index<storage_->class3_models[lease.index_].descriptor_count99&&
            storage_->descriptors->set_declared_fields(storage_->class3_leases[lease.index_][index],fields);
    }
    bool release(const SceneClass3Lease& lease,std::string* error){
        if(!owns(lease)){if(error)*error="scene class3 pool: foreign/stale lease";return false;}
        auto& bank=storage_->class3;auto& r=bank.records[lease.index_];auto& pool=*storage_->descriptors;
        const auto& fields=storage_->class3_models[lease.index_];
        if(r.registration85!=255||r.active00||fields.header_installed){if(error)*error="scene class3 pool: rootless/unregistered allocation required";return false;}
        for(unsigned i=0;i<fields.descriptor_count99;++i)
            if(!pool.can_release(storage_->class3_leases[lease.index_][i])){if(error)*error="scene class3 pool: current attached descriptor cleanup required";return false;}
        if(r.group01!=-1){
            unlink_group(unsigned(r.group01),r.free_next4,r.group_next8);
            for(unsigned i=0;i<fields.descriptor_count99;++i)
                if(!pool.release(storage_->class3_leases[lease.index_][i],error)){storage_->live=false;return false;}
        }
        push_physical(bank,lease.index_);if(error)error->clear();return true;
    }
    const ScenePhysicalBankSnapshot* collision_snapshot()const{return ready()&&storage_->collision.runtime_class==4?&storage_->collision:nullptr;}
    const ScenePhysicalRecord* inspect(const SceneCollisionLease& lease)const{return owns(lease)?&storage_->collision.records[lease.index_]:nullptr;}
    const SceneTrialBeamShapeFields* trial_beam_shape(const SceneCollisionLease& lease)const{
        if(!owns(lease))return nullptr;
        const auto& shape=storage_->trial_beam_shapes[lease.index_];
        if(!shape||shape->carrier20.physical!=3||!shape->carrier20.present()||
           unsigned(shape->carrier20.index)>=storage_->class3.records.size())return nullptr;
        const auto& carrier=storage_->class3.records[unsigned(shape->carrier20.index)];
        return carrier.allocated&&carrier.generation==shape->carrier_generation?&*shape:nullptr;
    }
    // Class4 uses the same group links and no model descriptors. Allocation
    // alone cannot make a body active or populate any collision registry.
    SceneObjectAcquire acquire_collision(s16 group,bool append,const std::optional<SceneObjectParentFields>& parent,
                                         SceneCollisionLease* out,std::string* error){
        if(out)*out={};
        auto refuse=[&](const char* why){if(error)*error=std::string("scene collision pool: ")+why;return SceneObjectAcquire::Refused;};
        if(!out||!collision_snapshot()||group< -1||group>=64)return refuse("current class4 domain/group required");
        s16 selected=group;
        if(group>=0&&group<56&&!(group&7)&&parent&&parent->kind83==0){
            if(parent->slot3e4<0||parent->slot3e4>=6)return refuse("ordinary physical parent slot required");
            selected=s16(group+1+parent->slot3e4);
        }
        auto& bank=storage_->collision;
        if(!bank.free_count){if(error)error->clear();return SceneObjectAcquire::NoObjects;}
        const auto index=u16(bank.free_head);auto& old=bank.records[index];
        if(old.generation==std::numeric_limits<u64>::max())return refuse("native generation overflow");
        old=pop_physical(bank,selected,append,index);storage_->trial_beam_shapes[index].reset();
        out->owner_=storage_;out->index_=index;out->generation_=old.generation;if(error)error->clear();return SceneObjectAcquire::Allocated;
    }
    bool release(const SceneCollisionLease& lease,std::string* error){
        if(!owns(lease)){if(error)*error="scene collision pool: foreign/stale lease";return false;}
        auto& bank=storage_->collision;const auto& r=bank.records[lease.index_];
        if(r.active00||r.update0c_key||r.registration85!=255){if(error)*error="scene collision pool: unbound allocation required";return false;}
        if(r.group01!=-1)unlink_group(unsigned(r.group01),r.free_next4,r.group_next8);
        push_physical(bank,lease.index_);if(error)error->clear();return true;
    }
    bool ready()const{return storage_&&storage_->current();}bool requires_scene_allocation_join()const{return true;}
    void retire(){if(storage_)storage_->live=false;storage_.reset();attempted_=true;} //native invalidation, not source bulk cleanup/descriptors-free ACK
private:
    friend class BattlePreparationWidgets;
    friend class BattlePreparationModels;
    friend class SceneModelBindings;
    friend class ScenePhysicalPools;
    friend class NativeGRedClass3TrialBridge;
    // The trial owner keeps the parent fields and provenance. This writes
    // only the class3 header into the existing allocated record.
    bool install_trial_beam_header(const SceneClass3Lease& lease){
        if(!owns(lease))return false;
        auto& record=storage_->class3.records[lease.index_];
        auto& models=storage_->class3_models[lease.index_];
        if(record.class02!=3||record.physical03!=3||record.group01!=9||record.kind83!=16||
           record.active00||record.update0c_key||record.registration85!=255||models.header_installed||
           models.descriptor_count99!=1||!storage_->class3_leases[lease.index_][0].current())return false;
        models.header={2,43,0,0x800c3cdcu,0x800c4468u};
        models.header_installed=true;record.update0c_key=models.header.update0c_key;record.active00=1;return true;
    }
    bool install_trial_beam_shape(const SceneClass3Lease& carrier,const SceneCollisionLease& child){
        if(!owns(carrier)||!owns(child))return false;
        const auto& parent=storage_->class3.records[carrier.index_];
        auto& body=storage_->collision.records[child.index_];
        auto& fields=storage_->trial_beam_shapes[child.index_];
        if(!parent.active00||parent.update0c_key!=0x800c3cdcu||parent.group01!=9||
           body.class02!=4||body.physical03!=4||body.group01!=49||body.kind83!=26||
           body.active00||body.update0c_key||body.registration85!=255||fields)return false;
        fields=SceneTrialBeamShapeFields{{3,carrier.index_},{4,child.index_},carrier.generation_,0,0,0,0x0f50,0x114,0,-2};
        body.update0c_key=0x8008a65c;body.active00=1;return true;
    }
    bool publish_trial_beam_frame(const SceneClass3Lease& carrier,const SceneCollisionLease& child,
                                  const std::array<f32,12>& frame){
        if(!owns(carrier))return false;
        const auto* shape=trial_beam_shape(child);
        auto& models=storage_->class3_models[carrier.index_];
        if(!shape||shape->carrier20!=ScenePhysicalLink{3,carrier.index_}||shape->carrier_generation!=carrier.generation_||
           !models.header_installed||models.trial_frame114)return false;
        for(f32 value:frame)if(!std::isfinite(value)||std::abs(value)>1e7f)return false;
        models.trial_frame114=frame;return true;
    }
    bool owns(const SceneObjectLease& lease)const{return ready()&&lease.owner_.lock()==storage_&&lease.current();}
    bool owns(const SceneClass3Lease& lease)const{return ready()&&lease.owner_.lock()==storage_&&lease.current();}
    bool owns(const SceneCollisionLease& lease)const{return ready()&&lease.owner_.lock()==storage_&&lease.current();}
    static bool descriptor_preflight(const SceneDescriptorPool& pool,unsigned count){
        const auto& snapshot=*pool.snapshot();s32 next=snapshot.free_head;
        for(unsigned i=0;i<count;++i){
            if(next<0||std::size_t(next)>=snapshot.records.size())return false;
            const auto& d=snapshot.records[unsigned(next)];
            if(d.allocated09||d.generation==std::numeric_limits<u64>::max())return false;
            next=d.link0;
        }
        return true;
    }
    bool initialize_class3_pool(u16 count){
        if(!ready()||storage_->class3.runtime_class!=-1||count!=512)return false;
        storage_->class3_models.resize(count);storage_->class3_leases.resize(count);
        initialize_physical(storage_->class3,3,count);return true;
    }
    bool initialize_collision_pool(u16 count){
        if(!ready()||storage_->collision.runtime_class!=-1||count!=384)return false;
        storage_->trial_beam_shapes.resize(count);
        initialize_physical(storage_->collision,4,count);return true;
    }
    static void initialize_physical(ScenePhysicalBankSnapshot& bank,u8 physical,u16 count){
        bank.records.resize(count);bank.runtime_class=s8(physical);bank.free_head=0;bank.free_tail=count-1;bank.free_count=count;
        for(unsigned i=0;i<count;++i){
            auto& r=bank.records[i];r.class02=r.physical03=physical;
            if(i+1<count)r.free_next4={physical,s32(i+1)};
        }
    }
    ScenePhysicalRecord pop_physical(ScenePhysicalBankSnapshot& bank,s16 group,bool append,u16 index){
        const auto& old=bank.records[index];bank.free_head=old.free_next4.index;if(bank.free_head<0)bank.free_tail=-1;--bank.free_count;
        ScenePhysicalRecord r;r.class02=r.physical03=u8(bank.runtime_class);r.group01=s8(group);r.registration85=255;
        r.allocated=true;r.generation=old.generation+1;
        if(group!=-1){
            r.kind83=storage_->profile.kind(unsigned(group));
            link_group(unsigned(group),append,{r.physical03,index},r.free_next4,r.group_next8);
        }
        return r;
    }
    static void push_physical(ScenePhysicalBankSnapshot& bank,u16 index){
        auto& r=bank.records[index];
        if(bank.free_tail>=0)bank.records[unsigned(bank.free_tail)].free_next4={r.physical03,index};else bank.free_head=index;
        bank.free_tail=index;r.free_next4={};r.active00=0;r.allocated=false;++bank.free_count;
    }
    ScenePhysicalLink& previous(ScenePhysicalLink link){
        if(link.physical==2)return storage_->state.records[unsigned(link.index)].link4;
        return (link.physical==3?storage_->class3:storage_->collision).records[unsigned(link.index)].free_next4;
    }
    ScenePhysicalLink& next(ScenePhysicalLink link){
        if(link.physical==2)return storage_->state.records[unsigned(link.index)].link8;
        return (link.physical==3?storage_->class3:storage_->collision).records[unsigned(link.index)].group_next8;
    }
    void link_group(unsigned group,bool append,ScenePhysicalLink self,ScenePhysicalLink& prev,ScenePhysicalLink& following){
        auto& s=storage_->state;
        if(!append){prev={};following=s.group_heads[group];if(following.present())previous(following)=self;else s.group_tails[group]=self;s.group_heads[group]=self;}
        else{following={};prev=s.group_tails[group];if(prev.present())next(prev)=self;else s.group_heads[group]=self;s.group_tails[group]=self;}
    }
    void unlink_group(unsigned group,ScenePhysicalLink prev,ScenePhysicalLink following){
        auto& s=storage_->state;
        if(prev.present())next(prev)=following;else s.group_heads[group]=following;
        if(following.present())previous(following)=prev;else s.group_tails[group]=prev;
    }
    std::shared_ptr<scene_object_pool_detail::Storage> storage_;bool attempted_=false;
};
} // namespace gf
