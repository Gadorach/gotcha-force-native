#pragma once
#include <array>
#include "engine/core/types.hpp"

namespace gf {
// Program IDs in normal G-Red's qualified locomotion profile, not clip IDs
// or selectors. Directional tables use the corresponding validated 0..3
// class; their order is local to this Borg.
struct NormalGRedMovementPrograms {
    static constexpr u8 idle=0,walk=1,walk_stop=31;
    static constexpr u8 jump_takeoff=2,jump_lift=4,jump_fall=6,landing=8;
    static constexpr u8 landing_prefix=jump_takeoff;
    static constexpr u8 air_dash_windup=10,air_dash_active=11,air_dash_recovery=12;
    static constexpr u8 momentum_fall=13,relaunch_windup=14,relaunch_active=15;
    static constexpr std::array<u8,4> ground_dash{27,28,29,30};
    static constexpr std::array<u8,4> relaunch_fall{21,22,23,24};
};
}
