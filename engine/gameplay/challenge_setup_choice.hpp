#pragma once
#include "engine/gameplay/challenge_force_selection.hpp"
#include "engine/gameplay/challenge_preparation_entry.hpp"
namespace gf {
class ChallengeSetupPresentation;class ChallengeInitialPartyAssembly;
namespace challenge_choice_detail {struct Batch {bool live=true,ready=false,claimed=false;u8 count=0;std::weak_ptr<challenge_preparation_entry_detail::Storage> preparation_entry;};}
enum class ChallengeSetupChoiceKind {None,Stored,RandomRequest};
//A selected player's immutable candidate handoff, NOT a battle-ready receipt.
//Random retains its actual weak request; no RNG is seeded/consumed/committed here.
class ChallengeSetupChoice {
public:
    ChallengeSetupChoiceKind kind()const{return kind_;}u8 player()const{return player_;}u8 port()const{return port_;}
    s32 energy()const{return energy_;}
    const ForceBattleRoster& stored()const{return stored_;}const ChallengeForceRequest& random()const{return random_;}
    bool current(const ForceInventory& owner)const{return kind_==ChallengeSetupChoiceKind::Stored?owner.current(stored_):kind_==ChallengeSetupChoiceKind::RandomRequest&&random_.current(owner);}
private:
    friend class ChallengeSetupPresentation;friend class ChallengeInitialPartyAssembly;
    ChallengeSetupChoiceKind kind_=ChallengeSetupChoiceKind::None;u8 player_=0,port_=0;ForceBattleRoster stored_;ChallengeForceRequest random_;
    std::weak_ptr<challenge_choice_detail::Batch> batch_;s32 energy_=0;
};
} // namespace gf
