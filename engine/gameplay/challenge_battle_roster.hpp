#pragma once
#include "engine/gameplay/battle_actor_member.hpp"
#include "engine/gameplay/challenge_character_resources.hpp"
#include "engine/assets/challenge_player_defaults.hpp"
namespace gf {
namespace challenge_battle_roster_detail {
struct Storage {
    bool live=true;u64 revision=1;
    std::weak_ptr<const challenge_character_resource_detail::Batch> allocation;
    std::array<std::vector<BattleRosterMember>,6> members;
    std::array<u8,6> selected{255,255,255,255,255,255};
    std::array<bool,6> member_initialized{};
    std::optional<std::array<ChallengePlayerDefaults,6>> player_defaults;
    bool preparation_context_claimed=false;
    std::optional<u8> scene45;
    bool current(const std::array<const ForceInventory*,2>& owners)const{const auto b=allocation.lock();return live&&b&&b->current(owners);}
};
}
enum class ChallengeBattleRosterStatus {Closed,Ready,Failed,Retired};
//Mutable first-battle context, distinct from canonical inventory and immutable
//party selection. 962C4 -> 0685C(0) clears populated row flags22/24. Challenge
//common50=1 bypasses the Story-only context1F/word8 branch in68344. No guessed
//zero optional shorts/word8, full28B codec, respawn or global pool authority.
class ChallengeBattleRoster {
public:
    ChallengeBattleRoster()=default;ChallengeBattleRoster(const ChallengeBattleRoster&)=delete;ChallengeBattleRoster& operator=(const ChallengeBattleRoster&)=delete;
    ChallengeBattleRoster(ChallengeBattleRoster&&)=default;ChallengeBattleRoster& operator=(ChallengeBattleRoster&&)=default;
    ~ChallengeBattleRoster(){retire();}
    bool begin(const ChallengeCharacterDescriptorReceipt& allocation,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("Challenge battle roster: ")+why;return false;};
        if(status_!=ChallengeBattleRosterStatus::Closed)return reject("fresh one-shot owner required");
        status_=ChallengeBattleRosterStatus::Failed;const auto batch=allocation.batch_.lock();
        if(!batch||!allocation.valid(owners)||!batch->party.current(owners)||batch->battle_roster_claimed)return reject("current unclaimed allocation required");
        batch->battle_roster_claimed=true; //serialized; never reset after retire/uncertain work
        try{auto next=std::make_shared<challenge_battle_roster_detail::Storage>();next->allocation=batch;next->selected=batch->selected_member_indices;
            for(unsigned slot=0;slot<6;++slot){const auto source=batch->party.members(slot);if(!(batch->party.active()&(1u<<slot))){if(next->selected[slot]!=255)return reject("inactive selected member");continue;}
                if(source.empty()||source.size()>30||next->selected[slot]>=source.size())return reject("captured member index outside populated roster");
                for(const auto& member:source){BattleRosterMember row;row.selected=member;row.flag22=0;row.flag24=0;next->members[slot].push_back(std::move(row));}
            }
            if(!next->current(owners))return reject("allocation retired before publication");
            storage_=std::move(next);status_=ChallengeBattleRosterStatus::Ready;if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain allocation; no replay");}
    }
    bool current(const std::array<const ForceInventory*,2>& owners)const{return status_==ChallengeBattleRosterStatus::Ready&&storage_&&storage_->current(owners);}
    //Explicit first-battle context continuation. Copies all six source writes,
    //including inactive slots, from the party's genuine selected difficulty/team.
    //Old begin alone intentionally supplies no scalar-default authority.
    bool initialize_player_defaults(const ChallengePlayerDefaultsProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("Challenge battle roster defaults: ")+why;return false;};
        if(!current(owners)||storage_->player_defaults||!profile.ready())return reject("current one-shot context/profile required");
        const auto batch=storage_->allocation.lock();if(!batch||batch->party.difficulty()>=3)return reject("captured party difficulty required");
        std::array<ChallengePlayerDefaults,6> values;for(unsigned slot=0;slot<6;++slot)if(!profile.select(batch->party.difficulty(),batch->party.team(slot),&values[slot]))return reject("signed team/difficulty domain");
        if(!current(owners))return reject("allocation retired before publication");
        storage_->player_defaults=values;++storage_->revision;if(error)error->clear();return true;
    }
    const ChallengePlayerDefaults* player_defaults(unsigned slot,const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&slot<6&&storage_->player_defaults?&(*storage_->player_defaults)[slot]:nullptr;}
    //Inspection only; neither copies nor equal field bytes authorize actor writes.
    std::span<const BattleRosterMember> members(unsigned slot,const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&slot<6?std::span<const BattleRosterMember>{storage_->members[slot]}:std::span<const BattleRosterMember>{};}
    u8 selected(unsigned slot,const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&slot<6?storage_->selected[slot]:255;}
    u64 revision(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?storage_->revision:0;}
    std::optional<u8> preparation_scene45(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?storage_->scene45:std::nullopt;}
    ChallengeBattleRosterStatus status()const{return status_;}
    void retire(){if(storage_)storage_->live=false;storage_.reset();status_=ChallengeBattleRosterStatus::Retired;}
private:friend class BattleActorConstruction;friend class BattlePreparationContext;std::shared_ptr<challenge_battle_roster_detail::Storage> storage_;ChallengeBattleRosterStatus status_=ChallengeBattleRosterStatus::Closed;
};
} // namespace gf
