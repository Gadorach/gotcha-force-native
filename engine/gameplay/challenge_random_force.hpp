#pragma once
#include "engine/assets/challenge_random_force.hpp"
#include "engine/gameplay/challenge_force_selection.hpp"
#include "engine/gameplay/source_byte_rng.hpp"
namespace gf {
enum class ChallengeRandomOrigin {Box,Warehouse,SourceDefault};
struct ChallengeRandomMember {ForceBorg borg;ChallengeRandomOrigin origin=ChallengeRandomOrigin::Box;u16 source_slot=0xffff;};
//Random is not a stored force number. Its fallback can originate in warehouse
//or source initialization, so it must not forge a ForceBattleRoster box index.
//Canonical ForceBorg values, catalog and owner/revision binding are reused.
class ChallengeRandomRoster {
public:
    std::span<const ChallengeRandomMember> members()const{return {members_.data(),count_};}
    bool current(const ForceInventory& owner)const{return count_&&owner.ready()&&scope_.lock()&&scope_.lock()==owner.instance_binding().lock()&&revision_==owner.revision();}
    const std::array<u8,41>& name()const{return name_;}u8 port()const{return port_;}s32 energy()const{return energy_;}
    u32 source_spent()const{return spent_;}u32 nominal_cost()const{return nominal_;}bool fallback()const{return fallback_;}
private:
    friend class ChallengeRandomAssembly;
    std::array<ChallengeRandomMember,kForceSlots> members_{};std::array<u8,41> name_{};std::weak_ptr<const ForceInventoryScope> scope_;u64 revision_=0;unsigned count_=0;u32 spent_=0,nominal_=0;s32 energy_=0;u8 port_=0;bool fallback_=false;
};
struct ChallengeRandomChoice {u16 candidates=0,ordinal=0,box_slot=0;u8 random_byte=0;};
enum class ChallengeRandomStatus {Closed,Ready,Taken,RejectedEdited,Failed};
//1910B4 RANDOM branch: enumerate fitting UNUSED box slots in increasing order,
//choose (source RNG low byte % count), stop after30 picks or empty candidate set.
//Caller provides explicit RNG state. Success publishes the NEXT state once;
//nothing here seeds/commits a live global RNG or acknowledges battle entry.
class ChallengeRandomAssembly {
public:
    ChallengeRandomAssembly()=default;ChallengeRandomAssembly(const ChallengeRandomAssembly&)=delete;ChallengeRandomAssembly& operator=(const ChallengeRandomAssembly&)=delete;
    bool begin(const ChallengeForceRequest& request,const ForceInventory& owner,const ChallengeRandomForceProfile& profile,SourceByteRngState state,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("Challenge Random Force: ")+why;return false;};
        if(status_!=ChallengeRandomStatus::Closed||request.kind()!=ChallengeForceRequestKind::Random||!request.current(owner)||!profile.for_catalog(*owner.catalog()))return reject("fresh current Random request/profile required");
        const auto scope=request.scope_.lock();if(!scope||scope->claimed)return reject("request already claimed");
        scope->claimed=true;request_=request;next_state_=state;status_=ChallengeRandomStatus::Failed;
        roster_.scope_=owner.instance_binding();roster_.revision_=owner.revision();roster_.energy_=request.energy();roster_.port_=request.port();roster_.name_=profile.name;
        std::array<bool,kBoxBorgs> picked{};std::array<u16,kBoxBorgs> candidates{};
        auto cost=[&](const ForceBorg& b){return u32(owner.catalog()->find(u16(b.model))->cost[b.variant]);};
        //Authored catalog cost, NOT a native tool cost override. Edited slots
        //are not silently filtered/rerolled: if chosen, native admission refuses.
        for(unsigned member=0;member<kForceSlots;++member){
            unsigned count=0;
            for(unsigned slot=0;slot<kBoxBorgs;++slot){const auto& b=owner.state()->box[slot].value;if(b.present()&&!picked[slot]&&roster_.spent_+cost(b)<=u32(request.energy()))candidates[count++]=u16(slot);}
            if(!count)break;
            const u8 byte=source_byte_rng_step(next_state_);const unsigned ordinal=byte%count;const auto slot=candidates[ordinal];const auto& b=owner.state()->box[slot].value;
            trace_[trace_count_++]={u16(count),u16(ordinal),slot,byte};
            if(b.edits.modified){status_=ChallengeRandomStatus::RejectedEdited;return reject("chosen Borg is natively modified; no reroll/publication");}
            picked[slot]=true;roster_.members_[roster_.count_++]={b,ChallengeRandomOrigin::Box,slot};roster_.spent_+=cost(b);roster_.nominal_+=cost(b);
        }
        if(!roster_.count_){
            unsigned selected=kWarehouseBorgs;u8 variant=6;
            for(unsigned slot=0;slot<kWarehouseBorgs;++slot){const auto& b=owner.state()->warehouse[slot];
                if(b.model==s16(profile.special_model)&&!b.variant){selected=slot;break;}
                if(b.model==s16(profile.preferred_model)&&b.variant<variant){selected=slot;variant=b.variant;}
            }
            ChallengeRandomMember fallback;
            if(selected<kWarehouseBorgs){fallback={owner.state()->warehouse[selected],ChallengeRandomOrigin::Warehouse,u16(selected)};}
            else{fallback.borg.model=s16(profile.default_model);fallback.origin=ChallengeRandomOrigin::SourceDefault;fallback.source_slot=0xffff;}
            if(fallback.borg.edits.modified){status_=ChallengeRandomStatus::RejectedEdited;return reject("warehouse fallback is natively modified; no substitution/publication");}
            roster_.members_[0]=fallback;roster_.count_=1;roster_.fallback_=true;roster_.nominal_=cost(fallback.borg);
            //Source fallback ignores energy and does not add to the pick-loop's
            //spent counter. Nominal cost is exposed separately, never clamped.
        }
        status_=ChallengeRandomStatus::Ready;if(error)error->clear();return true;
    }
    bool take(const ForceInventory& owner,ChallengeRandomRoster* out,SourceByteRngState* next){
        if(out)*out={};
        if(!out||!next)return false;
        if(status_!=ChallengeRandomStatus::Ready||!request_.current(owner)||!roster_.current(owner)){if(status_==ChallengeRandomStatus::Ready)status_=ChallengeRandomStatus::Failed;return false;}
        *out=roster_;*next=next_state_;status_=ChallengeRandomStatus::Taken;const auto scope=request_.scope_.lock();if(scope)scope->live=false;return true;
    }
    ChallengeRandomStatus status()const{return status_;}std::span<const ChallengeRandomChoice> trace()const{return {trace_.data(),trace_count_};}
private:
    ChallengeForceRequest request_;ChallengeRandomRoster roster_;SourceByteRngState next_state_;std::array<ChallengeRandomChoice,kForceSlots> trace_{};unsigned trace_count_=0;ChallengeRandomStatus status_=ChallengeRandomStatus::Closed;
};
} // namespace gf
