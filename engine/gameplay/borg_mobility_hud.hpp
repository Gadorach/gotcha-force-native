#pragma once

#include "engine/gameplay/native_battle_mvp.hpp"

namespace gf {

enum class BorgMobilityKind:u8 {Lift,Jumps,AirActions,Boost};

struct BorgMobilityHud {
    BorgMobilityKind kind;
    u16 remaining,capacity;
    bool operator==(const BorgMobilityHud&)const=default;
};

struct BorgMobilityLabels {const char* trace;const char* title;};
inline BorgMobilityLabels borg_mobility_labels(BorgMobilityKind kind){
    switch(kind){
    case BorgMobilityKind::Lift:return {"lift","LIFT"};
    case BorgMobilityKind::Jumps:return {"jumps","JUMPS"};
    case BorgMobilityKind::AirActions:return {"air-actions","AIR ACTIONS"};
    case BorgMobilityKind::Boost:return {"boost","BOOST"};
    }
    return {"-","-"};
}

inline std::string borg_mobility_text(const BorgMobilityHud& row){
    return std::string(borg_mobility_labels(row.kind).title)+" "+
        std::to_string(row.remaining)+"/"+std::to_string(row.capacity);
}

// Report the native package resource, not a prediction of whether the
// next input is allowed. Borrowed source motion keeps its own readout.
inline std::optional<BorgMobilityHud> borg_mobility_hud(const BorgGameplayProfile& profile,const BattleMvpActor& actor){
    if(!actor.health||profile.movement!=BorgBattleMovement::Grounded||actor.source_ground_motion)return {};
    if(profile.cruise){
        const auto capacity=profile.cruise->capacity;
        const auto value=actor.package_boost_meter.value736;
        if(!capacity||value<0||value>capacity)return {};
        return BorgMobilityHud{BorgMobilityKind::Boost,u16(value),capacity};
    }
    const auto unused=[](BorgMobilityKind kind,u16 capacity,u16 used)->std::optional<BorgMobilityHud>{
        if(!capacity||used>capacity)return {};
        return BorgMobilityHud{kind,u16(capacity-used),capacity};
    };
    if(profile.air_action_budget)return unused(BorgMobilityKind::AirActions,profile.air_action_budget,actor.package_air_points_used);
    if(profile.air_jump_points)return unused(BorgMobilityKind::Jumps,profile.air_jump_points,actor.package_air_points_used);
    return unused(BorgMobilityKind::Lift,profile.flight_ticks,actor.package_flight_used);
}

} // namespace gf
