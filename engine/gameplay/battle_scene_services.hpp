#pragma once
#include "engine/gameplay/challenge_battle_spawn.hpp"
#include "engine/gameplay/sound_control_owner.hpp"
namespace gf {
//Represented source state of52990/529DC/F0A34/F0A60/2A5F4/29CA4/2A1C0
//through28608. It is not a battle-frame, pause-input or device-audio receipt.
struct BattleSceneServices {
    std::array<u8,40> pause_context{};
    SoundControlSnapshot sound;
    u8 scene1828=0;
    std::array<u8,4> background_rgba{64,64,64,0};
    std::array<u8,4> sdk_background_rgba{64,64,64,0};
    std::array<u8,4> display_background_rgba{64,64,64,0};
    ChallengeBattleTeamPlan team_fields;
    std::array<u8,6> fields_c4_to_c9{};
    //Fresh scene clear;29CA4 does not write team+24. Later roster/cost
    //transitions must update this owner before contact health accounting.
    std::array<s32,4> team_remaining118{};
    bool current()const{return sound.live()&&sound.battle_flags();}
};
} // namespace gf
