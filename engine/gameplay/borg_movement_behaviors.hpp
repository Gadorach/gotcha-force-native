#pragma once
#include "engine/core/types.hpp"

namespace gf {
// Validated behaviors a Borg definition can select. This list does not
// implement terrain queries, choose animations, or make a movement offer valid.
enum class BorgMovementBehavior:u8 {
    OrderedGround, DashLedge, WalkLedge, Jump, LandingInterrupt, AirDash,
    AirDashFloor, Relaunch, RelaunchFloor, WindupEntryFloor, PairedSurfaceLanding,
    GroundLossSurface, RetainedTimerLandingJump, RetainedTimerLandingDash,
    RetainedTimerAirDash, RetainedTimerRelaunch, FallPush, PostAirFallPush,
    ReleasedRelaunchPush, RecoveryPush, WindupPush, ActivePush, RelaunchPush,
    RelaunchEntryPush, WindupEntryPush, SlopeDash, CoplanarSlopeDash, ShallowCreaseDash, Count
};
static_assert(unsigned(BorgMovementBehavior::Count)<=32);

struct BorgMovementBehaviors {
    constexpr bool has(BorgMovementBehavior behavior)const{return (bits_&bit(behavior))!=0;}
    constexpr void add(BorgMovementBehavior behavior){bits_|=bit(behavior);}
private:
    static constexpr u32 bit(BorgMovementBehavior behavior){
        return behavior<BorgMovementBehavior::Count?u32{1}<<unsigned(behavior):0;
    }
    u32 bits_=0;
};
}
