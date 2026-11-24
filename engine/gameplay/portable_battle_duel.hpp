#pragma once

#include "engine/gameplay/native_battle_mvp.hpp"
#include "engine/gameplay/portable_battle_request.hpp"

namespace gf {

struct PortableDuelInput {
    GamePadState movement;
    BorgBattleInputs attack;
};

// Reuse the native duel's movement and jump decisions, but feed its primary
// attack into the authored action scheduler as explicit held/edge samples.
// Unlike the legacy weapon path, an authored Press cannot retrigger from the
// same held bit. The v1 bridge gives it a deterministic release/repress cycle.
inline PortableDuelInput portable_duel_enemy_input(const NativeBattleMvp& world){
    auto pad=world.duel_ai_pad();
    BorgBattleInputs attack;
    if(pad.a){
        constexpr u32 period=30,hold=18;
        const auto phase=world.tick()%period;
        if(phase<hold)attack.held=BorgBattleInputs::b;
        if(phase==0)attack.pressed=BorgBattleInputs::b;
        if(phase==hold)attack.released=BorgBattleInputs::b;
    }
    pad.a=false;
    return {pad,attack};
}

enum class PortableDuelEnd {Fighting,Win,Loss,Draw,Timeout,Cancelled};

inline const char* portable_duel_end_name(PortableDuelEnd end){
    switch(end){
    case PortableDuelEnd::Fighting:return "fighting";
    case PortableDuelEnd::Win:return "win";
    case PortableDuelEnd::Loss:return "loss";
    case PortableDuelEnd::Draw:return "draw";
    case PortableDuelEnd::Timeout:return "timeout";
    case PortableDuelEnd::Cancelled:return "cancelled";
    }
    return "invalid";
}

// Read only after a simulated tick. KO on the last permitted tick takes
// precedence over expiry; draw/timeout/end-of-host never fabricate a win.
inline PortableDuelEnd portable_duel_end(const NativeBattleMvp& world,u32 tick_limit){
    if(!world.ready()||!world.tick())return PortableDuelEnd::Fighting;
    switch(world.result()){
    case BattleMvpResult::GRedWins:return PortableDuelEnd::Win; // native slot 0
    case BattleMvpResult::SasukeWins:return PortableDuelEnd::Loss; // native slot 1
    case BattleMvpResult::Draw:return PortableDuelEnd::Draw;
    case BattleMvpResult::Fighting:
        return tick_limit&&world.tick()>=tick_limit?PortableDuelEnd::Timeout:PortableDuelEnd::Fighting;
    }
    return PortableDuelEnd::Cancelled;
}

inline std::optional<PortableBattleOutcome> portable_duel_campaign_outcome(PortableDuelEnd end){
    if(end==PortableDuelEnd::Win)return PortableBattleOutcome::Win;
    if(end==PortableDuelEnd::Loss)return PortableBattleOutcome::Loss;
    return {};
}

} // namespace gf
