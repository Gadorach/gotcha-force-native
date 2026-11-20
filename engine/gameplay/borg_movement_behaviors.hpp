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
    RelaunchEntryPush, WindupEntryPush, SlopeDash, CoplanarSlopeDash, ShallowCreaseDash, FlatInclineDash,
    InclinedDashLedge, WalkLedgeFallPush, WalkLedgeHighAnglePush, WalkLedgeExpiryPush, WalkLedgeExpiryAirDash,
    RecoveryFloorRecontact, RecoveryRecontactDrift, RecoveryRecontactSeam, RecoveryFirstContactSeam,
    WalkLedgeDirectTimedAirDash, TimedPlainWindupRelaunch, ExpiryPlainWindupRelaunch, WalkExpiryDirectRelaunch,
    TimedSnapWindupRelaunch, PositiveSnapWindupRelaunch, ExpiringSnapWindupRelaunch, WalkTimedDirectRelaunch,
    WalkTimedDirectUpRelaunch, TimedWalkReleasePush, TimedWalkReleaseSecondPush, WalkExpiringDirectRelaunch,
    WalkExpiringDirectRelaunchPush, WalkExpiringAdjacentRelaunchPush, WalkExpiringAirDash, WalkExpiringAirDashRelaunch,
    WalkExpiringShortAirDashRelaunch, WalkExpiringAirDashSnap, WalkExpiringAirDashSnapRelaunch, TimedWalkReleaseThirdPush,
    TimedWalkReleaseDownPush, RisingPlainWindupRelaunch, Count
};
static_assert(unsigned(BorgMovementBehavior::Count)<=64);

struct BorgMovementBehaviors {
    constexpr bool has(BorgMovementBehavior behavior)const{return (bits_&bit(behavior))!=0;}
    constexpr void add(BorgMovementBehavior behavior){bits_|=bit(behavior);}
    // This selects permission to ask for terrain proof, not the proof itself.
    constexpr bool allows_shallow_dash_join(bool start_flat,bool end_flat)const{
        return start_flat!=end_flat?has(BorgMovementBehavior::FlatInclineDash):
            !start_flat&&has(BorgMovementBehavior::ShallowCreaseDash);
    }
private:
    static constexpr u64 bit(BorgMovementBehavior behavior){
        return behavior<BorgMovementBehavior::Count?u64{1}<<unsigned(behavior):0;
    }
    u64 bits_=0;
};
}
