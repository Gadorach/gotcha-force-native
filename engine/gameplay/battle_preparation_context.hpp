#pragma once
#include "engine/gameplay/challenge_battle_roster.hpp"
#include "engine/gameplay/battle_preparation_models.hpp"
#include "engine/gameplay/battle_preparation_update.hpp"
namespace gf {
struct BattlePreparationContextState {u8 scene45=0;bool scene45_known=false;u64 native_revision=0;std::optional<u8> root00,substate01;std::optional<u32> countera8,counterac,counterb0;};
struct BattlePreparationContextPending {bool entry_scene45_store=true,entry_branch_inputs=true,bootstrap_consumers=true,wait295e0=true,update29708=true,late29770=true,widget_dispatch=true,current_scene=true;};
enum class BattlePreparationContextStatus {Closed,AtEntryBranch,Failed,Retired,AtBootstrap};
//Partial native scene-data owner.2844C FIRST store only, stopping BEFORE common
//64/50 branch reads or any resource/scene call. No guessed root/substate/counters,
//full2844C acknowledgement, waiting update, tick, frame or widget dispatch token.
class BattlePreparationContext {
public:
    BattlePreparationContext()=default;BattlePreparationContext(const BattlePreparationContext&)=delete;BattlePreparationContext& operator=(const BattlePreparationContext&)=delete;
    BattlePreparationContext(BattlePreparationContext&&)=default;BattlePreparationContext& operator=(BattlePreparationContext&&)=default;~BattlePreparationContext(){retire();}
    bool begin_2844c_prefix(ChallengeBattleRoster& roster,BattlePreparationWidgets& widgets,const BattlePreparationModels& models,const BattlePreparationUpdateProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle preparation context: ")+why;return false;};
        if(attempted_||!profile.ready()||!roster.current(owners)||!widgets.current()||!models.current()||models.archive_pool1_join_pending())return reject("fresh/current roster/widgets/archive-bound resources/profile required");
        const auto r=roster.storage_;const auto w=widgets.storage_;if(models.storage_->widgets.lock()!=w||r->preparation_context_claimed||r->scene45||w->preparation_context_claimed)return reject("same unclaimed widget/resource scope required");
        const auto allocation=r->allocation.lock();const auto entry=allocation?allocation->party.preparation_entry_.lock():nullptr;if(entry&&(!entry->live||entry->prefix_claimed||entry->branch_claimed))return reject("fresh normal entry prefix already claimed/retired");
        attempted_=true;status_=BattlePreparationContextStatus::Failed;r->preparation_context_claimed=w->preparation_context_claimed=true;
        if(entry)entry->prefix_claimed=true;
        try{auto next=std::make_unique<Storage>();next->roster=r;next->widgets=w;next->model_scope=models.storage_->identity;next->archive=models.storage_->archive;next->bank=models.storage_->bank;next->state.scene45=profile.initial_scene45();next->state.scene45_known=true;next->state.native_revision=1;
            if(!r->current(owners)||!w->current()||!next->archive.bank()){return reject("retired before context publication");}if(entry){entry->scene45=next->state.scene45;next->entry=entry;next->entry_prefix_bound=true;}r->scene45=next->state.scene45;++r->revision;storage_=std::move(next);status_=BattlePreparationContextStatus::AtEntryBranch;if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain context allocation; claim retained/no replay");}
    }
    //Consumes ONLY the same party's privately propagated fresh entry owner.
    //The normal64==0/50!=0 short-circuit reads no busy/substate. Root00++ and
    //01=0 are real writes in that backing owner. Stop BEFORE197854(1), not an
    //acknowledgement of descriptor/physical-pool initialization or callbacks.
    bool advance_normal_entry_branch(const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle preparation entry branch: ")+why;return false;};
        if(status_!=BattlePreparationContextStatus::AtEntryBranch||!current(owners))return reject("same current context at28464 required");
        const auto roster=storage_->roster.lock();const auto allocation=roster->allocation.lock();if(!allocation)return reject("allocation retired");
        const auto entry=allocation->party.preparation_entry_.lock();if(!entry||!entry->live||!entry->handoff_ready||!entry->prefix_claimed||entry->branch_claimed||entry->state.common64!=0||entry->state.common50!=1||entry->state.root00!=0||entry->state.substate01!=0||entry->state.native_revision!=1)return reject("same privately owned fresh-normal entry and completed final handoff required; no inferred/late seed");
        entry->branch_claimed=true;++entry->state.root00;entry->state.substate01=0;++entry->state.native_revision;
        storage_->entry=entry;storage_->state.root00=entry->state.root00;storage_->state.substate01=entry->state.substate01;storage_->state.countera8=entry->state.countera8;storage_->state.counterac=entry->state.counterac;storage_->state.counterb0=entry->state.counterb0;++storage_->state.native_revision;++roster->revision;status_=BattlePreparationContextStatus::AtBootstrap;if(error)error->clear();return true;
    }
    bool current(const std::array<const ForceInventory*,2>& owners)const{if((status_!=BattlePreparationContextStatus::AtEntryBranch&&status_!=BattlePreparationContextStatus::AtBootstrap)||!storage_)return false;const auto r=storage_->roster.lock();const auto w=storage_->widgets.lock();const auto* bank=storage_->archive.bank();if(!r||!r->current(owners)||r->scene45!=storage_->state.scene45||!w||!w->current()||storage_->model_scope.expired()||!bank||!bank->same_source(storage_->bank))return false;if(storage_->entry_prefix_bound){const auto entry=storage_->entry.lock();if(!entry||!entry->live||!entry->prefix_claimed||entry->scene45!=storage_->state.scene45)return false;}if(status_==BattlePreparationContextStatus::AtBootstrap){const auto entry=storage_->entry.lock();return entry&&entry->live&&entry->branch_claimed&&entry->state.root00==storage_->state.root00&&entry->state.substate01==storage_->state.substate01;}return true;}
    const BattlePreparationContextState* state(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?&storage_->state:nullptr;}
    BattlePreparationContextPending pending()const{BattlePreparationContextPending p;if(storage_)p.entry_scene45_store=false;if(status_==BattlePreparationContextStatus::AtBootstrap)p.entry_branch_inputs=false;return p;}
    u32 next_source_address(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?(status_==BattlePreparationContextStatus::AtBootstrap?0x800284f4:0x80028464):0;}
    bool widget_dispatch_authority_ready()const{return false;}
    BattlePreparationContextStatus status()const{return status_;}
    void retire(){storage_.reset();status_=BattlePreparationContextStatus::Retired;attempted_=true;} //no source scene/frame cleanup acknowledgement
private:
    struct Storage {std::weak_ptr<challenge_battle_roster_detail::Storage> roster;std::weak_ptr<BattlePreparationWidgets::Storage> widgets;std::weak_ptr<const BattlePreparationModelScope> model_scope;std::weak_ptr<challenge_preparation_entry_detail::Storage> entry;bool entry_prefix_bound=false;SceneArchiveLease archive;HsdRecordAnimationBank bank;BattlePreparationContextState state;};
    std::unique_ptr<Storage> storage_;bool attempted_=false;BattlePreparationContextStatus status_=BattlePreparationContextStatus::Closed;
};
} // namespace gf
