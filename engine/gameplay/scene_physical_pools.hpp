#pragma once
#include "engine/assets/scene_physical_pools.hpp"
#include "engine/gameplay/scene_descriptor_bootstrap.hpp"
#include "engine/gameplay/scene_object_pool.hpp"
#include "engine/gameplay/scene_physical_fields.hpp"
#include "engine/gameplay/battle_fixed_slots.hpp"
#include "engine/assets/battle_stage_seed.hpp"
#include <optional>
namespace gf {
//Class2/3/4 share an allocation owner, not constructors or current frame data.
struct BattleStageSeedState {u8 phase00=0,draw08=0;u32 stage=0;std::array<f32,3> translation10{};f32 high34=0,low68=0;std::array<f32,4> bounds38_44{},expanded48_54{};};
struct ScenePhysicalSchedulerSnapshot {
    std::array<s32,12> callback_heads,callback_tails;std::array<s32,30> render_heads,render_tails;std::array<u16,10> registry_counts{};
    //8853C does NOT establish these three late-call queue counters, nor does it
    //clear registry reference arrays. Unknown is not an empty active registry.
    std::array<std::optional<u32>,3> late_queue_counts{};
    ScenePhysicalSchedulerSnapshot(){callback_heads.fill(-1);callback_tails.fill(-1);render_heads.fill(-1);render_tails.fill(-1);}
    bool operator==(const ScenePhysicalSchedulerSnapshot&)const=default;
};
class ScenePhysicalPools {
public:
    ScenePhysicalPools()=default;ScenePhysicalPools(const ScenePhysicalPools&)=delete;ScenePhysicalPools& operator=(const ScenePhysicalPools&)=delete;
    ScenePhysicalPools(ScenePhysicalPools&&)=default;ScenePhysicalPools& operator=(ScenePhysicalPools&&)=default;~ScenePhysicalPools(){retire();}
    bool begin_fresh(const ScenePhysicalPoolsProfile& profile,const SceneDescriptorBootstrap& descriptors,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("physical pools: ")+why;return false;};
        if(attempted_||!profile.ready()||!descriptors.pristine_for_physical_join()){return reject("fresh untouched same descriptor domain/profile required");}attempted_=true;
        try{auto next=std::make_unique<Storage>();next->profile=profile;next->domain=descriptors.model_pool();next->archives=descriptors.archive_pool();next->class2=std::make_shared<SceneObjectPool>();
            for(unsigned i=0;i<8;++i){
                if(i==2){if(!next->class2->begin_fresh(profile.class2_profile(),next->domain,error))return false;continue;}
                const auto& row=*profile.row(i);
                if(i==3){if(!next->class2->initialize_class3_pool(row.count))return reject("class3 bank recipe");continue;}
                if(i==4){if(!next->class2->initialize_collision_pool(row.count))return reject("class4 bank recipe");continue;}
                auto& b=next->banks[i];if(!row.count)continue;
                b.runtime_class=s8(row.source_class);b.free_head=0;b.free_tail=s32(row.count)-1;b.free_count=row.count;b.records.resize(row.count);
                for(unsigned j=0;j<row.count;++j){
                    auto& r=b.records[j];r.class02=row.source_class;r.physical03=u8(i);
                    if(j+1<row.count)r.free_next4={u8(i),s32(j+1)};
                }
            }
            next->controls=std::make_shared<ScenePhysicalControlFields>();auto& c=*next->controls;c.d1=0;c.d2=0;c.d3=0;c.d4=0;c.d6=0;c.d9=0;c.d8=0;c.da=0;c.db=0;c.eb=0;c.e8=0;
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain native allocation; no reset/retry acknowledgement");}
    }
    bool current()const{if(!storage_||!storage_->class2||!storage_->class2->ready())return false;const auto a=storage_->archives.lock();return a&&a->matches_descriptor_owner(storage_->domain);}
    //The historical class2 accessor exposes the one shared group/link owner.
    std::shared_ptr<SceneObjectPool> class2_pool()const{return current()?storage_->class2:nullptr;}
    const ScenePhysicalBankSnapshot* bank(unsigned i)const{
        if(!current()||i>=8||i==2)return nullptr;
        if(i==3)return storage_->class2->class3_snapshot();
        if(i==4)return storage_->class2->collision_snapshot();
        return &storage_->banks[i];
    }
    const ScenePhysicalSchedulerSnapshot* scheduler()const{return current()?&storage_->scheduler:nullptr;}
    const ScenePhysicalControlFields* controls()const{return current()?storage_->controls.get():nullptr;}
    const ScenePhysicalPoolsProfile* recipe()const{return current()?&storage_->profile:nullptr;}
    const BattleFixedSlotPlan* actor_seed()const{return current()&&storage_->actor_seeded?&storage_->actors:nullptr;}
    const BattleStageSeedState* stage_seed()const{return current()&&storage_->stage_seeded?&storage_->stage:nullptr;}
    const std::array<ScenePhysicalControlWrite,11>& control_writes()const{static const std::array<ScenePhysicalControlWrite,11> w{{{209,1,0},{210,1,0},{211,1,0},{212,1,0},{214,2,0},{217,1,0},{216,1,0},{218,1,0},{219,1,0},{235,1,0},{232,2,0}}};return w;}
    bool non_class2_consumers_pending()const{return true;}bool live_registry_frame_pending()const{return true;}
    void retire(){storage_.reset();attempted_=true;} //native invalidation, NOT source SDK/object bulk cleanup
private:
    friend class BattlePreparationBootstrap;
    friend class NativeGRedClass3TrialBridge;
    struct TrialAttackRegistry {
        SceneCollisionLease entry;u64 allocation_revision=0;
        std::optional<SceneCollisionLease> receiver{};
    };
    static std::array<u16,10> trial_registry_counts(bool paired){
        std::array<u16,10> counts{1}; // 477C
        if(paired){counts[1]=1;counts[9]=1;} // 417C and 117C
        return counts;
    }
    // A complete singleton only in an isolated trial arena. No live roster,
    // stage, or other allocated shape may be skipped by this entry.
    bool publish_trial_attack_registry(const SceneCollisionLease& child){
        if(!current()||storage_->actor_seeded||storage_->stage_seeded||storage_->trial_attack_registry||
           storage_->scheduler.registry_counts!=std::array<u16,10>{})return false;
        const auto& pool=*storage_->class2;
        const auto* bank=pool.collision_snapshot();
        const auto* body=pool.inspect(child);
        const auto* shape=pool.trial_beam_shape(child);
        const auto* lists=pool.snapshot();
        if(!bank||!body||!shape||!shape->sphere_seed||shape->trial_phase10!=1||body->active00!=1||
           body->group01!=49||bank->free_count+1u!=bank->records.size()||!lists||
           lists->group_heads[49]!=ScenePhysicalLink{4,child.index()}||
           lists->group_tails[49]!=ScenePhysicalLink{4,child.index()})return false;
        storage_->trial_attack_registry=TrialAttackRegistry{child,pool.collision_allocation_revision()};
        // Count array follows the clear order (477C first); list enum uses
        // ascending list addresses (477C last).
        storage_->scheduler.registry_counts[0]=1;
        return true;
    }
    std::optional<SceneCollisionLease> current_trial_attack_registry()const{
        if(!current()||!storage_->trial_attack_registry)return {};
        const auto& registry=*storage_->trial_attack_registry;
        const auto& pool=*storage_->class2;
        if(!pool.trial_beam_shape(registry.entry)||pool.collision_allocation_revision()!=registry.allocation_revision||
           (registry.receiver&&!pool.trial_sasuke_body(*registry.receiver))||
           storage_->scheduler.registry_counts!=trial_registry_counts(registry.receiver.has_value()))return {};
        return registry.entry;
    }
    bool publish_trial_receiver_registry(const SceneCollisionLease& child){
        if(!current()||storage_->actor_seeded||storage_->stage_seeded||!storage_->trial_attack_registry||
           storage_->scheduler.registry_counts!=trial_registry_counts(false))return false;
        auto& registry=*storage_->trial_attack_registry;
        const auto& pool=*storage_->class2;
        const auto* bank=pool.collision_snapshot();
        const auto* body=pool.inspect(child);
        const auto* lists=pool.snapshot();
        const auto revision=pool.collision_allocation_revision();
        if(registry.receiver||!revision||revision-1!=registry.allocation_revision||
           !pool.trial_beam_shape(registry.entry)||!pool.trial_sasuke_body(child)||
           !bank||bank->free_count+2u!=bank->records.size()||!body||body->active00!=1||body->group01!=50||!lists||
           lists->group_heads[50]!=ScenePhysicalLink{4,child.index()}||lists->group_tails[50]!=ScenePhysicalLink{4,child.index()})
            return false;
        registry.receiver=child;registry.allocation_revision=revision;
        storage_->scheduler.registry_counts=trial_registry_counts(true);
        return true;
    }
    std::optional<SceneCollisionLease> current_trial_receiver_registry()const{
        if(!current_trial_attack_registry())return {};
        return storage_->trial_attack_registry->receiver;
    }
    //Fixed arena factory, NOT a free-list pop. 537A8 clears link4 in all six
    //cells. Source global free-count/head/tail metadata is retained but no longer
    //describes an available generic actor free-list; no class0 allocator exists.
    bool seed_fixed_actors(const std::array<BattleFixedSlotInput,6>& inputs,std::string* error){
        if(!current()||storage_->actor_seeded||storage_->banks[0].records.size()!=6){if(error)*error="physical actor seed: current unseeded six-cell bank required";return false;}
        BattleFixedSlotPlan next;if(!battle_fixed_slot_seed(inputs,&next,error))return false;
        for(unsigned i=0;i<6;++i){auto& cell=storage_->banks[0].records[i];cell={};cell.active00=next.slots[i].active00;}
        storage_->actors=std::move(next);storage_->actor_seeded=true;if(error)error->clear();return true;
    }
    bool seed_stage(const BattleStageSeedRow& row,std::string* error){
        if(!current()||!storage_->actor_seeded||storage_->stage_seeded||storage_->banks[1].records.size()!=1){if(error)*error="physical stage seed: current singleton after actor factory required";return false;}
        BattleStageSeedState s;s.stage=row.stage;s.high34=bits_to_f32(row.high_bits);s.low68=bits_to_f32(row.low_bits);s.bounds38_44={5000,-5000,5000,-5000};s.expanded48_54={f32(s.bounds38_44[0]-200),f32(200+s.bounds38_44[1]),f32(s.bounds38_44[2]-200),f32(200+s.bounds38_44[3])};
        storage_->banks[1].records[0]={}; //87718 clears256B without restoring class02/pool03
        storage_->stage=s;storage_->stage_seeded=true;if(error)error->clear();return true;
    }
    std::weak_ptr<ScenePhysicalControlFields> control_binding()const{if(!current())return {};return storage_->controls;}
    struct Storage {ScenePhysicalPoolsProfile profile;std::shared_ptr<SceneDescriptorPool> domain;std::weak_ptr<SceneArchivePool> archives;std::shared_ptr<SceneObjectPool> class2;std::array<ScenePhysicalBankSnapshot,8> banks;ScenePhysicalSchedulerSnapshot scheduler;std::optional<TrialAttackRegistry> trial_attack_registry;std::shared_ptr<ScenePhysicalControlFields> controls;BattleFixedSlotPlan actors;BattleStageSeedState stage;bool actor_seeded=false,stage_seeded=false;~Storage(){if(class2)class2->retire();}};
    std::unique_ptr<Storage> storage_;bool attempted_=false;
};
} // namespace gf
