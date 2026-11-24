#pragma once

#include "engine/assets/portable_challenge.hpp"
#include "engine/gameplay/portable_battle_request.hpp"

namespace gf {

// Story can reuse the same immutable encounter without starting a Challenge
// run. The caller chooses only a saved Force selector, never its contents.
inline std::optional<PortableBattleRequest> portable_challenge_battle_request(const PortableChallengeManifest& manifest,
                                                                             std::string_view encounter_id,
                                                                             PortableBattleSavedForce selected_force,
                                                                             PortableBattleUse use=PortableBattleUse::Campaign){
    const auto* encounter=manifest.find(encounter_id);
    if(!encounter)return std::nullopt;
    return PortableBattleRequest{manifest.id,encounter->id,encounter->stage,{encounter->player_spawn},{encounter->enemy_spawn},
        selected_force,encounter->enemies,encounter->ai,encounter->rules,encounter->tick_limit,use};
}

// An engine-owned, in-memory progression. The manifest and selected Force
// selector are pinned; the trusted resolver verifies the current save/copies
// at each prepare and acceptance. No save interfaces are used here.
class PortableChallengeRun {
public:
    // Take a successfully parsed manifest (e.g. a discovered catalogue entry).
    PortableChallengeRun(PortableChallengeManifest manifest,PortableBattleSavedForce selected_force):
        manifest_(std::move(manifest)),selected_force_(selected_force){}

    const PortableChallengeEncounter* current()const{
        return finished()?nullptr:&manifest_.encounters[index_];
    }
    std::size_t completed_count()const{return index_;}
    bool finished()const{return index_==manifest_.encounters.size();}

    PortableBattleCheck prepare(const PortableBattleResolver& resolver){
        const auto* encounter=current();
        if(!encounter)return {PortableBattleStatus::Denied,"no remaining Challenge encounter"};
        return session_.prepare(*portable_challenge_battle_request(manifest_,encounter->id,selected_force_),resolver);
    }
    PortableBattleCheck accept(const PortableBattleResolver& resolver,PortableBattleReceipt* out){
        return session_.accept(resolver,out);
    }
    bool complete(const PortableBattleReceipt& receipt,PortableBattleOutcome outcome){
        if(!session_.complete(receipt,outcome))return false;
        if(outcome==PortableBattleOutcome::Win)++index_;
        return true;
    }
    void cancel(){session_.cancel();}
    bool cancel(const PortableBattleReceipt& receipt){return session_.cancel(receipt);}

private:
    const PortableChallengeManifest manifest_;
    const PortableBattleSavedForce selected_force_;
    std::size_t index_=0;
    PortableBattleSession session_;
};

} // namespace gf
