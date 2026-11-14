#pragma once
#include "engine/gameplay/challenge_initial_party.hpp"
#include "engine/gameplay/scene_descriptor_bootstrap.hpp"
#include "engine/gameplay/scene_physical_pools.hpp"
#include "engine/assets/battle_preparation_update.hpp"
#include "engine/gameplay/battle_views.hpp"
namespace gf {
enum class BattlePreparationBootstrapStatus {Closed,AtDescriptors,AtPhysicalPools,Failed,Retired,AtActorSeed,AtStageSeed,AtSceneServices,AtViewServices};
struct BattlePreparationBootstrapState {u8 scene45=0,root00=0,substate01=0;u32 countera8=0,counterac=0,counterb0=0;u64 native_revision=0;};
struct BattlePreparationBootstrapPending {bool descriptor197854=true,physical88398=true,remaining_bootstrap=true,widgets=true,update29708=true,late29770=true,current_scene=true;bool actor_seed536b0=true,stage87718=true,view_initialization=true;};
//Ordered source prefix starts BEFORE descriptor/object/widget allocation. It
//never imports/resets the populated component-context pools. No actor/world/
//render/widget dispatch receipt is produced by fresh descriptor initialization.
class BattlePreparationBootstrap {
public:
    BattlePreparationBootstrap()=default;BattlePreparationBootstrap(const BattlePreparationBootstrap&)=delete;BattlePreparationBootstrap& operator=(const BattlePreparationBootstrap&)=delete;
    BattlePreparationBootstrap(BattlePreparationBootstrap&&)=default;BattlePreparationBootstrap& operator=(BattlePreparationBootstrap&&)=default;~BattlePreparationBootstrap(){retire();}
    bool begin_normal_entry(const ChallengeInitialPartyCandidate& party,const BattlePreparationUpdateProfile& update,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("ordered preparation: ")+why;return false;};
        if(attempted_||!party.current(owners)||!update.ready())return reject("fresh/current party and source profile required");
        const auto entry=party.preparation_entry_.lock();if(!entry||!entry->live||!entry->handoff_ready||entry->prefix_claimed||entry->branch_claimed||entry->state.common50!=1||entry->state.common64||entry->state.root00||entry->state.substate01||entry->state.native_revision!=1)return reject("same unclaimed fresh normal final handoff required");
        attempted_=true;status_=BattlePreparationBootstrapStatus::Failed;entry->prefix_claimed=entry->branch_claimed=true;
        try{auto next=std::make_unique<Storage>();next->party=party;next->entry=entry;next->state.scene45=update.initial_scene45();entry->scene45=next->state.scene45;next->state.native_revision=1;
            ++entry->state.root00;entry->state.substate01=0;++entry->state.native_revision;next->state.root00=entry->state.root00;next->state.substate01=entry->state.substate01;next->state.countera8=entry->state.countera8;next->state.counterac=entry->state.counterac;next->state.counterb0=entry->state.counterb0;++next->state.native_revision;
            storage_=std::move(next);status_=BattlePreparationBootstrapStatus::AtDescriptors;if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain native entry allocation; source claim retained");}
    }
    bool initialize_descriptors(const SceneDescriptorBootstrapProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtDescriptors||!current(owners)||!profile.ready()){if(error)*error="ordered preparation: current descriptor call boundary/profile required";return false;}
        status_=BattlePreparationBootstrapStatus::Failed;if(!storage_->descriptors.begin_fresh(profile,error))return false;++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtPhysicalPools;if(error)error->clear();return true;
    }
    bool initialize_physical_pools(const ScenePhysicalPoolsProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtPhysicalPools||!current(owners)||!profile.ready()||!storage_->descriptors.pristine_for_physical_join()){if(error)*error="ordered preparation: current untouched physical-call boundary/profile required";return false;}
        status_=BattlePreparationBootstrapStatus::Failed;if(!storage_->physical.begin_fresh(profile,storage_->descriptors,error))return false;const auto entry=storage_->entry.lock();if(!entry||!entry->live){if(error)*error="ordered preparation: entry retired during physical initialization";return false;}entry->physical_controls=storage_->physical.control_binding();++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtActorSeed;if(error)error->clear();return true;
    }
    //Fresh962C4→0685C(0) resets selected84/next15C8, then68424 selects member0
    //for every active physical slot. Do not substitute a separately prepared
    //resource allocation's caller-selected member (which may intentionally be1).
    bool initialize_actor_slots(const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("ordered actor seed: ")+why;return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtActorSeed||!current(owners))return reject("same current physical seed boundary required");
        std::array<BattleFixedSlotInput,6> input{};std::array<u8,6> next_member{};
        for(unsigned i=0;i<6;++i){const bool active=(storage_->party.active()&(1u<<i))!=0;const auto members=storage_->party.members(i);if(!active){if(!members.empty()||storage_->party.team(i)!=-1)return reject("inconsistent inactive physical slot");continue;}if(members.empty()||members.size()>30||storage_->party.team(i)<0||storage_->party.team(i)>1)return reject("nonempty source member list required");const auto& first=members.front().fields;if(first.model==0xffff||first.variant>=6)return reject("invalid first source member");input[i]={first.model,first.variant,u8(0)};next_member[i]=u8(1%members.size());}
        status_=BattlePreparationBootstrapStatus::Failed;if(!storage_->physical.seed_fixed_actors(input,error))return false;storage_->next_member=next_member;++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtStageSeed;if(error)error->clear();return true;
    }
    bool initialize_stage(const BattleStageSeedProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtStageSeed||!current(owners)){if(error)*error="ordered stage seed: current boundary after six-slot factory required";return false;}const auto* row=profile.find(storage_->party.stage().stage);if(!row){if(error)*error="ordered stage seed: exact party stage source row required";return false;}status_=BattlePreparationBootstrapStatus::Failed;if(!storage_->physical.seed_stage(*row,error))return false;++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtSceneServices;if(error)error->clear();return true;
    }
    bool initialize_views(const BattleViewsProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtSceneServices||!current(owners)||!profile.ready()){if(error)*error="ordered views: current post-stage boundary/profile required";return false;}status_=BattlePreparationBootstrapStatus::Failed;if(!storage_->views.initialize(profile,error))return false;++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtViewServices;if(error)error->clear();return true;
    }
    bool current(const std::array<const ForceInventory*,2>& owners)const{if(!storage_||(status_!=BattlePreparationBootstrapStatus::AtDescriptors&&status_!=BattlePreparationBootstrapStatus::AtPhysicalPools&&status_!=BattlePreparationBootstrapStatus::AtActorSeed&&status_!=BattlePreparationBootstrapStatus::AtStageSeed&&status_!=BattlePreparationBootstrapStatus::AtSceneServices&&status_!=BattlePreparationBootstrapStatus::AtViewServices)||!storage_->party.current(owners))return false;const auto entry=storage_->entry.lock();if(!entry||!entry->live||!entry->prefix_claimed||!entry->branch_claimed||entry->scene45!=storage_->state.scene45||entry->state.root00!=storage_->state.root00||entry->state.substate01!=storage_->state.substate01)return false;if(status_!=BattlePreparationBootstrapStatus::AtDescriptors&&!storage_->descriptors.current())return false;return (status_==BattlePreparationBootstrapStatus::AtDescriptors||status_==BattlePreparationBootstrapStatus::AtPhysicalPools)||(storage_->physical.current()&&entry->physical_controls.lock().get()==storage_->physical.controls()&&(status_!=BattlePreparationBootstrapStatus::AtViewServices||storage_->views.ready()));}
    const BattlePreparationBootstrapState* state(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?&storage_->state:nullptr;}
    const SceneDescriptorBootstrap* descriptors(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_!=BattlePreparationBootstrapStatus::AtDescriptors?&storage_->descriptors:nullptr;}
    const ScenePhysicalPools* physical_pools(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&(status_==BattlePreparationBootstrapStatus::AtActorSeed||status_==BattlePreparationBootstrapStatus::AtStageSeed||status_==BattlePreparationBootstrapStatus::AtSceneServices||status_==BattlePreparationBootstrapStatus::AtViewServices)?&storage_->physical:nullptr;}
    const std::array<u8,6>* next_member_indices(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&(status_==BattlePreparationBootstrapStatus::AtStageSeed||status_==BattlePreparationBootstrapStatus::AtSceneServices||status_==BattlePreparationBootstrapStatus::AtViewServices)?&storage_->next_member:nullptr;}
    const BattleViews* views(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_==BattlePreparationBootstrapStatus::AtViewServices?&storage_->views:nullptr;}
    BattlePreparationBootstrapPending pending()const{BattlePreparationBootstrapPending p;switch(status_){case BattlePreparationBootstrapStatus::AtViewServices:p.view_initialization=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtSceneServices:p.stage87718=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtStageSeed:p.actor_seed536b0=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtActorSeed:p.physical88398=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtPhysicalPools:p.descriptor197854=false;break;default:break;}return p;}
    u32 next_source_address(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?(status_==BattlePreparationBootstrapStatus::AtDescriptors?0x800284f4:status_==BattlePreparationBootstrapStatus::AtPhysicalPools?0x800284f8:status_==BattlePreparationBootstrapStatus::AtActorSeed?0x80028510:status_==BattlePreparationBootstrapStatus::AtStageSeed?0x80028520:status_==BattlePreparationBootstrapStatus::AtSceneServices?0x80028530:0x8002859c):0;}
    bool widget_dispatch_authority_ready()const{return false;}BattlePreparationBootstrapStatus status()const{return status_;}
    void retire(){storage_.reset();attempted_=true;status_=BattlePreparationBootstrapStatus::Retired;} //does not clear entry fields or claims
private:struct Storage {ChallengeInitialPartyCandidate party;std::weak_ptr<challenge_preparation_entry_detail::Storage> entry;BattlePreparationBootstrapState state;SceneDescriptorBootstrap descriptors;ScenePhysicalPools physical;std::array<u8,6> next_member{};BattleViews views;};std::unique_ptr<Storage> storage_;bool attempted_=false;BattlePreparationBootstrapStatus status_=BattlePreparationBootstrapStatus::Closed;
};
} // namespace gf
