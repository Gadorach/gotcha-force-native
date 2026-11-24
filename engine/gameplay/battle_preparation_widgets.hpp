#pragma once
#include "engine/assets/battle_preparation_widgets.hpp"
#include "engine/gameplay/scene_object_pool.hpp"
#include "engine/gameplay/scene_model_bindings.hpp"
#include "engine/gameplay/source_main_digital_input.hpp"
#include <span>
namespace gf {
enum class BattlePreparationWidgetsStatus {Closed,AwaitWidgetCallbacks,Failed,Retired};
struct BattlePreparationWidgetAttempt {u8 type=0,argument=0,descriptors=0,counter1653_after=0;SceneObjectAcquire result=SceneObjectAcquire::Refused;u16 object_index=65535;u64 object_generation=0;SceneObjectHeader header;bool operator==(const BattlePreparationWidgetAttempt&)const=default;};
//Owns1999C8(3)→1999B60's real shared-pool allocations and header prefix. NOT
//widget models/pose/timers/counter decrements or genuine battle-scene authority.
//Source allocation failures keep the successful prefix and continue ALL attempts.
class BattlePreparationWidgets {
public:
    BattlePreparationWidgets()=default;BattlePreparationWidgets(const BattlePreparationWidgets&)=delete;BattlePreparationWidgets& operator=(const BattlePreparationWidgets&)=delete;
    BattlePreparationWidgets(BattlePreparationWidgets&&)=default;BattlePreparationWidgets& operator=(BattlePreparationWidgets&&)=default;~BattlePreparationWidgets(){retire();}
    bool begin(const BattlePreparationWidgetProfile& profile,const std::shared_ptr<SceneObjectPool>& pool,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle preparation widgets: ")+why;return false;};
        if(attempted_||!profile.ready()||!pool||!pool->ready()||pool->storage_->battle_widgets_claimed)return reject("fresh templates/unclaimed current shared objects required");
        attempted_=true;pool->storage_->battle_widgets_claimed=true;status_=BattlePreparationWidgetsStatus::Failed;
        try{auto s=std::make_unique<Storage>();s->pool=pool;s->identity=pool->snapshot();s->profile=profile;s->attempts.reserve(58);s->leases.reserve(58);storage_=std::move(s);
            auto attempt=[&](u8 type,u8 argument){const auto* row=storage_->profile.find(type);if(!row)return false;BattlePreparationWidgetAttempt entry;entry.type=type;entry.argument=argument;entry.descriptors=row->descriptors;SceneObjectLease lease;
                entry.result=pool->acquire(57,true,row->descriptors,std::nullopt,&lease,error);
                if(entry.result==SceneObjectAcquire::Refused)return false;
                if(entry.result==SceneObjectAcquire::Allocated){entry.object_index=lease.index();entry.object_generation=lease.generation();entry.header={0,type,argument,0x80199c0c,row->draw10c_key};if(!pool->install_header(lease,entry.header))return false;storage_->leases.push_back(lease);storage_->counter1653=u8(unsigned(storage_->counter1653)+1);}
                entry.counter1653_after=storage_->counter1653;storage_->attempts.push_back(entry);return true;
            };
            if(!attempt(4,3)||!attempt(0,0)||!attempt(1,0))return reject("uncertain constructor work; no replay");
            for(u8 i=0;i<18;++i)if(!attempt(2,i))return reject("uncertain type2 work; no replay");
            for(u8 i=0;i<36;++i)if(!attempt(5,i))return reject("uncertain type5 work; no replay");
            if(!attempt(6,0))return reject("uncertain type6 work; no replay");
            storage_->cue372_required=true;status_=BattlePreparationWidgetsStatus::AwaitWidgetCallbacks;if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain native allocation; keep consumed work/no replay");}
    }
    bool current()const{return status_==BattlePreparationWidgetsStatus::AwaitWidgetCallbacks&&storage_&&storage_->current();}
    std::span<const BattlePreparationWidgetAttempt> attempts()const{return current()?std::span<const BattlePreparationWidgetAttempt>{storage_->attempts}:std::span<const BattlePreparationWidgetAttempt>{};}
    std::optional<u8> counter1653()const{return current()?std::optional<u8>{storage_->counter1653}:std::nullopt;}
    const BattleInitialBriefingWidgetPrefix* first_widget_prefix()const{
        if(!current()||!storage_->first_widget_prefix_started)return nullptr;
        const auto* r=storage_->pool->inspect(storage_->leases.front());return &*r->briefing_widget_prefix;
    }
    const SceneMaterialAnimationOwner* first_widget_material_animation(unsigned index)const{return first_widget_prefix()&&index<2?&storage_->first_widget_animation[index]:nullptr;}
    const HsdJointInstanceOwner* first_widget_root(unsigned slot)const{return first_widget_prefix()&&slot<3?SceneModelBindings::object_root(*storage_->pool,storage_->leases.front(),slot):nullptr;}
    const BattleInitialBriefingType0State* type0_widget_state()const{
        if(!current()||!storage_->type0_widget_initialized)return nullptr;
        return &*storage_->pool->inspect(storage_->leases[1])->briefing_type0_update;
    }
    const HsdJointInstanceOwner* type0_widget_root(unsigned slot)const{return type0_widget_state()&&slot<2?SceneModelBindings::object_root(*storage_->pool,storage_->leases[1],slot):nullptr;}
    const BattleInitialBriefingType1State* type1_widget_state()const{
        if(!current()||!storage_->type1_widget_initialized)return nullptr;
        return &*storage_->pool->inspect(storage_->leases[2])->briefing_type1_update;
    }
    const HsdJointInstanceOwner* type1_widget_root(unsigned slot)const{return type1_widget_state()&&slot<3?SceneModelBindings::object_root(*storage_->pool,storage_->leases[2],slot):nullptr;}
    const SceneMaterialAnimationOwner* type1_digit_animation(unsigned digit)const{return type1_widget_state()&&digit<2&&storage_->type1_digit_animation[digit].current()?&storage_->type1_digit_animation[digit]:nullptr;}
    unsigned initialized_type2_widgets()const{return current()?storage_->type2_initialized:0;}
    const BattleInitialBriefingType2State* type2_widget_state(unsigned ordinal)const{
        if(!current()||ordinal>=storage_->type2_initialized)return nullptr;
        return &*storage_->pool->inspect(storage_->leases[3+ordinal])->briefing_type2_update;
    }
    const HsdJointInstanceOwner* type2_widget_root(unsigned ordinal,unsigned slot)const{return type2_widget_state(ordinal)&&slot<3?SceneModelBindings::object_root(*storage_->pool,storage_->leases[3+ordinal],slot):nullptr;}
    const SceneMaterialAnimationOwner* type2_name_animation(unsigned ordinal,unsigned part)const{return type2_widget_state(ordinal)&&part<2&&storage_->type2_animation[ordinal][part].current()?&storage_->type2_animation[ordinal][part]:nullptr;}
    unsigned initialized_type5_widgets()const{return current()?storage_->type5_initialized:0;}
    const BattleInitialBriefingType5State* type5_widget_state(unsigned ordinal)const{
        if(!current()||ordinal>=storage_->type5_initialized)return nullptr;
        return &*storage_->pool->inspect(storage_->leases[21+ordinal])->briefing_type5_update;
    }
    const HsdJointInstanceOwner* type5_widget_root(unsigned ordinal,unsigned slot)const{return type5_widget_state(ordinal)&&slot<5?SceneModelBindings::object_root(*storage_->pool,storage_->leases[21+ordinal],slot):nullptr;}
    const BattleInitialBriefingType6State* type6_widget_state()const{
        if(!current()||!storage_->type6_initialized)return nullptr;
        return &*storage_->pool->inspect(storage_->leases[57])->briefing_type6_update;
    }
    const HsdJointInstanceOwner* type6_widget_root()const{return type6_widget_state()?SceneModelBindings::object_root(*storage_->pool,storage_->leases[57],0):nullptr;}
    bool cue372_required()const{return current()&&storage_->cue372_required;} //not sound delivery
    bool requires_scene_allocation_join()const{return true;}bool models_and_callbacks_pending()const{return true;}
    u32 next_source_update()const{return current()?0x80199c0c:0;}
    BattlePreparationWidgetsStatus status()const{return status_;}
    void retire(){if(storage_)storage_->live=false;storage_.reset();status_=BattlePreparationWidgetsStatus::Retired;attempted_=true;} //no source callback/cleanup/counter-zero ACK
private:
    friend class BattlePreparationBootstrap;
    friend class BattlePreparationModels;
    friend class BattlePreparationContext;
    struct Storage {
        bool live=true,models_preparation_claimed=false,preparation_context_claimed=false;BattlePreparationWidgetProfile profile;std::shared_ptr<SceneObjectPool> pool;const SceneObjectPoolSnapshot* identity=nullptr;std::vector<BattlePreparationWidgetAttempt> attempts;std::vector<SceneObjectLease> leases;u8 counter1653=0;bool cue372_required=false;
        bool first_widget_prefix_started=false;
        bool first_widget_input_bound=false;SourceMainDigitalInputReceipt first_widget_input;
        std::array<HsdJointInstanceReceipt,3> first_widget_roots;
        std::array<SceneMaterialAnimationOwner,2> first_widget_animation;
        bool type0_widget_initialized=false;std::array<HsdJointInstanceReceipt,2> type0_widget_roots;
        bool type1_widget_initialized=false;std::array<HsdJointInstanceReceipt,3> type1_widget_roots;
        std::array<SceneMaterialAnimationOwner,2> type1_digit_animation;
        unsigned type2_initialized=0;
        std::array<std::array<HsdJointInstanceReceipt,3>,18> type2_roots;
        std::array<std::array<SceneMaterialAnimationOwner,2>,18> type2_animation;
        ScenePhysicalLink type2_callback_tail;
        unsigned type5_initialized=0;std::array<std::array<HsdJointInstanceReceipt,5>,36> type5_roots;ScenePhysicalLink type5_callback_tail;
        bool type6_initialized=false;HsdJointInstanceReceipt type6_root;
        bool current()const{
            if(!live||!pool||!pool->ready()||pool->snapshot()!=identity)return false;
            for(const auto& lease:leases)if(!lease.current())return false;
            if(first_widget_prefix_started){
                if(leases.empty())return false;
                const auto* r=pool->inspect(leases.front());
                if(!r||!r->briefing_widget_prefix||!r->briefing_widget_prefix->applied||r->briefing_widget_prefix->awaiting_input==r->briefing_widget_prefix->update_complete)return false;
                if(r->briefing_widget_prefix->update_complete&&!first_widget_input_bound)return false;
                for(unsigned slot=0;slot<3;++slot){const auto* root=SceneModelBindings::object_root(*pool,leases.front(),slot);if(!root||!root->receipt().same_instance(first_widget_roots[slot]))return false;}
                for(unsigned slot=0;slot<2;++slot)if(!first_widget_animation[slot].current()||!first_widget_animation[slot].same_root(*SceneModelBindings::object_root(*pool,leases.front(),slot+1)))return false;
            }
            if(type0_widget_initialized){
                if(leases.size()<2)return false;
                const auto* r=pool->inspect(leases[1]);if(!r||!r->briefing_type0_update||!r->briefing_type0_update->initialized)return false;
                for(unsigned slot=0;slot<2;++slot){const auto* root=SceneModelBindings::object_root(*pool,leases[1],slot);if(!root||!root->receipt().same_instance(type0_widget_roots[slot]))return false;}
            }
            if(type1_widget_initialized){
                if(leases.size()<3)return false;
                const auto* r=pool->inspect(leases[2]);if(!r||!r->briefing_type1_update||!r->briefing_type1_update->initialized)return false;
                for(unsigned slot=0;slot<3;++slot){const auto* root=SceneModelBindings::object_root(*pool,leases[2],slot);if(!root||!root->receipt().same_instance(type1_widget_roots[slot]))return false;}
                if(type1_digit_animation[0].current()||!type1_digit_animation[1].current()||!type1_digit_animation[1].same_root(*SceneModelBindings::object_root(*pool,leases[2],2)))return false;
            }
            for(unsigned ordinal=0;ordinal<type2_initialized;++ordinal){
                if(ordinal>=18||leases.size()<=3+ordinal)return false;
                const auto& lease=leases[3+ordinal];const auto* r=pool->inspect(lease);if(!r||!r->briefing_type2_update||!r->briefing_type2_update->initialized)return false;
                const auto& state=*r->briefing_type2_update;
                for(unsigned slot=0;slot<3;++slot){
                    const auto* root=SceneModelBindings::object_root(*pool,lease,slot);
                    if(state.records[slot]<0){if(root)return false;}
                    else if(!root||!root->receipt().same_instance(type2_roots[ordinal][slot]))return false;
                }
                if(state.active&&!state.variant12)for(unsigned part=0;part<2;++part)if(!type2_animation[ordinal][part].current()||!type2_animation[ordinal][part].same_root(*SceneModelBindings::object_root(*pool,lease,part)))return false;
            }
            for(unsigned ordinal=0;ordinal<type5_initialized;++ordinal){
                if(ordinal>=36||leases.size()<=21+ordinal)return false;
                const auto& lease=leases[21+ordinal];const auto* r=pool->inspect(lease);if(!r||!r->briefing_type5_update||!r->briefing_type5_update->initialized)return false;
                for(unsigned slot=0;slot<5;++slot){const auto* root=SceneModelBindings::object_root(*pool,lease,slot);if(r->briefing_type5_update->records[slot]<0){if(root)return false;}else if(!root||!root->receipt().same_instance(type5_roots[ordinal][slot]))return false;}
            }
            if(type6_initialized){
                if(leases.size()<58)return false;
                const auto* r=pool->inspect(leases[57]);const auto* root=SceneModelBindings::object_root(*pool,leases[57],0);
                if(!r||!r->briefing_type6_update||!r->briefing_type6_update->initialized||!root||!root->receipt().same_instance(type6_root))return false;
            }
            return true;
        }
    };
    std::shared_ptr<Storage> storage_;bool attempted_=false;BattlePreparationWidgetsStatus status_=BattlePreparationWidgetsStatus::Closed;
};
} // namespace gf
