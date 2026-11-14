#pragma once
#include <exception>
#include "engine/assets/battle_character_resources.hpp"
#include "engine/gameplay/challenge_initial_party.hpp"
#include "engine/core/backends.hpp"
namespace gf {
enum class ChallengeCharacterResourceStatus {Closed,Loading,DescriptorAllocated,Failed,Retired};
namespace challenge_character_resource_detail {
struct Batch {bool live=true;ChallengeInitialPartyCandidate party;std::array<std::shared_ptr<const BattleCharacterResource>,6> resources;std::array<u8,6> selected_member_indices{255,255,255,255,255,255};mutable std::array<bool,6> construction_claimed{};mutable bool battle_roster_claimed=false;};
}
class ChallengeCharacterResources;
class ChallengeCharacterDescriptorReceipt {
public:
    std::shared_ptr<const BattleCharacterResource> data(const std::array<const ForceInventory*,2>& owners)const{const auto b=batch_.lock();return b&&b->live&&b->party.current(owners)&&slot_<6?b->resources[slot_]:nullptr;}
    bool valid(const std::array<const ForceInventory*,2>& owners)const{return bool(data(owners));}
private:friend class ChallengeCharacterResources;friend class BattleActorConstruction;friend class ChallengeBattleRoster;std::weak_ptr<const challenge_character_resource_detail::Batch> batch_;u8 slot_=255;
};
//Explicit resource preparation for selected members of a genuine current party.
//Member indices are supplied, NOT assumed to be the original first-member policy.
//One fresh read per occupied slot; native ALL-batch publication is stronger than
//source per-slot callback timing and is not an original asynchronous ACK.
class ChallengeCharacterResources {
public:
    ChallengeCharacterResources()=default;ChallengeCharacterResources(const ChallengeCharacterResources&)=delete;ChallengeCharacterResources& operator=(const ChallengeCharacterResources&)=delete;
    ChallengeCharacterResources(ChallengeCharacterResources&&)=default;ChallengeCharacterResources& operator=(ChallengeCharacterResources&&)=default;
    ~ChallengeCharacterResources(){retire();}
    bool begin(const ChallengeInitialPartyCandidate& party,const std::array<const ForceInventory*,2>& owners,const std::array<u8,6>& selected,const BattleCharacterResourceProfile& profile,IFileSystem& files,const std::string& directory,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("Challenge character resources: ")+why;return false;};
        if(status_!=ChallengeCharacterResourceStatus::Closed)return reject("fresh one-shot owner required; no retry");
        status_=ChallengeCharacterResourceStatus::Loading;auto next=std::make_shared<challenge_character_resource_detail::Batch>();next->party=party;batch_=next;const auto indices=selected;
        auto fail=[&](const char* why){next->live=false;next->resources={};if(status_!=ChallengeCharacterResourceStatus::Retired)status_=ChallengeCharacterResourceStatus::Failed;return reject(why);};
        if(!next->party.current(owners)||!owners[0]||!profile.for_catalog(*owners[0]->catalog())||directory.empty())return fail("current party/catalog/profile/native directory required");
        std::array<BattleCharacterResourceRow,6> rows{};std::array<bool,6> active{};std::array<u8,6> variants{};
        for(unsigned slot=0;slot<6;++slot){active[slot]=(next->party.active()&(1u<<slot))!=0;if(!active[slot]){if(indices[slot]!=255)return fail("inactive member index must be absent");continue;}const auto members=next->party.members(slot);if(indices[slot]>=members.size())return fail("selected member outside current roster");const auto& fields=members[indices[slot]].fields;const auto* row=profile.find(fields.model);if(!row||fields.variant>=6)return fail("selected model/variant not bound by profile");rows[slot]=*row;variants[slot]=fields.variant;}
        next->selected_member_indices=indices;
        try{for(unsigned slot=0;slot<6;++slot){if(!active[slot])continue;const auto& row=rows[slot];const auto path=directory+"/"+std::to_string(row.member)+".gfnb";u64 size=0;std::vector<u8> bytes;
                if(!files.file_size(path,&size)||size!=row.bytes||!files.read(path,bytes)||bytes.size()!=row.bytes)return fail("missing/short native resource; request not replayed");
                if(!next->live||batch_!=next||!next->party.current(owners))return fail("party/owner retired or changed during read");
                auto resource=std::make_shared<BattleCharacterResource>();if(!resource->load(row,variants[slot],{bytes.data(),bytes.size()},error))return fail("native byte identity/role refusal");next->resources[slot]=std::move(resource);
            }}catch(const std::exception&){return fail("uncertain native read/allocation; request not replayed");}
        if(!next->live||batch_!=next||!next->party.current(owners))return fail("party/owner retired before publication");
        status_=ChallengeCharacterResourceStatus::DescriptorAllocated;if(error)error->clear();return true;
    }
    ChallengeCharacterDescriptorReceipt receipt(unsigned slot)const{ChallengeCharacterDescriptorReceipt r;if(status_==ChallengeCharacterResourceStatus::DescriptorAllocated&&slot<6&&batch_&&batch_->live&&batch_->resources[slot]){r.batch_=batch_;r.slot_=u8(slot);}return r;}
    ChallengeCharacterResourceStatus status()const{return status_;}
    void retire(){if(batch_)batch_->live=false;batch_.reset();status_=ChallengeCharacterResourceStatus::Retired;}
private:std::shared_ptr<challenge_character_resource_detail::Batch> batch_;ChallengeCharacterResourceStatus status_=ChallengeCharacterResourceStatus::Closed;
};
} // namespace gf
