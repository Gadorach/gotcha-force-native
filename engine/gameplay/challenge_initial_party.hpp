#pragma once
#include "engine/assets/challenge_difficulty.hpp"
#include "engine/gameplay/challenge_setup_choice.hpp"
#include "engine/gameplay/challenge_random_force.hpp"
#include "engine/gameplay/challenge_stage_rng.hpp"
#include "engine/gameplay/challenge_ai_force.hpp"
#include "engine/gameplay/source_byte_rng_owner.hpp"
namespace gf {
//Known source roster fields only. Inventory freshness/reserved/tool fields are
//not actor metadata, and no general28B actor-record or memory-layout claim is made.
struct ChallengeRosterFields {u16 model=0;u8 variant=0,level=0;u32 experience=0,acquired=0;bool operator==(const ChallengeRosterFields&)const=default;};
inline bool challenge_roster_fields(const ForceCatalog& catalog,const ForceBorg& borg,ChallengeRosterFields* out){
    if(out)*out={};
    if(!out||!catalog.find(u16(borg.model))||!borg.present()||borg.variant>=6||borg.level>=10||borg.edits!=BorgEdits{})return false;
    *out={u16(borg.model),borg.variant,borg.level,borg.experience,borg.acquired};return true;
}
enum class ChallengePartyOrigin {StoredBox,RandomBox,RandomWarehouse,RandomSourceDefault,InitialAi};
struct ChallengePartyMember {ChallengeRosterFields fields;ChallengePartyOrigin origin=ChallengePartyOrigin::StoredBox;u16 source_slot=0xffff;u8 force_slot=0xff;};
class ChallengeInitialPartyAssembly;
class ChallengeInitialPartyCandidate {
public:
    bool ready()const{return ready_;}u8 humans()const{return human_mask_;}u8 active()const{return active_mask_;}u8 players()const{return count_;}
    const std::array<s8,4>& controller_order()const{return ports_;}
    s8 team(unsigned slot)const{return slot<6?teams_[slot]:s8(-1);}
    std::span<const ChallengePartyMember> members(unsigned slot)const{return slot<6?std::span<const ChallengePartyMember>{members_[slot].data(),counts_[slot]}:std::span<const ChallengePartyMember>{};}
    const std::array<u8,41>* name(unsigned slot)const{return slot<6?&names_[slot]:nullptr;}
    const ChallengeStageRngPlan& stage()const{return stage_;}const ChallengeAiPhasePlan& ai()const{return ai_;}
    u8 difficulty()const{return ready_?difficulty_:255;}
    const ChallengePreparationEntryState* preparation_entry_state(const std::array<const ForceInventory*,2>& owners)const{if(!current(owners))return nullptr;const auto state=preparation_entry_.lock();return state&&state->live?&state->state:nullptr;}
    SourceByteRngState before()const{return before_;}SourceByteRngState after()const{return after_;}u32 draws()const{return draws_;}
    SourceByteRngState ai_before()const{return ai_before_;}u64 intervening_draws()const{return intervening_draws_;}
    bool current(const std::array<const ForceInventory*,2>& owners)const{return ready_&&bound(owners);}
private:
    friend class ChallengeInitialPartyAssembly;
    friend class BattlePreparationContext;
    friend class BattlePreparationBootstrap;
    friend class ChallengeFinalSetup;
    bool bound(const std::array<const ForceInventory*,2>& owners)const{for(unsigned i=0;i<count_;++i)if(!owners[i]||!owners[i]->ready()||owners[i]->instance_binding().lock()!=owners_[i].lock()||!owners_[i].lock()||owners[i]->revision()!=revisions_[i])return false;return true;}
    std::array<std::array<ChallengePartyMember,30>,6> members_{};std::array<u8,6> counts_{};std::array<std::array<u8,41>,6> names_{};
    std::array<s8,6> teams_{{-1,-1,-1,-1,-1,-1}};std::array<s8,4> ports_{{-1,-1,-1,-1}};
    std::array<std::weak_ptr<const ForceInventoryScope>,2> owners_;std::array<u64,2> revisions_{};
    ChallengeStageRngPlan stage_;ChallengeAiPhasePlan ai_;SourceByteRngState before_,after_;u32 draws_=0;u8 human_mask_=0,active_mask_=0,count_=0;bool ready_=false;
    SourceByteRngState ai_before_;u64 intervening_draws_=0;
    u8 difficulty_=255;
    std::weak_ptr<challenge_preparation_entry_detail::Storage> preparation_entry_;
};
enum class ChallengeInitialPartyStatus {Closed,Ready,Taken,Failed,StageReady,AwaitAi};
//Fresh first-battle candidate ONLY:95F2C/95FBC clear context/roster storage and
//previous stage=-1;962C4 uses wave0. AI's unwritten XP/acquired fields are zero
//in this explicitly fresh representation. NOT a later-wave or actor-start path.
//Selection batch claims are one-shot even on failure. Only complete publication
//and the serialized RNG commit are atomic; input request claims are not rolled back.
class ChallengeInitialPartyAssembly {
public:
    bool begin(const std::array<ChallengeSetupChoice,2>& choices,unsigned count,const std::array<const ForceInventory*,2>& owners,const ChallengeDifficultyProfile& difficulty,unsigned selected,const ChallengeRandomForceProfile& random,const ChallengeStageRngProfile& stages,const ChallengeAiForceProfile& ai,SourceByteRngOwner& rng,std::string* error){
        return begin_impl(choices,count,owners,difficulty,selected,random,stages,ai,rng,error,false);
    }
    //Explicit source-phase mode: publish/commit human+stage first, then compute
    //AI from the actual current stream AFTER cover/resource completion. A later
    //failure retains that earlier commit; it is never rolled back or replayed.
    bool prepare_stage(const std::array<ChallengeSetupChoice,2>& choices,unsigned count,const std::array<const ForceInventory*,2>& owners,const ChallengeDifficultyProfile& difficulty,unsigned selected,const ChallengeRandomForceProfile& random,const ChallengeStageRngProfile& stages,const ChallengeAiForceProfile& ai,SourceByteRngOwner& rng,std::string* error){
        return begin_impl(choices,count,owners,difficulty,selected,random,stages,ai,rng,error,true);
    }
private:
    bool begin_impl(const std::array<ChallengeSetupChoice,2>& choices,unsigned count,const std::array<const ForceInventory*,2>& owners,const ChallengeDifficultyProfile& difficulty,unsigned selected,const ChallengeRandomForceProfile& random,const ChallengeStageRngProfile& stages,const ChallengeAiForceProfile& ai,SourceByteRngOwner& rng,std::string* error,bool defer_ai){
        auto fail=[&](const char* why){status_=ChallengeInitialPartyStatus::Failed;candidate_={};if(error)*error=std::string("Challenge initial party: ")+why;return false;};
        if(status_!=ChallengeInitialPartyStatus::Closed)return false;
        if(count<1||count>2||!difficulty.valid()||selected>=3||!rng.ready()||!stages.valid()||!owners[0]||!owners[0]->ready())return fail("fresh selection/profile/explicit RNG required");
        const auto batch=choices[0].batch_.lock();if(!batch||!batch->live||!batch->ready||batch->claimed||batch->count!=count)return fail("current completed unclaimed selection batch required");
        const auto& catalog=*owners[0]->catalog();if(!random.for_catalog(catalog)||!ai.for_catalog(catalog)||!ai.wave(selected,0))return fail("catalog-bound profiles required");
        for(unsigned i=0;i<count;++i)if(!owners[i]||!choices[i].current(*owners[i])||choices[i].batch_.lock()!=batch||choices[i].player()!=i||choices[i].port()>=4||choices[i].energy()!=difficulty.energy[selected]||owners[i]->catalog()->fingerprint()!=catalog.fingerprint()||(i&&choices[i].port()==choices[0].port())||(choices[i].kind()!=ChallengeSetupChoiceKind::Stored&&choices[i].kind()!=ChallengeSetupChoiceKind::RandomRequest))return fail("selected player/port/energy/owner mismatch");
        batch->claimed=true;batch_=batch;snapshot_=rng.snapshot();ChallengeInitialPartyCandidate next;next.preparation_entry_=batch->preparation_entry;next.before_=snapshot_.state();next.after_=next.before_;next.count_=u8(count);next.difficulty_=u8(selected);
        for(unsigned i=0;i<count;++i){next.ports_[i]=s8(choices[i].port());next.human_mask_|=u8(1u<<choices[i].port());next.owners_[i]=owners[i]->instance_binding();next.revisions_[i]=owners[i]->revision();}
        //Assembly source slot order is distinct from selection ordinal/port map.
        for(unsigned slot=0;slot<4;++slot)for(unsigned i=0;i<count;++i){if(choices[i].port()!=slot)continue;
            if(choices[i].kind()==ChallengeSetupChoiceKind::Stored){const auto& roster=choices[i].stored();std::copy_n(roster.name().begin(),40,next.names_[slot].begin());for(const auto& member:roster.members()){auto& dest=next.members_[slot][next.counts_[slot]++];if(!challenge_roster_fields(catalog,member.borg,&dest.fields))return fail("stored metadata/modified Borg refusal");dest.origin=ChallengePartyOrigin::StoredBox;dest.source_slot=member.box_slot;dest.force_slot=member.force_slot;}}
            else{ChallengeRandomAssembly assembly;ChallengeRandomRoster roster;SourceByteRngState after;if(!assembly.begin(choices[i].random(),*owners[i],random,next.after_,error)||!assembly.take(*owners[i],&roster,&after))return fail("Random request refused; no reroll or partial publication");next.draws_+=u32(assembly.trace().size());next.after_=after;std::copy(roster.name().begin(),std::find(roster.name().begin(),roster.name().end(),u8(0)),next.names_[slot].begin());for(const auto& member:roster.members()){auto& dest=next.members_[slot][next.counts_[slot]++];if(!challenge_roster_fields(catalog,member.borg,&dest.fields))return fail("Random metadata refusal");dest.origin=member.origin==ChallengeRandomOrigin::Box?ChallengePartyOrigin::RandomBox:member.origin==ChallengeRandomOrigin::Warehouse?ChallengePartyOrigin::RandomWarehouse:ChallengePartyOrigin::RandomSourceDefault;dest.source_slot=member.source_slot;}}
            if(!next.counts_[slot])return fail("empty human roster");
        }
        if(!challenge_stage_rng_plan(stages,next.after_,-1,&next.stage_))return fail("stage candidate");
        next.after_=next.stage_.next;next.draws_+=next.stage_.draws;
        if(defer_ai){deferred_ai_=ai;selected_=selected;candidate_=std::move(next);status_=ChallengeInitialPartyStatus::StageReady;if(error)error->clear();return true;}
        if(const auto why=finish_ai(next,ai,catalog,selected,error))return fail(why);
        next.ready_=true;candidate_=std::move(next);status_=ChallengeInitialPartyStatus::Ready;if(error)error->clear();return true;
    }
public:
    bool take_stage(const std::array<const ForceInventory*,2>& owners,SourceByteRngOwner& rng,ChallengeStageRngPlan* out){
        if(out)*out={};
        if(!out||status_!=ChallengeInitialPartyStatus::StageReady)return false;
        const auto batch=batch_.lock();if(!batch||!batch->live||!candidate_.bound(owners)||!rng.commit(snapshot_,candidate_.after_,candidate_.draws_)){status_=ChallengeInitialPartyStatus::Failed;candidate_={};return false;}
        *out=candidate_.stage_;stage_committed_=true;stage_draws_at_commit_=rng.draws();snapshot_=rng.snapshot();status_=ChallengeInitialPartyStatus::AwaitAi;return true;
    }
    bool complete_ai(const std::array<const ForceInventory*,2>& owners,SourceByteRngOwner& rng,std::string* error){
        if(status_!=ChallengeInitialPartyStatus::AwaitAi)return false;
        const auto batch=batch_.lock();auto fail=[&](const char* why){status_=ChallengeInitialPartyStatus::Failed;candidate_={};if(error)*error=std::string("Challenge delayed AI: ")+why;return false;};
        if(!batch||!batch->live||!candidate_.bound(owners)||!rng.same_owner(snapshot_))return fail("stage owner/inventory/selection expired");
        snapshot_=rng.snapshot();candidate_.after_=snapshot_.state();candidate_.intervening_draws_=rng.draws()-stage_draws_at_commit_;
        if(const auto why=finish_ai(candidate_,deferred_ai_,*owners[0]->catalog(),selected_,error))return fail(why);
        candidate_.ready_=true;status_=ChallengeInitialPartyStatus::Ready;if(error)error->clear();return true;
    }
    bool take(const std::array<const ForceInventory*,2>& owners,SourceByteRngOwner& rng,ChallengeInitialPartyCandidate* out){
        if(out)*out={};
        if(!out||status_!=ChallengeInitialPartyStatus::Ready)return false;
        const auto batch=batch_.lock();if(!batch||!batch->live||!batch->claimed||!candidate_.current(owners)||!rng.commit(snapshot_,candidate_.after_,stage_committed_?candidate_.ai_.draws:candidate_.draws_)){status_=ChallengeInitialPartyStatus::Failed;candidate_={};return false;}
        *out=candidate_;status_=ChallengeInitialPartyStatus::Taken;return true;
    }
    ChallengeInitialPartyStatus status()const{return status_;}
    const ChallengeStageRngPlan* pending_stage()const{return status_==ChallengeInitialPartyStatus::StageReady||status_==ChallengeInitialPartyStatus::AwaitAi?&candidate_.stage_:nullptr;}
private:
    static const char* finish_ai(ChallengeInitialPartyCandidate& next,const ChallengeAiForceProfile& ai,const ForceCatalog& catalog,unsigned selected,std::string* error){
        next.ai_before_=next.after_;
        if(!challenge_ai_force_plan(ai,catalog,selected,0,next.human_mask_,next.after_,&next.ai_,error))return "AI field candidate";
        next.after_=next.ai_.next;next.draws_+=next.ai_.draws;next.active_mask_=next.ai_.active_mask;
        for(unsigned slot=0;slot<6;++slot){next.teams_[slot]=next.ai_.slots[slot].team;if(!(next.human_mask_&(1u<<slot))&&next.teams_[slot]>=0){for(unsigned j=0;j<next.ai_.slots[slot].count;++j){const auto& fields=next.ai_.slots[slot].members[j];auto& dest=next.members_[slot][next.counts_[slot]++];dest.fields={fields.model,fields.variant,fields.level,0,0};dest.origin=ChallengePartyOrigin::InitialAi;}}if(next.teams_[slot]>=0&&!next.counts_[slot])return "empty active roster cannot reach next-member modulo consumer";}
        return nullptr;
    }
    std::weak_ptr<challenge_choice_detail::Batch> batch_;SourceByteRngSnapshot snapshot_;ChallengeInitialPartyCandidate candidate_;ChallengeInitialPartyStatus status_=ChallengeInitialPartyStatus::Closed;
    ChallengeAiForceProfile deferred_ai_;unsigned selected_=0;u64 stage_draws_at_commit_=0;bool stage_committed_=false;
};
} // namespace gf
