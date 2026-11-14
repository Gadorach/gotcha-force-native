#pragma once
#include "engine/assets/battle_preparation_widgets.hpp"
#include "engine/gameplay/scene_object_pool.hpp"
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
    bool cue372_required()const{return current()&&storage_->cue372_required;} //not sound delivery
    bool requires_scene_allocation_join()const{return true;}bool models_and_callbacks_pending()const{return true;}
    u32 next_source_update()const{return current()?0x80199c0c:0;}
    BattlePreparationWidgetsStatus status()const{return status_;}
    void retire(){if(storage_)storage_->live=false;storage_.reset();status_=BattlePreparationWidgetsStatus::Retired;attempted_=true;} //no source callback/cleanup/counter-zero ACK
private:
    friend class BattlePreparationModels;
    friend class BattlePreparationContext;
    struct Storage {
        bool live=true,models_preparation_claimed=false,preparation_context_claimed=false;BattlePreparationWidgetProfile profile;std::shared_ptr<SceneObjectPool> pool;const SceneObjectPoolSnapshot* identity=nullptr;std::vector<BattlePreparationWidgetAttempt> attempts;std::vector<SceneObjectLease> leases;u8 counter1653=0;bool cue372_required=false;
        bool current()const{if(!live||!pool||!pool->ready()||pool->snapshot()!=identity)return false;for(const auto& lease:leases)if(!lease.current())return false;return true;}
    };
    std::shared_ptr<Storage> storage_;bool attempted_=false;BattlePreparationWidgetsStatus status_=BattlePreparationWidgetsStatus::Closed;
};
} // namespace gf
