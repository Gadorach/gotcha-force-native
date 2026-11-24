#pragma once
#include "engine/assets/scene_object_pool.hpp"
#include "engine/gameplay/scene_descriptor_pool.hpp"
#include "engine/gameplay/scene_actor_reference.hpp"
#include "engine/gameplay/scene_physical_links.hpp"
#include "engine/gameplay/collider_sphere_shape.hpp"
#include "engine/gameplay/battle_common_resources.hpp"
#include "engine/gameplay/scene_view_reference.hpp"
#include "engine/gameplay/collider_initial_shape.hpp"
#include "engine/gameplay/battle_initial_point_child.hpp"
#include "engine/gameplay/battle_initial_field_child.hpp"
#include "engine/gameplay/startup_attachment_state.hpp"
#include "engine/gameplay/battle_initial_shape_child.hpp"
#include "engine/gameplay/startup_auxiliary.hpp"
#include "engine/gameplay/scene_scratch_buffer.hpp"
#include "engine/gameplay/battle_initial_briefing_widget.hpp"
#include "engine/gameplay/battle_initial_briefing_type0.hpp"
#include "engine/gameplay/battle_initial_briefing_type1.hpp"
#include "engine/gameplay/battle_initial_briefing_type2.hpp"
#include "engine/gameplay/battle_initial_briefing_type5.hpp"
#include "engine/gameplay/battle_initial_briefing_type6.hpp"
#include "engine/gameplay/battle_initial_world_dispatch.hpp"
#include "engine/gameplay/battle_initial_shape_world.hpp"
#include <cmath>
#include <optional>
namespace gf {
struct SceneObjectParentFields {u8 kind83=0; s8 slot3e4=0;}; //declared numeric fields ONLY
struct SceneObjectHeader {u8 kind10=0,type11=0,argument12=0;u32 update0c_key=0,draw10c_key=0;bool operator==(const SceneObjectHeader&)const=default;};
struct SceneObjectActorChildFields {
    SceneActorReference parent8c,parent90;
    u16 model94=0;u8 index96=0,resource97=0,team88=0,fieldd0=0,field13=0,field89=0;
    std::array<u32,2> words_c8_cc{};u32 word54=0;
    f32 scale_b8=0,scale_b4=0,fieldc0=0,fieldc4=0;std::array<f32,3> scale{};
    std::optional<SceneActorFieldReference> field144,field148;
    bool operator==(const SceneObjectActorChildFields&)const=default;
};
struct ScenePointChildFirstUpdate {
    u8 phase18=0,render82=0,flags84=0,bone89=0;u64 generation=0;
    BattleInitialPointChildNumbers numbers;bool initialized=false;
    bool operator==(const ScenePointChildFirstUpdate&)const=default;
};
struct SceneFieldChildFirstUpdate {
    u8 phase18=0,render82=0,flags84=0,bone89=0,fielda8=0;u64 generation=0;
    BattleInitialFieldChildNumbers numbers;bool initialized=false;
    bool operator==(const SceneFieldChildFirstUpdate&)const=default;
};
struct SceneBankChildFirstUpdate {
    StartupAttachmentState state;u16 record=0;u8 type11=0;u64 generation=0;
    bool initialized=false;
    //Read-only initial source draw-branch observations, NOT a draw/publication.
    bool draw_branch_visible=false,draw_branch_alternate=false;
    bool operator==(const SceneBankChildFirstUpdate&)const=default;
};
struct SceneShapeChildFirstUpdate {
    BattleInitialShapeNumbers numbers;u8 phase18=0,render82=0;u64 generation=0,light_constructor_generation=0;
    bool initialized=false;
    std::optional<BattleInitialShapeWorld> first_world;
    bool operator==(const SceneShapeChildFirstUpdate&)const=default;
};
struct SceneAuxiliaryInlineMatrixReference {
    ScenePhysicalLink owner;u64 generation=0;u16 source_field=0x144;u8 count=2;
    bool operator==(const SceneAuxiliaryInlineMatrixReference&)const=default;
};
struct SceneCommonAuxiliaryFirstUpdate {
    StartupAuxiliaryState state;SceneAuxiliaryInlineMatrixReference matrix54;
    u64 generation=0;bool initialized=false,scratch_allocated=false;
    //No raw host pointer is stored in actor_child.word54 or a guest word.
    bool operator==(const SceneCommonAuxiliaryFirstUpdate&)const=default;
};
struct SceneBarChildFirstUpdate {
    u8 phase18=0,render82=0,bone89=0;u16 counter1c=0,mask1e=0;
    std::optional<SceneScratchReference> scratch_dc,buffer54;
    SceneScratchAcquire allocation=SceneScratchAcquire::Refused;
    u64 generation=0;bool initialized=false;
    bool operator==(const SceneBarChildFirstUpdate&)const=default;
};
struct SceneHudChildFirstUpdate {
    u8 phase18=0,render82=0;u16 yaw74=0;
    //Source144..15F: exact sentinels/clears; other fresh bytes stay zero.
    std::array<u8,28> controls144{};
    bool empty_animation_traversed=false,animation_requested=false,initialized=false;
    u64 generation=0;
    bool operator==(const SceneHudChildFirstUpdate&)const=default;
};
struct SceneObjectRecord {
    ScenePhysicalLink link4,link8;s16 group01=0;u8 active00=0,type02=2,pool03=2,kind83=0,registration85=0,descriptor_count99=0;
    ScenePhysicalLink render_next110;
    u32 callback100_key=0;ScenePhysicalLink callback_previous104,callback_next108;
    bool allocated=false;u64 generation=0;std::array<u16,8> descriptor_indices{};
    SceneObjectHeader header;bool header_installed=false;
    std::optional<SceneObjectActorChildFields> actor_child;
    std::optional<ScenePointChildFirstUpdate> point_child_update;
    std::optional<SceneFieldChildFirstUpdate> field_child_update;
    std::optional<SceneBankChildFirstUpdate> bank_child_update;
    std::optional<SceneShapeChildFirstUpdate> shape_child_update;
    std::optional<SceneCommonAuxiliaryFirstUpdate> common_auxiliary_update;
    std::optional<SceneBarChildFirstUpdate> bar_child_update;
    std::optional<SceneHudChildFirstUpdate> hud_child_update;
    std::optional<BattleInitialBriefingWidgetPrefix> briefing_widget_prefix;
    std::optional<BattleInitialBriefingType0State> briefing_type0_update;
    std::optional<BattleInitialBriefingType1State> briefing_type1_update;
    std::optional<BattleInitialBriefingType2State> briefing_type2_update;
    std::optional<BattleInitialBriefingType5State> briefing_type5_update;
    std::optional<BattleInitialBriefingType6State> briefing_type6_update;
    std::optional<SceneObjectWorldCallbackState> world_callback;
    bool operator==(const SceneObjectRecord&)const=default;
};
struct SceneObjectPoolSnapshot {
    s32 free_head=-1,free_tail=-1;u16 free_count=0;std::array<ScenePhysicalLink,64> group_heads{},group_tails{};std::vector<SceneObjectRecord> records;
    bool operator==(const SceneObjectPoolSnapshot&)const=default;
};
enum class SceneActorMatrixSlotKind {Bone,Body1aa4};
struct SceneCollisionFields {
    SceneActorReference parent20,parent24,descriptor_owner28,parameter_owner2c,matrix_owner60;
    u32 descriptor_offset28=0,parameter_source_key=0;
    ScenePhysicalLink self5c;SceneActorMatrixSlotKind matrix_kind=SceneActorMatrixSlotKind::Bone;
    u8 bone=0,parameter11=0,record12=0;s16 start48=0,end4a=0;
    bool operator==(const SceneCollisionFields&)const=default;
};
struct SceneClass3ModelFields {
    u8 descriptor_count99=0;std::array<u16,8> descriptor_indices{65535,65535,65535,65535,65535,65535,65535,65535};
    SceneObjectHeader header;bool header_installed=false;
    std::optional<std::array<f32,12>> trial_frame114;
    u16 trial_token26c=0;
    u8 trial_once1ca=0,trial_byte2ad=0;
    bool operator==(const SceneClass3ModelFields&)const=default;
};
struct SceneTrialBeamShapeFields {
    ScenePhysicalLink carrier20,self5c;
    u64 carrier_generation=0;
    u8 credit_slot24=255,selector14=0,parameter11=0;
    u16 descriptor_offset28=0,carrier_matrix_offset60=0;
    s16 start48=0,end4a=0;
    std::optional<ColliderSphereSeed> sphere_seed{};
    u8 trial_phase10=0;
};
class SceneStageCollisionFields;
struct SceneViewCollisionFields {
    SceneViewReference parent20,parent24,matrix_owner60;BattleCommonResourceBinding source;
    u32 descriptor_offset28=0,parameter_key2c=0;ScenePhysicalLink self5c;
    u8 selector14=0,record12=0,parameter11=0;s16 start48=0,end4a=0;
    ByteView descriptor()const{const auto b=source.block(0);return parent20.live()&&parent24.live()&&b.contains(descriptor_offset28,0x50)?ByteView{b.data+descriptor_offset28,0x50}:ByteView{};}
};
namespace scene_object_pool_detail {
struct Storage {
    bool live=true,battle_widgets_claimed=false;SceneObjectPoolProfile profile;std::shared_ptr<SceneDescriptorPool> descriptors;const SceneDescriptorPoolSnapshot* descriptor_identity=nullptr;
    SceneObjectPoolSnapshot state;std::vector<std::array<SceneDescriptorLease,8>> leases;
    ScenePhysicalBankSnapshot collision;std::vector<std::optional<SceneCollisionFields>> collision_fields;
    std::vector<std::shared_ptr<const SceneStageCollisionFields>> stage_collision_fields;
    std::vector<std::shared_ptr<const SceneViewCollisionFields>> view_collision_fields;
    std::vector<std::optional<ColliderInitialPlan>> collision_initial;
    ScenePhysicalBankSnapshot class3;std::vector<SceneClass3ModelFields> class3_models;std::vector<std::array<SceneDescriptorLease,8>> class3_leases;
    std::vector<std::optional<SceneTrialBeamShapeFields>> trial_beam_shapes;
    std::vector<std::optional<ColliderSphereSeed>> trial_sasuke_bodies;
    u16 trial_scene_token=0;
    u64 collision_allocation_revision=0;
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
class SceneCollisionLease {
public:
    bool current()const{const auto s=owner_.lock();return s&&s->current()&&index_<s->collision.records.size()&&s->collision.records[index_].allocated&&s->collision.records[index_].generation==generation_;}
    u16 index()const{return index_;}u64 generation()const{return generation_;}
private:friend class SceneObjectPool;std::weak_ptr<scene_object_pool_detail::Storage> owner_;u16 index_=65535;u64 generation_=0;
};
class SceneClass3Lease {
public:
    bool current()const{const auto s=owner_.lock();return s&&s->current()&&index_<s->class3.records.size()&&s->class3.records[index_].allocated&&s->class3.records[index_].generation==generation_;}
    u16 index()const{return index_;}u64 generation()const{return generation_;}
private:friend class SceneObjectPool;std::weak_ptr<scene_object_pool_detail::Storage> owner_;u16 index_=65535;u64 generation_=0;
};
class SceneStageObjectLifetime {
public:bool live()const{return live_;}
private:friend class BattleStageConstruction;bool live_=true;
};
class SceneClass3Reference {
public:
    bool live()const{const auto s=scope_.lock();return s&&s->live()&&lease_.current();}
    ScenePhysicalLink identity()const{return live()?ScenePhysicalLink{3,lease_.index()}:ScenePhysicalLink{};}
    bool operator==(const SceneClass3Reference& other)const{return !scope_.owner_before(other.scope_)&&!other.scope_.owner_before(scope_)&&lease_.index()==other.lease_.index()&&lease_.generation()==other.lease_.generation();}
private:friend class BattleStageConstruction;friend class SceneObjectPool;SceneClass3Lease lease_;std::weak_ptr<const SceneStageObjectLifetime> scope_;
};
class SceneStageCollisionFields {
public:
    SceneClass3Reference parent20,parent24,matrix_owner60;
    BattleCommonResourceBinding source;u32 descriptor_offset28=0,parameter_key2c=0;
    ScenePhysicalLink self5c;u8 selector14=0,parameter11=0,record12=0;s16 start48=0,end4a=0;
    //Reference to parent114, NOT an acknowledgement that the matrix was written.
    ByteView descriptor()const{const auto b=source.block(0);return parent20.live()&&parent24.live()&&b.contains(descriptor_offset28,0x50)?ByteView{b.data+descriptor_offset28,0x50}:ByteView{};}
};
enum class SceneObjectAcquire {Allocated,NoDescriptors,NoObjects,Refused};
//Shared class2/3/4 owner:8853C/88D98/8893C/88AA0/88E50/89814. Owns ACTUAL
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
        if(selected!=-1){s32 next=pool.snapshot()->free_head;for(unsigned i=0;i<descriptors;++i){if(next<0||std::size_t(next)>=pool.snapshot()->records.size())return refuse("inconsistent descriptor chain");const auto& d=pool.snapshot()->records[unsigned(next)];if(d.allocated09||d.generation==std::numeric_limits<u64>::max())return refuse("descriptor generation/state");next=d.link0;}}
        const auto generation=old.generation+1;state.free_head=old.link4.index;if(state.free_head<0)state.free_tail=-1;--state.free_count;
        SceneObjectRecord r;r.descriptor_indices.fill(65535);r.generation=generation;r.allocated=true;r.group01=selected;r.registration85=255; //88D98 clear452 then restore02/03,88AA0 sets01/85
        if(selected!=-1){r.kind83=storage_->profile.kind(unsigned(selected));const unsigned g=unsigned(selected);
            link_group(g,append,{2,index},r.link4,r.link8);
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
    const SceneObjectActorChildFields* actor_child(const SceneObjectLease& lease)const{const auto* r=inspect(lease);return r&&r->actor_child&&r->actor_child->parent8c.live()&&r->actor_child->parent90.live()?&*r->actor_child:nullptr;}
    bool matches_descriptor_owner(const std::shared_ptr<SceneDescriptorPool>& owner)const{return ready()&&owner&&owner==storage_->descriptors&&owner->snapshot()==storage_->descriptor_identity;}
    const SceneObjectPoolSnapshot* snapshot()const{return ready()?&storage_->state:nullptr;}
    const ScenePhysicalBankSnapshot* class3_snapshot()const{return ready()&&storage_->class3.runtime_class==3?&storage_->class3:nullptr;}
    const ScenePhysicalRecord* inspect(const SceneClass3Lease& lease)const{return owns(lease)?&storage_->class3.records[lease.index_]:nullptr;}
    const SceneClass3ModelFields* class3_models(const SceneClass3Lease& lease)const{return owns(lease)?&storage_->class3_models[lease.index_]:nullptr;}
    //8893C(3,...): same descriptor precheck and group store as class2.
    //This factory allocation is not a side-object/prop constructor receipt.
    SceneObjectAcquire acquire_class3(s16 group,bool append,u8 descriptors,SceneClass3Lease* out,std::string* error){
        if(out)*out={};
        auto refuse=[&](const char* why){if(error)*error=std::string("scene class3 pool: ")+why;return SceneObjectAcquire::Refused;};
        if(!out||!class3_snapshot()||descriptors>8||group< -1||group>=64)return refuse("current class3 domain/request/group required");
        auto& bank=storage_->class3;auto& pool=*storage_->descriptors;
        //Even group−1 and an exhausted object bank check descriptors first.
        if(pool.snapshot()->free_count<descriptors){if(error)error->clear();return SceneObjectAcquire::NoDescriptors;}
        if(!bank.free_count){if(error)error->clear();return SceneObjectAcquire::NoObjects;}
        const auto index=u16(bank.free_head);auto& old=bank.records[index];if(old.generation==std::numeric_limits<u64>::max())return refuse("native generation overflow");
        if(group!=-1){s32 next=pool.snapshot()->free_head;for(unsigned i=0;i<descriptors;++i){if(next<0||std::size_t(next)>=pool.snapshot()->records.size())return refuse("inconsistent descriptor chain");const auto& d=pool.snapshot()->records[unsigned(next)];if(d.allocated09||d.generation==std::numeric_limits<u64>::max())return refuse("descriptor generation/state");next=d.link0;}}
        const auto generation=old.generation+1;bank.free_head=old.free_next4.index;if(bank.free_head<0)bank.free_tail=-1;--bank.free_count;
        ScenePhysicalRecord r;r.class02=r.physical03=3;r.group01=s8(group);r.registration85=255;r.allocated=true;r.generation=generation;
        SceneClass3ModelFields fields;
        if(group!=-1){
            r.kind83=storage_->profile.kind(unsigned(group));link_group(unsigned(group),append,{3,index},r.free_next4,r.group_next8);
            for(unsigned i=0;i<descriptors;++i){if(pool.acquire(&storage_->class3_leases[index][i],error)!=SceneDescriptorAcquire::Acquired){storage_->live=false;return refuse("uncertain descriptor acquisition; consumed prefix retained");}fields.descriptor_indices[i]=storage_->class3_leases[index][i].index();}
            fields.descriptor_count99=descriptors;
        }
        old=r;storage_->class3_models[index]=fields;out->owner_=storage_;out->index_=index;out->generation_=generation;if(error)error->clear();return SceneObjectAcquire::Allocated;
    }
    bool set_declared_descriptor_fields(const SceneClass3Lease& lease,unsigned index,const SceneDescriptorFields& fields){return owns(lease)&&index<storage_->class3_models[lease.index_].descriptor_count99&&storage_->descriptors->set_declared_fields(storage_->class3_leases[lease.index_][index],fields);}
    bool release(const SceneClass3Lease& lease,std::string* error){
        if(!owns(lease)){if(error)*error="scene class3 pool: foreign/stale lease";return false;}
        auto& bank=storage_->class3;auto& r=bank.records[lease.index_];const auto& fields=storage_->class3_models[lease.index_];auto& pool=*storage_->descriptors;
        if(r.registration85!=255){if(error)*error="scene class3 pool: renderer unregister required";return false;}
        if(fields.header_installed&&fields.header.update0c_key==0x800c3cdcu){if(error)*error="scene class3 pool: beam cleanup consumer required";return false;}
        for(const auto& child:storage_->stage_collision_fields)if(child&&owns(child->parent20.lease_)&&child->parent20.lease_.index()==lease.index()&&child->parent20.lease_.generation()==lease.generation()){if(error)*error="scene class3 pool: referenced collision-parent cleanup required";return false;}
        for(unsigned i=0;i<fields.descriptor_count99;++i)if(!pool.can_release(storage_->class3_leases[lease.index_][i])){if(error)*error="scene class3 pool: current attached descriptor cleanup required";return false;}
        if(r.group01!=-1){
            unlink_group(unsigned(r.group01),r.free_next4,r.group_next8);
            for(unsigned i=0;i<fields.descriptor_count99;++i)if(!pool.release(storage_->class3_leases[lease.index_][i],error)){storage_->live=false;return false;}
        }
        if(bank.free_tail>=0)bank.records[unsigned(bank.free_tail)].free_next4={3,lease.index_};else bank.free_head=lease.index_;
        bank.free_tail=lease.index_;r.free_next4={};r.active00=0;r.allocated=false;++bank.free_count;if(error)error->clear();return true;
    }
    const ScenePhysicalBankSnapshot* collision_snapshot()const{return ready()&&storage_->collision.runtime_class==4?&storage_->collision:nullptr;}
    u64 collision_allocation_revision()const{return collision_snapshot()?storage_->collision_allocation_revision:0;}
    const ScenePhysicalRecord* inspect(const SceneCollisionLease& lease)const{return owns(lease)?&storage_->collision.records[lease.index_]:nullptr;}
    const ColliderSphereSeed* trial_sasuke_body(const SceneCollisionLease& lease)const{
        if(!owns(lease))return nullptr;
        const auto& seed=storage_->trial_sasuke_bodies[lease.index_];
        return seed?&*seed:nullptr;
    }
    const SceneTrialBeamShapeFields* trial_beam_shape(const SceneCollisionLease& lease)const{
        if(!owns(lease))return nullptr;
        const auto& shape=storage_->trial_beam_shapes[lease.index_];
        if(!shape||shape->carrier20.physical!=3||!shape->carrier20.present()||
           unsigned(shape->carrier20.index)>=storage_->class3.records.size())return nullptr;
        const auto& carrier=storage_->class3.records[unsigned(shape->carrier20.index)];
        return carrier.allocated&&carrier.generation==shape->carrier_generation?&*shape:nullptr;
    }
    const SceneCollisionFields* collision_child(const SceneCollisionLease& lease)const{if(!owns(lease))return nullptr;const auto& p=storage_->collision_fields[lease.index_];return p&&p->parent20.live()&&p->parent24.live()?&*p:nullptr;}
    const SceneStageCollisionFields* stage_collision_child(const SceneCollisionLease& lease)const{if(!owns(lease))return nullptr;const auto& p=storage_->stage_collision_fields[lease.index_];return p&&p->descriptor().data?p.get():nullptr;}
    const SceneViewCollisionFields* view_collision_child(const SceneCollisionLease& lease)const{if(!owns(lease))return nullptr;const auto& p=storage_->view_collision_fields[lease.index_];return p&&p->descriptor().data?p.get():nullptr;}
    //The shared allocator's class4 route. A rootless allocation is not a collider
    //constructor or update receipt. Class4 skips model descriptors and renderer
    //unregistration; mixed class2/class3/class4 groups retain ONE head/tail store.
    SceneObjectAcquire acquire_collision(s16 group,bool append,const std::optional<SceneObjectParentFields>& parent,SceneCollisionLease* out,std::string* error){
        if(out)*out={};
        auto fail=[&](const char* why){if(error)*error=std::string("scene collision pool: ")+why;return SceneObjectAcquire::Refused;};
        if(!out||!collision_snapshot()||group< -1||group>=64)return fail("current class4 domain/group required");
        s16 selected=group;if(group>=0&&group<56&&!(group&7)&&parent&&parent->kind83==0){if(parent->slot3e4<0||parent->slot3e4>=6)return fail("ordinary physical parent slot");selected=s16(group+1+parent->slot3e4);}
        auto& bank=storage_->collision;if(!bank.free_count){if(error)error->clear();return SceneObjectAcquire::NoObjects;}
        const auto index=u16(bank.free_head);auto& old=bank.records[index];
        if(old.generation==std::numeric_limits<u64>::max()||storage_->collision_allocation_revision==std::numeric_limits<u64>::max())
            return fail("native generation overflow");
        const auto generation=old.generation+1;bank.free_head=old.free_next4.index;if(bank.free_head<0)bank.free_tail=-1;--bank.free_count;
        ScenePhysicalRecord r;r.class02=r.physical03=4;r.group01=s8(selected);r.registration85=255;r.allocated=true;r.generation=generation;
        if(selected!=-1){r.kind83=storage_->profile.kind(unsigned(selected));link_group(unsigned(selected),append,{4,index},r.free_next4,r.group_next8);}
        old=r;storage_->collision_fields[index].reset();storage_->stage_collision_fields[index].reset();storage_->view_collision_fields[index].reset();storage_->collision_initial[index].reset();storage_->trial_beam_shapes[index].reset();storage_->trial_sasuke_bodies[index].reset();++storage_->collision_allocation_revision;out->owner_=storage_;out->index_=index;out->generation_=generation;if(error)error->clear();return SceneObjectAcquire::Allocated;
    }
    bool release(const SceneCollisionLease& lease,std::string* error){
        if(!owns(lease)){if(error)*error="scene collision pool: foreign/stale lease";return false;}
        //Bound collision bodies need source registry/body cleanup before release.
        if(storage_->collision_fields[lease.index_]||storage_->stage_collision_fields[lease.index_]||storage_->view_collision_fields[lease.index_]||storage_->trial_beam_shapes[lease.index_]){if(error)*error="scene collision pool: collision cleanup consumer required";return false;}
        if(storage_->trial_sasuke_bodies[lease.index_]){
            if(error)*error="scene collision pool: trial receiver cleanup consumer required";
            return false;
        }
        if(storage_->collision_allocation_revision==std::numeric_limits<u64>::max()){if(error)*error="scene collision pool: allocation revision exhausted";return false;}
        auto& bank=storage_->collision;auto& r=bank.records[lease.index_];
        if(r.group01!=-1)unlink_group(unsigned(r.group01),r.free_next4,r.group_next8);
        if(bank.free_tail>=0)bank.records[unsigned(bank.free_tail)].free_next4={4,lease.index_};else bank.free_head=lease.index_;
        bank.free_tail=lease.index_;r.free_next4={};r.active00=0;r.allocated=false;++bank.free_count;++storage_->collision_allocation_revision;if(error)error->clear();return true;
    }
    bool ready()const{return storage_&&storage_->current();}bool requires_scene_allocation_join()const{return true;}
    void retire(){if(storage_)storage_->live=false;storage_.reset();attempted_=true;} //native invalidation, not source bulk cleanup/descriptors-free ACK
private:
    friend class BattlePreparationWidgets;
    friend class BattlePreparationModels;
    friend class SceneModelBindings;
    friend class BattleActorConstruction;
    friend class ScenePhysicalPools;
    friend class BattleStageConstruction;
    friend class BattleViews;
    friend class BattlePreparationBootstrap;
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
    bool publish_trial_beam_sphere(const SceneClass3Lease& carrier,const SceneCollisionLease& child,
                                   const ColliderSphereSeed& sphere){
        if(!owns(carrier))return false;
        const auto* shape=trial_beam_shape(child);
        if(!shape||shape->sphere_seed||shape->carrier20!=ScenePhysicalLink{3,carrier.index_}||
           shape->carrier_generation!=carrier.generation_||!storage_->class3_models[carrier.index_].trial_frame114||
           !sphere.sweep||sphere.previous_radius8c)return false;
        storage_->trial_beam_shapes[child.index_]->sphere_seed=sphere;return true;
    }
    bool publish_trial_beam_activation(const SceneClass3Lease& carrier,const SceneCollisionLease& child,
                                       u16 previous_token,u16 token,u8 phase){
        if(!owns(carrier)||previous_token!=storage_->trial_scene_token||phase!=1)return false;
        const auto* shape=trial_beam_shape(child);
        auto& models=storage_->class3_models[carrier.index_];
        const u16 expected=u16(previous_token+1u);
        if(!shape||!shape->sphere_seed||shape->trial_phase10||shape->carrier20!=ScenePhysicalLink{3,carrier.index_}||
           shape->carrier_generation!=carrier.generation_||models.trial_once1ca||models.trial_token26c||
           token!=(expected?expected:1))return false;
        storage_->trial_scene_token=token;models.trial_token26c=token;models.trial_once1ca=1;models.trial_byte2ad=0;
        storage_->trial_beam_shapes[child.index_]->trial_phase10=phase;return true;
    }
    bool install_trial_sasuke_body(const SceneCollisionLease& child,const ColliderSphereSeed& seed){
        if(!owns(child))return false;
        auto& body=storage_->collision.records[child.index_];
        if(body.group01!=50||body.kind83!=26||body.active00||body.update0c_key||body.registration85!=255||
           !seed.previous_radius8c||seed.sweep)return false;
        storage_->trial_sasuke_bodies[child.index_]=seed;
        body.active00=1;body.update0c_key=0x8008a65c;return true;
    }
    //Called by the fresh physical bootstrap, before any object allocations.
    bool initialize_class3_pool(u16 count){
        if(!ready()||storage_->class3.runtime_class!=-1||count!=512)return false;
        auto& b=storage_->class3;b.records.resize(count);storage_->class3_models.resize(count);storage_->class3_leases.resize(count);b.runtime_class=3;b.free_head=0;b.free_tail=count-1;b.free_count=count;
        for(unsigned i=0;i<count;++i){auto& r=b.records[i];r.class02=r.physical03=3;r.free_next4=i+1<count?ScenePhysicalLink{3,s32(i+1)}:ScenePhysicalLink{};}return true;
    }
    bool initialize_collision_pool(u16 count){
        if(!ready()||storage_->collision.runtime_class!=-1||count!=384)return false;
        storage_->trial_beam_shapes.resize(count);
        storage_->trial_sasuke_bodies.resize(count);
        auto& b=storage_->collision;b.records.resize(count);storage_->collision_fields.resize(count);storage_->stage_collision_fields.resize(count);storage_->view_collision_fields.resize(count);storage_->collision_initial.resize(count);b.runtime_class=4;b.free_head=0;b.free_tail=count-1;b.free_count=count;
        for(unsigned i=0;i<count;++i){auto& r=b.records[i];r.class02=r.physical03=4;r.free_next4=i+1<count?ScenePhysicalLink{4,s32(i+1)}:ScenePhysicalLink{};}return true;
    }
    ScenePhysicalLink& previous(const ScenePhysicalLink& link){return link.physical==2?storage_->state.records[unsigned(link.index)].link4:(link.physical==3?storage_->class3:storage_->collision).records[unsigned(link.index)].free_next4;}
    ScenePhysicalLink& next(const ScenePhysicalLink& link){return link.physical==2?storage_->state.records[unsigned(link.index)].link8:(link.physical==3?storage_->class3:storage_->collision).records[unsigned(link.index)].group_next8;}
    void link_group(unsigned group,bool append,const ScenePhysicalLink& self,ScenePhysicalLink& prev,ScenePhysicalLink& following){
        auto& s=storage_->state;
        if(!append){prev={};following=s.group_heads[group];if(following.present())previous(following)=self;else s.group_tails[group]=self;s.group_heads[group]=self;}
        else{following={};prev=s.group_tails[group];if(prev.present())next(prev)=self;else s.group_heads[group]=self;s.group_tails[group]=self;}
    }
    void unlink_group(unsigned group,const ScenePhysicalLink& prev,const ScenePhysicalLink& following){
        auto& s=storage_->state;if(prev.present())next(prev)=following;else s.group_heads[group]=following;if(following.present())previous(following)=prev;else s.group_tails[group]=prev;
    }
    bool install_collision_child(const SceneCollisionLease& lease,const SceneCollisionFields& fields){
        if(!owns(lease)||storage_->collision_fields[lease.index_]||storage_->stage_collision_fields[lease.index_]||storage_->view_collision_fields[lease.index_]||!fields.parent20.live()||!(fields.parent20==fields.parent24)||!(fields.parent20==fields.descriptor_owner28)||!(fields.parent20==fields.parameter_owner2c)||!(fields.parent20==fields.matrix_owner60)||fields.self5c!=ScenePhysicalLink{4,lease.index_})return false;
        auto& r=storage_->collision.records[lease.index_];if(r.active00||r.update0c_key)return false;
        r.active00=1;r.update0c_key=0x8008a65c;storage_->collision_fields[lease.index_]=fields;return true;
    }
    bool install_stage_collision(const SceneCollisionLease& lease,const SceneStageCollisionFields& fields){
        if(!owns(lease)||storage_->collision_fields[lease.index_]||storage_->stage_collision_fields[lease.index_]||storage_->view_collision_fields[lease.index_]||!fields.parent20.live()||!owns(fields.parent20.lease_)||!(fields.parent20==fields.parent24)||!(fields.parent20==fields.matrix_owner60)||fields.self5c!=ScenePhysicalLink{4,lease.index_}||fields.parameter_key2c!=0x802c4760||!fields.descriptor().data)return false;
        const auto bank=fields.source.block(0),record=fields.descriptor();if(!bank.contains(4*unsigned(fields.selector14),4)||fields.descriptor_offset28!=0x400+unsigned(fields.record12)*0x50||fields.parameter11!=u8(record.be16(4))||std::bit_cast<u16>(fields.start48)!=record.be16(6)||std::bit_cast<u16>(fields.end4a)!=record.be16(8))return false;
        bool selected=false;for(unsigned i=0;i<4&&bank.data[4*unsigned(fields.selector14)+i]!=255;++i)selected|=bank.data[4*unsigned(fields.selector14)+i]==fields.record12;if(!selected)return false;
        auto& r=storage_->collision.records[lease.index_];if(r.active00||r.update0c_key||r.group01!=48)return false;
        auto next=std::make_shared<const SceneStageCollisionFields>(fields);r.active00=1;r.update0c_key=0x8008a65c;storage_->stage_collision_fields[lease.index_]=std::move(next);return true;
    }
    bool install_view_collision(const SceneCollisionLease& lease,const SceneViewCollisionFields& fields){
        if(!owns(lease)||storage_->collision_fields[lease.index_]||storage_->stage_collision_fields[lease.index_]||storage_->view_collision_fields[lease.index_]||!fields.parent20.live()||!(fields.parent20==fields.parent24)||!(fields.parent20==fields.matrix_owner60)||fields.self5c!=ScenePhysicalLink{4,lease.index_}||fields.parameter_key2c!=0x802c4760||fields.selector14!=61)return false;
        const auto scope=fields.parent20.scope_.lock();if(!scope||scope->objects.lock()!=storage_)return false;
        const auto b=fields.source.block(0),row=fields.descriptor();
        if(!b.contains(244,4)||!row.data||fields.descriptor_offset28!=0x400+unsigned(fields.record12)*0x50||fields.parameter11!=u8(row.be16(4))||std::bit_cast<u16>(fields.start48)!=row.be16(6)||std::bit_cast<u16>(fields.end4a)!=row.be16(8))return false;
        bool selected=false;for(unsigned i=0;i<4&&b.data[244+i]!=255;++i)selected|=b.data[244+i]==fields.record12;if(!selected)return false;
        auto& r=storage_->collision.records[lease.index_];if(r.active00||r.update0c_key||r.group01!=48)return false;
        auto next=std::make_shared<const SceneViewCollisionFields>(fields);r.active00=1;r.update0c_key=0x8008a65c;storage_->view_collision_fields[lease.index_]=std::move(next);return true;
    }
    //88E50's class4 branch. Only the view owner may retire a proven
    //never-initialized/unregistered body. Public bound-body cleanup stays closed.
    bool retire_initial_view_collision(const SceneCollisionLease& lease,const SceneViewReference& parent,std::string* error){
        if(!owns(lease)||storage_->collision_initial[lease.index_]||storage_->collision_fields[lease.index_]||storage_->stage_collision_fields[lease.index_])return false;
        const auto& fields=storage_->view_collision_fields[lease.index_];const auto& record=storage_->collision.records[lease.index_];
        if(!fields||!parent.live()||fields->parent20!=parent||fields->parent24!=parent||record.group01!=48||record.class02!=4||record.physical03!=4||record.registration85!=255||record.update0c_key!=0x8008a65c)return false;
        storage_->view_collision_fields[lease.index_].reset();
        return release(lease,error);
    }
    bool install_actor_child(const SceneObjectLease& lease,const SceneObjectHeader& header,const SceneObjectActorChildFields& fields,std::optional<u8> source_kind83=std::nullopt){
        if(!owns(lease)||!fields.parent8c.live()||!fields.parent90.live()||!(fields.parent8c==fields.parent90)||storage_->state.records[lease.index_].actor_child)return false;
        for(const auto* ref:{&fields.field144,&fields.field148})if(*ref&&(!(*ref)->live()||!((*ref)->owner==fields.parent8c)))return false;
        if(!install_header(lease,header))return false;
        if(source_kind83)storage_->state.records[lease.index_].kind83=*source_kind83;
        storage_->state.records[lease.index_].actor_child=fields;return true;
    }
    bool owns(const SceneObjectLease& lease)const{return ready()&&lease.owner_.lock()==storage_&&lease.current();}
    bool owns(const SceneCollisionLease& lease)const{return ready()&&lease.owner_.lock()==storage_&&lease.current();}
    bool owns(const SceneClass3Lease& lease)const{return ready()&&lease.owner_.lock()==storage_&&lease.current();}
    std::shared_ptr<scene_object_pool_detail::Storage> storage_;bool attempted_=false;
};
} // namespace gf
