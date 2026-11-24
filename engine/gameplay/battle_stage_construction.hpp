#pragma once
#include "engine/assets/battle_stage_side.hpp"
#include "engine/assets/battle_stage_props.hpp"
#include "engine/gameplay/battle_stage_resources.hpp"
#include "engine/gameplay/battle_side_resources.hpp"
#include "engine/gameplay/scene_physical_pools.hpp"
#include "engine/gameplay/scene_model_bindings.hpp"
#include "engine/gameplay/attachment_transform.hpp"
#include "engine/gameplay/attachment_model_scene.hpp"
#include "engine/gameplay/model_pass_queue.hpp"
#include "engine/assets/battle_stage_object_update.hpp"
#include "engine/gameplay/collision_contact_reset.hpp"
#include "engine/assets/battle_prop_publication.hpp"
#include "engine/assets/battle_side_publication.hpp"
namespace gf {
enum class BattleStageConstructionPhase {Closed,SideObjectsRunning,AwaitProps,PropsRunning,RootsRunning,InitializedAwaitFrame,Failed,Retired};
struct BattleStageCollisionAttempt {u8 selector=0,record=0;SceneObjectAcquire result=SceneObjectAcquire::Refused;SceneCollisionLease lease;};
struct BattleStageSideObject {
    u8 ordinal=0,type11=0,variant12=0,record13=0,bank18b=0,flags84=0,team88=0,mask18a=0;
    SceneObjectAcquire result=SceneObjectAcquire::Refused;SceneObjectLease class2;SceneClass3Lease class3;
    std::array<f32,3> local64{},position20{},scale58{};std::array<s16,3> angles70{};
    std::array<f32,13> factors144_174{};std::optional<Mat4> world114;
    SceneClass3Reference self8c;u32 program178=0,parameter27c=0;f32 radius17c=0,bound204=0;
    std::array<f32,2> progress180_184{};u16 duration188=0;std::array<u16,3> hp1c4_1c8{};
    f32 scale_b4=0,scale_b8=0,scale_c0=0,scale_c4=0;u8 selector1b=0,enabled26f=0,enabled281=0,collision86=0,field1d8=0;
    bool constructor_returned=false;
    u8 phase18=0,render82=0,contact1ca=0;u64 update_generation=0,world_generation=0;
    u16 suppressed272=0;u8 disabled1eb=0,disabled276=0;
    CollisionContactState contact;u64 contact_generation=0;
    std::array<f32,3> previous2c{};std::array<s16,3> previous76{};
};
struct BattleStagePropObject {
    u8 record13=0,type11=0,flags84=0,team88=0,collision86=0,field1d8=0,enabled26f=0,enabled281=0;
    bool static_prop=false,constructor_returned=false;SceneObjectAcquire result=SceneObjectAcquire::Refused;
    SceneObjectLease class2;SceneClass3Lease class3;SceneClass3Reference self8c;
    std::array<f32,3> position20{},scale58{};std::array<s16,3> angles70{};std::optional<Mat4> world114;
    f32 scale_b4=0,scale_b8=0,scale_c0=0,scale_c4=0,bound204=0;u32 parameter27c=0;
    u8 render82=0,contact1ca=0;u64 update_generation=0;
    u16 suppressed272=0;u8 disabled1eb=0,disabled276=0;
    CollisionContactState contact;u64 contact_generation=0;
    std::array<f32,3> previous2c{};std::array<s16,3> previous76{};
};
struct BattleStagePublishedModel {u8 slot=0;u16 descriptor=0,record=0;u64 world_generation=0;};
struct BattlePropPublishedModel {ScenePhysicalLink object;u16 descriptor=0,record=0;u8 mask=0;u64 world_generation=0;};
struct BattleSidePublishedModel {ScenePhysicalLink object;u8 ordinal=0,bank=0,variant=0,mask=0;u16 descriptor=0;u64 world_generation=0;};
//Stage2's87D2C construction through818D0/87924 and four stage roots.
//Constructor completion does not grant update callbacks, camera or render frames.
class BattleStageConstruction {
public:
    BattleStageConstruction()=default;BattleStageConstruction(const BattleStageConstruction&)=delete;BattleStageConstruction& operator=(const BattleStageConstruction&)=delete;
    BattleStageConstruction(BattleStageConstruction&&)=default;BattleStageConstruction& operator=(BattleStageConstruction&&)=default;
    ~BattleStageConstruction(){retire();}
    BattleStageConstructionPhase phase()const{return phase_;}
    bool current()const{
        if(!storage_||phase_==BattleStageConstructionPhase::Failed||phase_==BattleStageConstructionPhase::Retired||!storage_->objects->ready()||!storage_->common.live())return false;
        for(const auto& e:storage_->entries)if(e.state.result==SceneObjectAcquire::Allocated){
            if(!(e.state.variant12?e.state.class2.current():e.state.class3.current())||!models_current(e,e.state.variant12?1:2))return false;
        }
        for(const auto& e:storage_->props)if(e.state.result==SceneObjectAcquire::Allocated)if(!(e.state.static_prop?e.state.class2.current():e.state.class3.current())||!models_current(e,e.state.static_prop?1:2))return false;
        for(const auto& e:storage_->stage_roots)if(e.lease.current()){const auto* root=storage_->models->root(e.lease);if(!storage_->main_archive.bank()||!root||!root->receipt().same_instance(e.root))return false;}else if(phase_==BattleStageConstructionPhase::InitializedAwaitFrame)return false;
        return true;
    }
    const BattleStageSideObject* side_object(unsigned i)const{return current()&&i<storage_->entries.size()?&storage_->entries[i].state:nullptr;}
    unsigned prop_attempts()const{return current()?unsigned(storage_->props.size()):0;}
    const BattleStagePropObject* prop_object(unsigned i)const{return current()&&i<storage_->props.size()?&storage_->props[i].state:nullptr;}
    const HsdJointInstanceOwner* stage_model(unsigned i)const{return current()&&i<4?storage_->models->root(storage_->stage_roots[i].lease):nullptr;}
    const SceneModelMaterialContext* stage_material_context(unsigned i)const{return current()&&i<4?storage_->models->material_context(storage_->stage_roots[i].lease):nullptr;}
    std::span<const BattleStageCollisionAttempt> collision_attempts()const{return current()?std::span<const BattleStageCollisionAttempt>{storage_->collisions}:std::span<const BattleStageCollisionAttempt>{};}
    const std::optional<std::array<u32,16>>* side_globals()const{return current()?&storage_->side_globals:nullptr;}
    u64 object_world_generation()const{return current()?storage_->object_world_generation:0;}
    bool first_object_updates_complete()const{return current()&&storage_->object_updates_complete;}
    std::span<const ScenePhysicalLink> object_update_order()const{return current()?std::span<const ScenePhysicalLink>{storage_->update_order}:std::span<const ScenePhysicalLink>{};}
    std::span<const ScenePhysicalLink> object_world_order()const{return current()?std::span<const ScenePhysicalLink>{storage_->world_order}:std::span<const ScenePhysicalLink>{};}
    void retire(){invalidate_scopes();storage_.reset();attempted_=true;phase_=BattleStageConstructionPhase::Retired;}
private:
    friend class BattlePreparationBootstrap;
    struct ModelOwner {
        SceneArchiveLease archive;std::array<SceneDescriptorLease,2> descriptors;std::array<HsdJointInstanceReceipt,2> roots;
        std::shared_ptr<SceneStageObjectLifetime> scope;std::shared_ptr<const SceneModelMaterialContext> context;
    };
    struct Entry:ModelOwner {BattleStageSideObject state;};
    struct PropEntry:ModelOwner {BattleStagePropObject state;};
    struct StageRoot {SceneDescriptorLease lease;HsdJointInstanceReceipt root;std::shared_ptr<const SceneModelMaterialContext> context;};
    struct Storage {
        std::shared_ptr<SceneObjectPool> objects;std::shared_ptr<SceneDescriptorPool> models;BattleCommonResourceBinding common;
        std::vector<Entry> entries;std::vector<BattleStageCollisionAttempt> collisions;std::optional<std::array<u32,16>> side_globals;
        std::vector<PropEntry> props;SceneArchiveLease main_archive;std::array<StageRoot,4> stage_roots;
        std::weak_ptr<const HsdJointWorldScope> publication_scope;
        std::vector<BattleStagePublishedModel> published;
        std::array<AttachmentModelScene,2> geometry;bool geometry_prepared=false;
        u64 object_world_generation=0;bool object_updates_complete=false;
        std::vector<ScenePhysicalLink> update_order,world_order;
        std::vector<BattlePropPublishedModel> published_props;ScenePhysicalLink next_object_draw;
        std::vector<AttachmentModelScene> prop_geometry;bool prop_geometry_prepared=false;
        std::vector<BattleSidePublishedModel> published_sides;std::vector<AttachmentModelScene> side_geometry;bool side_geometry_prepared=false;
        std::optional<BattleSidePublishedModel> presented_side;AttachmentModelScene presented_side_geometry;bool presented_side_geometry_prepared=false;
    };
    struct ObjectRef {ScenePhysicalLink link;BattleStageSideObject* side=nullptr;BattleStagePropObject* prop=nullptr;};
    std::optional<ObjectRef> object_ref(ScenePhysicalLink link){
        if(!link.present())return {};
        for(auto& e:storage_->entries){auto& s=e.state;if(s.result==SceneObjectAcquire::Allocated&&link==(s.variant12?ScenePhysicalLink{2,s.class2.index()}:ScenePhysicalLink{3,s.class3.index()}))return ObjectRef{link,&s,nullptr};}
        for(auto& e:storage_->props){auto& s=e.state;if(s.result==SceneObjectAcquire::Allocated&&link==(s.static_prop?ScenePhysicalLink{2,s.class2.index()}:ScenePhysicalLink{3,s.class3.index()}))return ObjectRef{link,nullptr,&s};}
        return {};
    }
    bool group40(std::vector<ObjectRef>* out,std::string* error){
        out->clear();auto reject=[&](const char* why){if(error)*error=std::string("stage group40: ")+why;return false;};
        auto& pool=*storage_->objects->storage_;const auto& s=pool.state;
        //8997C's per-actor children and group47 are separate consumers. None
        //may be silently ignored or replaced by the known stage-object subset.
        for(unsigned i=41;i<=47;++i)if(s.group_heads[i].present()||s.group_tails[i].present())return reject("nonempty adjacent source group requires its consumers");
        ScenePhysicalLink previous;auto link=s.group_heads[40];
        while(link.present()){
            if(out->size()>=56)return reject("group cycle/budget");
            const auto ref=object_ref(link);if(!ref)return reject("unknown occupied object in actual group");
            for(const auto& prior:*out)if(prior.link==link)return reject("group cycle");
            ScenePhysicalLink before,next;u32 callback=0;bool active=false;
            if(link.physical==2){const auto& r=s.records[unsigned(link.index)];before=r.link4;next=r.link8;active=r.allocated&&r.active00==1&&r.group01==40&&r.header_installed;callback=r.header.update0c_key;}
            else{const auto& r=pool.class3.records[unsigned(link.index)];before=r.free_next4;next=r.group_next8;active=r.allocated&&r.active00==1&&r.group01==40;callback=r.update0c_key;}
            if(!active||before!=previous||callback!=(ref->side?0x80081d98u:0x80087b58u))return reject("current group header/callback correspondence required");
            out->push_back(*ref);previous=link;link=next;
        }
        unsigned expected=0;for(const auto& e:storage_->entries)expected+=e.state.result==SceneObjectAcquire::Allocated;for(const auto& e:storage_->props)expected+=e.state.result==SceneObjectAcquire::Allocated;
        if(previous!=s.group_tails[40]||out->size()!=expected)return reject("complete source group membership required");
        return true;
    }
    bool prepare_side_world_callback(const BattleStageObjectUpdateProfile& profile,ScenePhysicalPools& physical,ScenePhysicalLink link,AttachmentBoneTransform* out,std::string* error){
        auto reject=[&](){if(error)*error="stage side callback: same fresh owned82060 source object required";return false;};
        if(!out||!profile.ready()||phase_!=BattleStageConstructionPhase::InitializedAwaitFrame||!current()||!physical.current()||physical.class2_pool()!=storage_->objects||storage_->object_world_generation>=2||link.physical!=3||link.index<0||unsigned(link.index)>=storage_->objects->storage_->class3.records.size())return reject();
        const auto ref=object_ref(link);if(!ref||!ref->side||ref->side->variant12)return reject();
        const auto& s=*ref->side;const auto& r=storage_->objects->storage_->class3.records[unsigned(link.index)];
        if(!s.self8c.live()||!s.constructor_returned||s.flags84!=0x0f||s.world_generation!=storage_->object_world_generation||s.progress180_184!=std::array<f32,2>{}||r.callback100_key!=0x80082060||r.registration85!=1)return reject();
        AttachmentBoneTransformInput input;input.flags=s.flags84;input.local_position=s.local64;input.angles=s.angles70;AttachmentBoneTransform transform;
        if(!attachment_identity_transform(input,&transform,error))return false;
        const f32 offset[3]{0,s.progress180_184[0],0};mat4_transform_point(transform.world,offset,transform.position.data());
        for(unsigned k=0;k<3;++k)transform.world.at(k,3)=transform.position[k];
        *out=transform;if(error){error->clear();}return true;
    }
    bool commit_side_world_callback(ScenePhysicalLink link,const AttachmentBoneTransform& transform){
        const auto ref=object_ref(link);if(!ref||!ref->side||!transform.valid)return false;
        ref->side->position20=transform.position;ref->side->world114=transform.world;++ref->side->world_generation;return true;
    }
    //No callback replay. The scene's completed first89554 traversal owns this
    //aggregate handoff; partial side matrices must never call it.
    bool release_initial_callback_worlds(){
        if(phase_!=BattleStageConstructionPhase::InitializedAwaitFrame||!current()||storage_->object_world_generation||storage_->object_updates_complete||!storage_->published.empty())return false;
        unsigned expected=0;for(const auto& e:storage_->entries)if(e.state.result==SceneObjectAcquire::Allocated&&!e.state.variant12){
            const auto& s=e.state;const ScenePhysicalLink link{3,s32(s.class3.index())};
            if(!s.class3.current()||s.world_generation!=1||!s.world114)return false;
            unsigned occurrences=0;for(auto visited:storage_->world_order)occurrences+=visited==link;
            if(occurrences!=1)return false;
            ++expected;
        }
        if(storage_->world_order.size()!=expected)return false;
        storage_->object_world_generation=1;return true;
    }
    bool advance_object_worlds(const BattleStageObjectUpdateProfile& profile,ScenePhysicalPools& physical,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("stage side worlds: ")+why;return false;};
        if(!profile.ready()||phase_!=BattleStageConstructionPhase::InitializedAwaitFrame||!current()||!physical.current()||physical.class2_pool()!=storage_->objects||!storage_->published.empty()||storage_->object_world_generation>=2||bool(storage_->object_world_generation)!=storage_->object_updates_complete)return reject("initial source side-callback boundary required");
        const auto& scheduler=*physical.scheduler();auto& pool=*storage_->objects->storage_;
        for(unsigned i=0;i<12;++i)if(i!=1&&(scheduler.callback_heads[i].present()||scheduler.callback_tails[i].present()))return reject("unjoined occupied89554 callback list");
        struct Pending {BattleStageSideObject* state;AttachmentBoneTransform transform;};
        std::vector<Pending> pending;std::vector<ScenePhysicalLink> order;ScenePhysicalLink previous;auto link=scheduler.callback_heads[1];
        while(link.present()){
            if(order.size()>=8||link.physical!=3)return reject("source callback class/cycle/budget");
            for(auto old:order)if(old==link)return reject("source callback cycle");
            const auto ref=object_ref(link);if(!ref||!ref->side||ref->side->variant12)return reject("registered side owner required");
            auto& s=*ref->side;const auto& r=pool.class3.records[unsigned(link.index)];
            if(r.callback_previous104!=previous)return reject("fresh82060 callback correspondence");
            AttachmentBoneTransform transform;if(!prepare_side_world_callback(profile,physical,link,&transform,error))return false;
            pending.push_back({&s,transform});order.push_back(link);previous=link;link=r.callback_next108;
        }
        unsigned expected=0;for(const auto& e:storage_->entries)expected+=e.state.result==SceneObjectAcquire::Allocated&&!e.state.variant12;
        if(previous!=scheduler.callback_tails[1]||pending.size()!=expected)return reject("complete actual side callback list required");
        for(auto& p:pending){p.state->position20=p.transform.position;p.state->world114=p.transform.world;++p.state->world_generation;}
        storage_->world_order=std::move(order);++storage_->object_world_generation;if(error)error->clear();return true;
    }
    bool advance_first_object_updates(const BattleStageObjectUpdateProfile& profile,ScenePhysicalPools& physical,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("stage object first update: ")+why;return false;};
        if(!profile.ready()||phase_!=BattleStageConstructionPhase::InitializedAwaitFrame||!current()||!physical.current()||physical.class2_pool()!=storage_->objects||storage_->object_world_generation!=1||storage_->object_updates_complete||(physical.controls()->d1&0x80))return reject("unconsumed unmasked first group40 update required");
        std::vector<ObjectRef> order;if(!group40(&order,error))return false;
        for(const auto& r:order){
            if(r.side){const auto& s=*r.side;if(s.phase18||s.render82||s.update_generation||!s.world114)return reject("fresh side phase/world required");}
            else{const auto& s=*r.prop;if(s.render82||s.update_generation||!s.world114)return reject("fresh prop world required");}
        }
        std::vector<ScenePhysicalLink> links;links.reserve(order.size());for(const auto& r:order)links.push_back(r.link);
        for(auto& r:order){
            if(r.side){auto& s=*r.side;s.previous2c=s.position20;s.previous76=s.angles70;s.phase18=s.render82=1;if(s.collision86)s.contact1ca=0;++s.update_generation;}
            else{auto& s=*r.prop;s.previous2c=s.position20;s.previous76=s.angles70;s.render82=1;if(s.collision86)s.contact1ca=0;++s.update_generation;}
        }
        storage_->update_order=std::move(links);storage_->object_updates_complete=true;if(error)error->clear();return true;
    }
    bool collect_first_objects(ScenePhysicalPools& physical,std::string* error){
        if(!current()||!physical.current()||physical.class2_pool()!=storage_->objects||storage_->object_world_generation!=2||!storage_->object_updates_complete){if(error)*error="stage collection: current source object worlds/updates required";return false;}
        std::vector<ObjectRef> order;if(!group40(&order,error))return false;
        for(const auto& r:order)if((r.side?r.side->render82:r.prop->render82)!=1){if(error)*error="stage collection: changed render selector";return false;}
        auto& scheduler=physical.storage_->scheduler;const auto index=ScenePhysicalSchedulerSnapshot::object_render_base;
        if(scheduler.render_heads[index].present()||scheduler.render_tails[index].present()){if(error)*error="stage collection: consumed source render-list prefix";return false;}
        auto& pool=*storage_->objects->storage_;auto next=[&](ScenePhysicalLink link)->ScenePhysicalLink&{return link.physical==2?pool.state.records[unsigned(link.index)].render_next110:pool.class3.records[unsigned(link.index)].render_next110;};
        for(const auto& r:order){next(r.link)={};if(scheduler.render_tails[index].present())next(scheduler.render_tails[index])=r.link;else scheduler.render_heads[index]=r.link;scheduler.render_tails[index]=r.link;}
        if(error){error->clear();}return true;
    }
    bool object_collection_current(const ScenePhysicalPools& physical)const{
        if(!current()||!physical.current()||physical.class2_pool()!=storage_->objects||storage_->object_world_generation!=2||!storage_->object_updates_complete)return false;
        const auto& pool=*storage_->objects->storage_;const auto& order=storage_->update_order;const auto& scheduler=*physical.scheduler();
        const auto first=order.empty()?ScenePhysicalLink{}:order.front(),last=order.empty()?ScenePhysicalLink{}:order.back();
        const auto index=ScenePhysicalSchedulerSnapshot::object_render_base;
        if(pool.state.group_heads[40]!=first||pool.state.group_tails[40]!=last||scheduler.render_heads[index]!=first||scheduler.render_tails[index]!=last)return false;
        for(unsigned i=41;i<=47;++i)if(pool.state.group_heads[i].present()||pool.state.group_tails[i].present())return false;
        for(unsigned i=0;i<order.size();++i){
            const auto link=order[i],previous=i?order[i-1]:ScenePhysicalLink{},next=i+1<order.size()?order[i+1]:ScenePhysicalLink{};
            if(link.physical==2){const auto& r=pool.state.records[unsigned(link.index)];if(!r.allocated||r.active00!=1||r.group01!=40||r.link4!=previous||r.link8!=next||r.render_next110!=next)return false;}
            else if(link.physical==3){const auto& r=pool.class3.records[unsigned(link.index)];if(!r.allocated||r.active00!=1||r.group01!=40||r.free_next4!=previous||r.group_next8!=next||r.render_next110!=next)return false;}
            else return false;
        }
        return true;
    }
    bool prop_publication_current()const{
        if(!publication_current()||storage_->published_props.size()!=48)return false;
        const auto scope=storage_->publication_scope.lock();if(!scope)return false;
        for(const auto& model:storage_->published_props){
            const auto found=std::find_if(storage_->props.begin(),storage_->props.end(),[&](const auto& p){return p.state.record13==model.record;});
            if(found==storage_->props.end()||found->descriptors[0].index()!=model.descriptor)return false;
            const auto* root=storage_->models->root(found->descriptors[0]);if(!root||!root->world_from(scope)||root->state()->world_generation!=model.world_generation)return false;
        }
        return true;
    }
    bool prepare_prop_published_geometry(std::string* error){
        if(!prop_publication_current()||storage_->prop_geometry_prepared){if(error)*error="prop geometry: fresh current published prefix required";return false;}
        try{
            AttachmentModelBank bank;if(!bank.load_prepared(*storage_->main_archive.bank(),error))return false;
            std::vector<AttachmentModelScene> geometry;geometry.reserve(storage_->published_props.size());
            for(const auto& model:storage_->published_props){
                const auto found=std::find_if(storage_->props.begin(),storage_->props.end(),[&](const auto& entry){return entry.state.record13==model.record;});
                if(found==storage_->props.end())return false;
                const auto* root=storage_->models->root(found->descriptors[0]);if(!root)return false;
                AttachmentModelScene renderer;if(!renderer.load_published_static(bank,*storage_->main_archive.bank(),*root,error,HsdPublishedMaterialPolicy::StaticIdentityColourSubtract)){if(error)*error="prop record"+std::to_string(model.record)+": "+*error;return false;}
                geometry.push_back(std::move(renderer));
            }
            if(!prop_publication_current())return false;
            storage_->prop_geometry=std::move(geometry);storage_->prop_geometry_prepared=true;if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain prop geometry preparation; no draw grant");}
    }
    bool side_publication_current()const{
        if(!prop_publication_current()||storage_->published_sides.size()!=7)return false;
        const auto scope=storage_->publication_scope.lock();if(!scope)return false;
        for(const auto& model:storage_->published_sides){
            if(model.ordinal>=storage_->entries.size())return false;
            const auto& entry=storage_->entries[model.ordinal];
            const auto* root=storage_->models->root(entry.descriptors[0]);
            if(!root||entry.descriptors[0].index()!=model.descriptor||!root->world_from(scope)||root->state()->world_generation!=model.world_generation||!root->state()->source_zero_colour_cache)return false;
        }
        return true;
    }
    bool publish_side_identity_prefix(const BattleSidePublicationProfile& profile,ScenePhysicalPools& physical,ModelPassQueue& queue,const std::shared_ptr<const HsdJointWorldScope>& scope,u64 generation,u8 active_view_mask,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("side draw prefix: ")+why;return false;};
        if(!profile.ready()||!prop_publication_current()||!object_collection_current(physical)||!scope||storage_->publication_scope.lock()!=scope||!queue.ready()||!generation||!active_view_mask||!storage_->published_sides.empty())return reject("fresh current prop-complete source callback prefix required");
        struct Pending {Entry* entry;BattleSidePublishedModel model;HsdJointInstanceStateSet world;};
        std::vector<Pending> pending;pending.reserve(7);auto link=storage_->next_object_draw;auto& arena=*storage_->objects->storage_;
        for(unsigned i=0;i<7;++i){
            const auto ref=object_ref(link);if(!ref||!ref->side||ref->side->ordinal!=7-i)return reject("actual source side render110 order required");
            auto& s=*ref->side;
            if(!s.constructor_returned||!s.world114||s.phase18!=1||s.render82!=1||s.update_generation!=1||(!s.variant12&&s.world_generation!=2)||s.contact_generation!=u64(s.collision86?2:0)||(s.contact.hidden270&active_view_mask))return reject("unoccluded current side/frame fields required");
            for(f32 factor:s.factors144_174)if(factor!=1)return reject("non-identity per-view fade requires presentation selection");
            const auto& header=s.variant12?arena.state.records[s.class2.index()].header:arena.class3_models[s.class3.index()].header;
            if(header.draw10c_key!=0x800820c8||s.mask18a!=(s.variant12?0x0f:0x47))return reject("source820C8 callback/mask correspondence");
            auto& entry=storage_->entries[s.ordinal];Pending p;p.entry=&entry;p.model={link,s.ordinal,s.bank18b,s.variant12,s.mask18a,entry.descriptors[0].index(),generation};
            if(!SceneModelBindings::prepare_side_identity_world(*storage_->models,entry.descriptors[0],entry.archive,entry.roots[0],entry.context.get(),s.variant12,generation,*s.world114,&p.world,error))return false;
            pending.push_back(std::move(p));link=s.variant12?arena.state.records[s.class2.index()].render_next110:arena.class3.records[s.class3.index()].render_next110;
        }
        const auto next=object_ref(link);if(!next||!next->side||next->side->ordinal!=0||!(next->side->contact.hidden270&active_view_mask))return reject("source viewer-dependent side0 must remain pending");
        try{
            storage_->published_sides.reserve(7);
            for(auto& p:pending){
                const auto& lease=p.entry->descriptors[0];const auto id=u32(lease.index())+1;
                if(!SceneModelBindings::commit_world(*storage_->models,lease,p.entry->roots[0],scope,std::move(p.world))||!SceneModelBindings::set_published_descriptor_flag(*storage_->models,lease,scope,1)||!queue.register_model(id,p.model.mask,storage_->models->inspect(lease)->fields.payload10_1f[0]!=0))return fail(error,"uncertain side world/model registration; prefix retained");
                std::array<u32,3> links;if(!queue.source_links(id,&links)||!SceneModelBindings::bind_queue_links(*storage_->models,lease,scope,links,generation))return fail(error,"side source model links refused");
                storage_->published_sides.push_back(p.model);
            }
            storage_->next_object_draw=link;if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain side publication; no replay");}
    }
    bool prepare_side_identity_geometry(std::string* error){
        if(!side_publication_current()||storage_->side_geometry_prepared){if(error)*error="side geometry: fresh current zero-colour/identity prefix required";return false;}
        try{
            std::array<AttachmentModelBank,2> banks;std::array<bool,2> loaded{};
            std::vector<AttachmentModelScene> geometry;geometry.reserve(7);
            for(const auto& model:storage_->published_sides){
                auto& entry=storage_->entries[model.ordinal];if(model.bank>=2||!entry.archive.bank())return false;
                if(!loaded[model.bank]){if(!banks[model.bank].load_prepared(*entry.archive.bank(),error))return false;loaded[model.bank]=true;}
                const auto* root=storage_->models->root(entry.descriptors[0]);if(!root)return false;
                AttachmentModelScene renderer;if(!renderer.load_published_side_identity(banks[model.bank],*entry.archive.bank(),*root,error,HsdPublishedMaterialPolicy::StaticIdentityColourSubtract)){if(error)*error="side ordinal"+std::to_string(model.ordinal)+": "+*error;return false;}
                geometry.push_back(std::move(renderer));
            }
            if(!side_publication_current())return false;
            storage_->side_geometry=std::move(geometry);storage_->side_geometry_prepared=true;if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain side geometry preparation");}
    }
    bool publish_presented_last_side(const BattleSidePublicationProfile& profile,ScenePhysicalPools& physical,ModelPassQueue& queue,const std::shared_ptr<const HsdJointWorldScope>& scope,u64 generation,u8 viewer,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("presented side0: ")+why;return false;};
        if(!profile.ready()||!side_publication_current()||!object_collection_current(physical)||storage_->publication_scope.lock()!=scope||storage_->presented_side||viewer>=6||!generation||!queue.ready())return reject("current seven-side prefix and real presented viewer required");
        const auto ref=object_ref(storage_->next_object_draw);if(!ref||!ref->side||ref->side->ordinal!=0||ref->side->variant12)return reject("actual next side0 root/callback required");
        auto& s=*ref->side;auto& entry=storage_->entries[0];
        if(!s.world114||s.world_generation!=2||s.contact_generation!=2||s.phase18!=1||s.render82!=1||s.mask18a!=0x47||s.factors144_174[viewer]!=1||(s.contact.hidden270&(1u<<viewer)))return reject("first presented unoccluded/identity branch only; other viewers need styled fade continuation");
        const auto& header=storage_->objects->storage_->class3_models[s.class3.index()].header;if(header.draw10c_key!=0x800820c8)return reject("actual820C8 callback");
        HsdJointInstanceStateSet world;
        if(!SceneModelBindings::prepare_side_identity_world(*storage_->models,entry.descriptors[0],entry.archive,entry.roots[0],entry.context.get(),0,generation,*s.world114,&world,error))return false;
        try{
            const auto& lease=entry.descriptors[0];const auto id=u32(lease.index())+1;
            if(!SceneModelBindings::commit_world(*storage_->models,lease,entry.roots[0],scope,std::move(world))||!SceneModelBindings::set_published_descriptor_flag(*storage_->models,lease,scope,1)||!queue.register_model(id,0x47,storage_->models->inspect(lease)->fields.payload10_1f[0]!=0))return fail(error,"uncertain presented side0 model/queue prefix");
            std::array<u32,3> links;if(!queue.source_links(id,&links)||!SceneModelBindings::bind_queue_links(*storage_->models,lease,scope,links,generation))return fail(error,"presented side0 model links refused");
            storage_->presented_side=BattleSidePublishedModel{storage_->next_object_draw,0,s.bank18b,0,0x47,lease.index(),generation};
            storage_->next_object_draw=storage_->objects->storage_->class3.records[s.class3.index()].render_next110;
            if(error){error->clear();}return true;
        }catch(const std::exception&){return fail(error,"uncertain presented side0 continuation; no replay");}
    }
    bool prepare_presented_side_geometry(std::string* error){
        if(!side_publication_current()||!storage_->presented_side||storage_->presented_side_geometry_prepared){if(error)*error="presented side geometry: fresh current source root required";return false;}
        auto& entry=storage_->entries[0];const auto* root=storage_->models->root(entry.descriptors[0]);if(!root)return false;
        AttachmentModelBank bank;if(!bank.load_prepared(*entry.archive.bank(),error))return false;
        AttachmentModelScene geometry;if(!geometry.load_published_side_identity(bank,*entry.archive.bank(),*root,error,HsdPublishedMaterialPolicy::StaticIdentityColourSubtract))return false;
        storage_->presented_side_geometry=std::move(geometry);storage_->presented_side_geometry_prepared=true;if(error)error->clear();return true;
    }
    bool publish_prop_draw_prefix(const BattlePropPublicationProfile& profile,ScenePhysicalPools& physical,ModelPassQueue& queue,const std::shared_ptr<const HsdJointWorldScope>& scope,u64 generation,u8 active_view_mask,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("stage prop draw prefix: ")+why;return false;};
        if(!profile.ready()||!publication_current()||storage_->publication_scope.lock()!=scope||!object_collection_current(physical)||!queue.ready()||!generation||!storage_->published_props.empty()||!active_view_mask)return reject("fresh current stage callback/object list/same model queue required");
        struct Pending {PropEntry* entry;BattlePropPublishedModel model;HsdJointInstanceStateSet world;};
        std::vector<Pending> pending;pending.reserve(48);
        auto link=physical.scheduler()->render_heads[ScenePhysicalSchedulerSnapshot::object_render_base];auto& arena=*storage_->objects->storage_;
        for(unsigned i=0;i<48;++i){
            const auto ref=object_ref(link);if(!ref||!ref->prop)return reject("actual complete48-prop render-list prefix required");
            auto& state=*ref->prop;const u8 mask=state.static_prop?0x14:0x17;
            if(!state.constructor_returned||state.render82!=1||state.update_generation!=1||(state.contact.hidden270&active_view_mask)||state.contact_generation!=u64(state.collision86?2:0))return reject("camera-invariant normal prop visibility/current constructor/update/contact fields required");
            const auto& header=state.static_prop?arena.state.records[state.class2.index()].header:arena.class3_models[state.class3.index()].header;
            if(header.draw10c_key!=(state.static_prop?0x80087c84u:0x80087b64u))return reject("actual source prop draw callback");
            auto found=std::find_if(storage_->props.begin(),storage_->props.end(),[&](const auto& entry){return &entry.state==ref->prop;});if(found==storage_->props.end())return reject("same owned prop entry");
            Pending p;p.entry=&*found;p.model={link,p.entry->descriptors[0].index(),state.record13,mask,generation};
            if(!SceneModelBindings::prepare_authored_translation_world(*storage_->models,p.entry->descriptors[0],p.entry->archive,p.entry->roots[0],state.record13,generation,&p.world,error))return false;
            for(const auto& before:pending)if(before.model.object==link||before.model.descriptor==p.model.descriptor)return reject("render-list cycle/shared root");
            pending.push_back(std::move(p));link=state.static_prop?arena.state.records[state.class2.index()].render_next110:arena.class3.records[state.class3.index()].render_next110;
        }
        const auto following=object_ref(link);if(!following||!following->side)return reject("source side callbacks must remain pending after the prop prefix");
        try{
            storage_->published_props.reserve(48);
            for(auto& p:pending){
                const auto& lease=p.entry->descriptors[0];const u32 id=u32(lease.index())+1;
                if(!SceneModelBindings::commit_world(*storage_->models,lease,p.entry->roots[0],scope,std::move(p.world))||!SceneModelBindings::set_published_descriptor_flag(*storage_->models,lease,scope,1)||!queue.register_model(id,p.model.mask,storage_->models->inspect(lease)->fields.payload10_1f[0]!=0))return fail(error,"uncertain source prop root/queue publication; prefix retained");
                std::array<u32,3> links;if(!queue.source_links(id,&links)||!SceneModelBindings::bind_queue_links(*storage_->models,lease,scope,links,generation))return fail(error,"source prop queue links refused");
                storage_->published_props.push_back(p.model);
            }
            storage_->next_object_draw=link;if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain prop draw prefix; no replay");}
    }
    bool publication_current()const{
        if(!current()||storage_->published.size()!=2)return false;
        const auto scope=storage_->publication_scope.lock();if(!scope||!scope->live())return false;
        for(const auto& p:storage_->published){const auto* root=storage_->models->root(storage_->stage_roots[p.slot].lease);if(!root||!root->world_from(scope)||root->state()->world_generation!=p.world_generation)return false;}
        return true;
    }
    bool publish_initial_normal_models(ModelPassQueue& queue,const std::shared_ptr<const HsdJointWorldScope>& scope,u64 generation,std::string* error){
        if(phase_!=BattleStageConstructionPhase::InitializedAwaitFrame||!current()||!storage_->published.empty()||!scope||!scope->live()||!queue.ready()||!generation)return fail(error,"fresh initialized stage/source model queue required");
        std::array<HsdJointInstanceStateSet,2> worlds;
        const std::array<unsigned,2> slots{0,3};
        for(unsigned i=0;i<2;++i){const auto& e=storage_->stage_roots[slots[i]];
            if(!SceneModelBindings::prepare_static_root_world(*storage_->models,e.lease,storage_->main_archive,e.root,u16(i*2),generation,&worlds[i],error))return false;}
        try{
            storage_->published.reserve(2);storage_->publication_scope=scope;
            for(unsigned i=0;i<2;++i){const auto slot=slots[i];const auto& e=storage_->stage_roots[slot];const auto id=u32(e.lease.index())+1;
                if(!SceneModelBindings::commit_world(*storage_->models,e.lease,e.root,scope,std::move(worlds[i]))||
                   !SceneModelBindings::set_published_descriptor_flag(*storage_->models,e.lease,scope,1)||
                   !queue.register_model(id,0x0f,storage_->models->inspect(e.lease)->fields.payload10_1f[0]!=0))return fail(error,"uncertain initial stage root/queue publication");
                std::array<u32,3> links;if(!queue.source_links(id,&links)||!SceneModelBindings::bind_queue_links(*storage_->models,e.lease,scope,links,generation))return fail(error,"source stage queue links refused");
                storage_->published.push_back({u8(slot),e.lease.index(),u16(i*2),generation});
            }
            if(error){error->clear();}return true;
        }catch(const std::exception&){return fail(error,"uncertain stage publication; consumed prefix retained");}
    }
    bool prepare_published_geometry(std::string* error){
        if(!publication_current()||storage_->geometry_prepared){if(error)*error="stage geometry: fresh current publication required";return false;}
        try{
            AttachmentModelBank bank;if(!bank.load_prepared(*storage_->main_archive.bank(),error))return false;
            std::array<AttachmentModelScene,2> geometry;
            for(unsigned i=0;i<2;++i){
                const auto& p=storage_->published[i];const auto* root=stage_model(p.slot);
                if(!root||!root->state()->joints[0].world||!geometry[i].load_shared(bank,p.record,error))return false;
                AttachmentBoneTransform transform;transform.valid=true;transform.world=*root->state()->joints[0].world;
                if(!geometry[i].set_world_static(transform,error))return false;
            }
            storage_->geometry=std::move(geometry);storage_->geometry_prepared=true;if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain stage rigid geometry preparation");}
    }
    bool models_current(const ModelOwner& e,unsigned count)const{
        if(!e.scope||!e.scope->live()||!e.archive.bank())return false;
        for(unsigned i=0;i<count;++i){const auto* root=storage_->models->root(e.descriptors[i]);if(!root||!root->receipt().same_instance(e.roots[i]))return false;}return true;
    }
    void invalidate_scopes(){if(storage_){for(auto& e:storage_->entries)if(e.scope)e.scope->live_=false;for(auto& e:storage_->props)if(e.scope)e.scope->live_=false;}}
    bool fail(std::string* error,const char* why){
        phase_=BattleStageConstructionPhase::Failed;invalidate_scopes();
        if(error){*error=std::string("stage side construction: ")+why;}return false;
    }
    bool bind_normal(ModelOwner& owner,unsigned slot,u16 record,std::string* error){
        HsdJointInstanceBank instance;if(!instance.construct_normal_records(*owner.archive.bank(),{&record,1},error))return false;
        owner.roots[slot]=instance.receipt(0);HsdJointInstanceOwner root;
        return instance.take_instance(0,&root,error)&&SceneModelBindings::attach_normal(*storage_->models,owner.descriptors[slot],owner.archive,record,std::move(root),error);
    }
    bool bind_collisions(const SceneClass3Reference& parent,u8 selector,u32& parameter,u8& collision,std::string* error){
        parameter=0x802c4760;const auto b=storage_->common.block(0);auto& pool=*storage_->objects;
        if(!parent.live()||!b.contains(4*unsigned(selector),4))return false;
        for(unsigned k=0;k<4&&b.data[4*unsigned(selector)+k]!=255;++k){
            BattleStageCollisionAttempt attempt;attempt.selector=selector;attempt.record=b.data[4*unsigned(selector)+k];const u32 offset=0x400+unsigned(attempt.record)*0x50;if(!b.contains(offset,0x50))return false;
            attempt.result=pool.acquire_collision(48,false,{},&attempt.lease,error);storage_->collisions.push_back(attempt);
            if(attempt.result==SceneObjectAcquire::Refused)return false;
            if(attempt.result!=SceneObjectAcquire::Allocated)continue;
            SceneStageCollisionFields fields;fields.parent20=fields.parent24=fields.matrix_owner60=parent;fields.source=storage_->common;fields.descriptor_offset28=offset;fields.parameter_key2c=parameter;fields.self5c={4,attempt.lease.index()};fields.selector14=selector;fields.record12=attempt.record;fields.parameter11=u8(b.be16(offset+4));fields.start48=std::bit_cast<s16>(b.be16(offset+6));fields.end4a=std::bit_cast<s16>(b.be16(offset+8));
            if(!pool.install_stage_collision(attempt.lease,fields))return false;
            collision=1;
        }
        return true;
    }
    bool construct_prop(PropEntry& e,std::string* error){
        auto& s=e.state;auto& pool=*storage_->objects;e.archive=storage_->main_archive;e.scope=std::make_shared<SceneStageObjectLifetime>();
        SceneObjectHeader header;header.update0c_key=0x80087b58;
        if(s.static_prop){
            auto& r=pool.storage_->state.records[s.class2.index()];r.active00=1;r.header=header;r.header_installed=true;e.descriptors[0]=pool.storage_->leases[s.class2.index()][0];
        }else{
            auto& r=pool.storage_->class3.records[s.class3.index()];auto& m=pool.storage_->class3_models[s.class3.index()];r.active00=1;r.update0c_key=header.update0c_key;m.header=header;m.header_installed=true;
            for(unsigned i=0;i<2;++i)e.descriptors[i]=pool.storage_->class3_leases[s.class3.index()][i];
        }
        s.team88=4;
        if(!bind_normal(e,0,s.record13,error))return false;
        const auto& authored=storage_->models->root(e.descriptors[0])->state()->joints.front().joint;
        for(unsigned k=0;k<3;++k){
            s.position20[k]=authored.translation[k];const f32 angle=f32(bits_to_f32(0x4622f983)*authored.rotation[k]);
            if(!std::isfinite(angle)||f64(angle)<-2147483648.||f64(angle)>2147483647.)return false;
            s.angles70[k]=std::bit_cast<s16>(u16(u32(s32(angle))));
        }
        s.flags84=0x0e;AttachmentWorldPositionInput input;input.position20=s.position20;input.angles=s.angles70;AttachmentBoneTransform transform;
        if(!attachment_world_position_transform(input,&transform,error))return false;
        s.world114=transform.world;
        if(s.static_prop)pool.storage_->state.records[s.class2.index()].header.draw10c_key=0x80087c84;
        else{
            pool.storage_->class3_models[s.class3.index()].header.draw10c_key=0x80087b64;
            if(!bind_normal(e,1,s.record13,error)||!SceneModelBindings::style_stage_shadow(*storage_->models,e.descriptors[1],false,error))return false;
            s.self8c.lease_=s.class3;s.self8c.scope_=e.scope;s.scale58={1,1,1};s.scale_b4=s.scale_b8=s.scale_c0=s.scale_c4=1;
        }
        return true;
    }
    bool finish_props_and_roots(const BattleStagePropsProfile& profile,ScenePhysicalPools& physical,const BattleStageResources& main,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("stage props construction: ")+why;return false;};
        if(phase_!=BattleStageConstructionPhase::AwaitProps||!current()||!profile.ready()||!physical.current()||physical.class2_pool()!=storage_->objects||physical.stage_seed()->stage!=2||physical.stage_seed()->phase00!=1||physical.stage_seed()->draw08!=1||physical.recipe()->class2_profile().kind(40)!=0x16||!main.current()||main.inputs()->stage()!=2||main.bank()->records()!=51)return reject("same current stage2 side prefix/main bank/source operands required");
        const auto b=storage_->common.block(0);
        for(unsigned record=3;record<=32;++record)for(u8 selector:profile.selectors(record))if(selector!=255){
            if(!b.contains(unsigned(selector)*4,4))return reject("collision selector extent");
            for(unsigned i=0;i<4&&b.data[unsigned(selector)*4+i]!=255;++i)if(!b.contains(0x400+unsigned(b.data[unsigned(selector)*4+i])*0x50,0x50))return reject("collision descriptor extent");
        }
        try{
            storage_->props.reserve(48);storage_->collisions.reserve(storage_->collisions.size()+240);storage_->main_archive=main.storage_->archive;phase_=BattleStageConstructionPhase::PropsRunning;
            auto& pool=*storage_->objects;std::optional<unsigned> retained_r31;
            for(unsigned record=3;record<=32;++record){
                storage_->props.emplace_back();auto& e=storage_->props.back();auto& s=e.state;s.record13=u8(record);
                s.result=pool.acquire_class3(40,false,2,&s.class3,error);retained_r31.reset();
                if(s.result==SceneObjectAcquire::Refused)return fail(error,"ordinary prop allocation refused");
                if(s.result!=SceneObjectAcquire::Allocated)break; //source exits this range
                retained_r31=unsigned(storage_->props.size()-1);
                if(!construct_prop(e,error))return fail(error,"ordinary prop constructor refused");
                for(u8 selector:profile.selectors(record))if(selector!=255&&!bind_collisions(s.self8c,selector,s.parameter27c,s.collision86,error))return fail(error,"ordinary prop collision consumer refused");
                s.enabled26f=s.enabled281=1;s.bound204=bits_to_f32(0x47435000);s.constructor_returned=true;
            }
            for(unsigned record=33;record<=50;++record){
                storage_->props.emplace_back();auto& e=storage_->props.back();auto& s=e.state;s.record13=u8(record);s.static_prop=true;
                s.result=pool.acquire(40,false,1,{},&s.class2,error);
                if(s.result==SceneObjectAcquire::Refused)return fail(error,"static prop allocation refused");
                if(s.result!=SceneObjectAcquire::Allocated)break;
                //80087B08 really writes through retained r31, not new r27.
                //An earlier ordinary allocation failure leaves r31 null. Refuse
                //that source-invalid store after retaining this consumed slot.
                if(!retained_r31)return fail(error,"source static-prop type11 destination is null after ordinary allocation failure");
                auto& previous=storage_->props[*retained_r31].state;previous.type11=1;pool.storage_->class3_models[previous.class3.index()].header.type11=1;
                if(!construct_prop(e,error))return fail(error,"static prop constructor refused");
                s.constructor_returned=true;
            }
            phase_=BattleStageConstructionPhase::RootsRunning;
            for(unsigned i=0;i<4;++i){
                auto& entry=storage_->stage_roots[i];if(storage_->models->acquire(&entry.lease,error)!=SceneDescriptorAcquire::Acquired)return fail(error,"stage-root descriptor allocation failed; consumed prefix retained");
                physical.storage_->stage.model_indices58_64[i]=entry.lease.index();
                const u16 record=i<2?0:u16(i-1);HsdJointInstanceBank instance;
                if(!instance.construct_normal_records(*storage_->main_archive.bank(),{&record,1},error))return fail(error,"stage-root metadata construction refused");
                entry.root=instance.receipt(0);HsdJointInstanceOwner root;if(!instance.take_instance(0,&root,error))return fail(error,"stage-root exclusive transfer refused");
                if(i==2){
                    const auto& fields=storage_->models->inspect(entry.lease)->fields;const ByteView cache{fields.payload10_1f.data(),16};
                    entry.context=std::make_shared<const SceneModelMaterialContext>(std::array<u32,2>{cache.be32(8),cache.be32(12)},SceneModelMaterialKind::ColourWrapper);
                    if(!SceneModelBindings::attach_colour_context(*storage_->models,entry.lease,storage_->main_archive,record,std::move(root),entry.context,error)||!SceneModelBindings::set_descriptor_cache_colour(*storage_->models,entry.lease,entry.context,255)||!SceneModelBindings::set_descriptor_cache_word(*storage_->models,entry.lease,entry.context,1,0xffffffffu))return fail(error,"070E4 stage colour-wrapper binding refused");
                }else{
                    if(!SceneModelBindings::attach_normal(*storage_->models,entry.lease,storage_->main_archive,record,std::move(root),error))return fail(error,"stage normal root binding refused");
                    if(i==1&&!SceneModelBindings::style_stage_shadow(*storage_->models,entry.lease,true,error))return fail(error,"stage shadow root styling refused");
                }
            }
            phase_=BattleStageConstructionPhase::InitializedAwaitFrame;if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain prop/root construction; source prefix retained");}
    }
    bool begin_side(const BattleStageSideProfile& profile,ScenePhysicalPools& physical,const BattleSideResources& sides,const BattleCommonResources& common,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("stage side construction: ")+why;return false;};
        if(attempted_||!profile.ready()||!physical.current()||!physical.stage_seed()||physical.stage_seed()->stage!=2||physical.stage_seed()->phase00||physical.stage_seed()->draw08||!sides.current()||sides.count()!=2||!common.ready())return reject("fresh current stage2/side/common owners required");
        for(const auto& head:physical.scheduler()->callback_heads)if(head.present())return reject("fresh source callback lists required");
        for(const auto& tail:physical.scheduler()->callback_tails)if(tail.present())return reject("fresh source callback tails required");
        const auto raw=sides.placement();if(!raw.contains(0,228)||raw.be32(0)!=8)return reject("bounded actual eight-row stage2 placement block required");
        //Validate source data before the irreversible phase increment. This is
        //not a caller-supplied placement list or a reset of occupied pools.
        std::array<BattleStageSideObject,8> rows{};
        for(unsigned i=0;i<8;++i){
            auto& s=rows[i];const unsigned at=4+28*i;s.ordinal=u8(i);s.type11=raw.data[at];s.record13=raw.data[at+1];s.variant12=raw.data[at+2];s.bank18b=raw.data[at+3];
            if(!profile.type(s.type11)||s.variant12>1||s.bank18b>=sides.count()||!sides.bank(s.bank18b)||s.variant12>=sides.bank(s.bank18b)->records())return reject("unsupported placement type/bank/record");
            for(unsigned k=0;k<3;++k){s.local64[k]=raw.bef32(at+4+4*k);const f32 angle=f32(bits_to_f32(0x4622f983)*raw.bef32(at+16+4*k));if(!std::isfinite(s.local64[k])||std::abs(s.local64[k])>1e4f||!std::isfinite(angle)||f64(angle)<-2147483648.||f64(angle)>2147483647.)return reject("placement position/angle domain");s.angles70[k]=std::bit_cast<s16>(u16(u32(s32(angle))));}
            const auto selector=profile.type(s.type11)->collision;const auto b=common.context98();if(selector>255||!b.contains(4*selector,4))return reject("source collision selector extent");
            for(unsigned k=0;k<4&&b.data[4*selector+k]!=255;++k)if(!b.contains(0x400+unsigned(b.data[4*selector+k])*0x50,0x50))return reject("source collision record extent");
        }
        attempted_=true;
        try{
            storage_=std::make_unique<Storage>();storage_->objects=physical.class2_pool();storage_->models=physical.storage_->domain;storage_->common=common.binding();storage_->entries.reserve(8);storage_->collisions.reserve(32);
            phase_=BattleStageConstructionPhase::SideObjectsRunning;
            ++physical.storage_->stage.phase00;physical.storage_->stage.draw08=1;physical.storage_->banks[1].records[0].active00=physical.storage_->stage.phase00;
            for(const auto& row:rows){
                storage_->entries.emplace_back();auto& entry=storage_->entries.back();entry.state=row;auto& s=entry.state;const auto& type=*profile.type(s.type11);auto& pool=*storage_->objects;
                s.result=s.variant12?pool.acquire(40,false,1,{},&s.class2,error):pool.acquire_class3(40,false,2,&s.class3,error);
                if(s.result==SceneObjectAcquire::Refused)return fail(error,"allocation domain refused; consumed source prefix retained");
                if(s.result!=SceneObjectAcquire::Allocated)continue; //81980 continues subsequent placements
                entry.scope=std::make_shared<SceneStageObjectLifetime>();entry.archive=sides.storage_->leases[s.bank18b];
                const SceneObjectHeader header{4,s.type11,s.variant12,0x80081d98,0x800820c8};
                if(s.variant12){
                    if(!pool.install_header(s.class2,header))return fail(error,"class2 header already consumed");
                    pool.storage_->state.records[s.class2.index()].kind83=0x16;entry.descriptors[0]=pool.storage_->leases[s.class2.index()][0];
                }else{
                    auto& r=pool.storage_->class3.records[s.class3.index()];auto& models=pool.storage_->class3_models[s.class3.index()];
                    if(r.active00||models.header_installed)return fail(error,"class3 header already consumed");
                    r.active00=1;r.kind83=0x16;r.update0c_key=header.update0c_key;models.header=header;models.header_installed=true;
                    for(unsigned i=0;i<2;++i)entry.descriptors[i]=pool.storage_->class3_leases[s.class3.index()][i];
                }
                s.team88=4;s.flags84=0xf;
                for(unsigned i=0;i<(s.variant12?1u:2u);++i){
                    const u16 record=s.variant12;HsdJointInstanceBank instance;if(!instance.construct_normal_records(*entry.archive.bank(),{&record,1},error))return fail(error,"side source root construction refused");
                    entry.roots[i]=instance.receipt(0);HsdJointInstanceOwner root;if(!instance.take_instance(0,&root,error))return fail(error,"side root transfer refused");
                    if(!s.variant12&&!i){
                        const auto& fields=storage_->models->inspect(entry.descriptors[i])->fields;
                        const ByteView cache{fields.payload10_1f.data(),fields.payload10_1f.size()};
                        entry.context=std::make_shared<const SceneModelMaterialContext>(std::array<u32,2>{cache.be32(8),cache.be32(12)});
                        if(!SceneModelBindings::attach_descriptor_context(*storage_->models,entry.descriptors[i],entry.archive,record,std::move(root),entry.context,error))return fail(error,"scoped side material binding refused");
                        if(!SceneModelBindings::set_descriptor_cache_colour(*storage_->models,entry.descriptors[i],entry.context,255))return fail(error,"descriptor cache write refused");
                    }else if(!SceneModelBindings::attach_normal(*storage_->models,entry.descriptors[i],entry.archive,record,std::move(root),error))return fail(error,"side normal model binding refused");
                }
                s.factors144_174.fill(1);
                if(s.variant12){
                    AttachmentBoneTransformInput input;input.flags=0xf;input.local_position=s.local64;input.angles=s.angles70;AttachmentBoneTransform transform;
                    if(!attachment_identity_transform(input,&transform,error))return fail(error,"static side world transform refused");
                    s.position20=transform.position;s.world114=transform.world;s.mask18a=0xf;
                }else{
                    //89100(1,append) uses the source scheduler and actual104/108
                    //links. Allocation does not itself register this callback.
                    auto& r=pool.storage_->class3.records[s.class3.index()];auto& scheduler=physical.storage_->scheduler;
                    r.callback100_key=0x80082060;r.registration85=1;r.callback_previous104=scheduler.callback_tails[1];r.callback_next108={};
                    const ScenePhysicalLink self{3,s.class3.index()};
                    if(r.callback_previous104.present()){
                        const auto prior=r.callback_previous104;if(prior.physical!=3||unsigned(prior.index)>=pool.storage_->class3.records.size())return fail(error,"unsupported callback neighbor");
                        pool.storage_->class3.records[unsigned(prior.index)].callback_next108=self;
                    }else scheduler.callback_heads[1]=self;
                    scheduler.callback_tails[1]=self;s.mask18a=0x47;
                    if(!SceneModelBindings::style_side_root(*storage_->models,entry.descriptors[1],type.cull,error))return fail(error,"side second-root style refused");
                    s.program178=type.program_key;s.radius17c=type.radius;s.duration188=type.duration;
                    s.self8c.lease_=s.class3;s.self8c.scope_=entry.scope;s.scale_b4=s.scale_b8=s.scale_c0=s.scale_c4=1;s.scale58={1,1,1};
                    const auto selector=u8(type.collision);
                    if(!bind_collisions(s.self8c,selector,s.parameter27c,s.collision86,error))return fail(error,"same-owner non-actor collision binding refused");
                    s.selector1b=selector;s.bound204=bits_to_f32(0x47435000);s.enabled281=s.enabled26f=1;s.hp1c4_1c8.fill(type.hp);
                }
                s.constructor_returned=true;
            }
            storage_->side_globals=std::array<u32,16>{};phase_=BattleStageConstructionPhase::AwaitProps;if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain side construction; consumed prefix retained");}
    }
    std::unique_ptr<Storage> storage_;bool attempted_=false;BattleStageConstructionPhase phase_=BattleStageConstructionPhase::Closed;
};
}
