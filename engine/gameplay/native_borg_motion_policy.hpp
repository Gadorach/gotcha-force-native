#pragma once
#include "engine/core/types.hpp"

namespace gf {
// Historical lab policy IDs. Keep ordinals stable; these are not Borg IDs.
enum class NativeBorgMotionPolicy:u8 {
    GroundOnly,VerticalJump,Jump,AirDash,Relaunch,StaticPush,FallHistoryPush,RecoveryPush,WindupPush,
    ActivePush,RelaunchPush,RelaunchEntryPush,RelaunchFloor,AirDashFloor,WindupEntryFloor,WindupPositiveFloor,
    WindupEntryPush,WalkLedge,OrderedGround,DashLedge,GroundLedge,GroundJump,GroundLanding,GroundAir,
    GroundAirFloor,GroundRelaunch,GroundRelaunchFloor,GroundWindupEntryFloor,GroundSurface,GroundLossSurface,
    GroundLossJump,GroundLossLanding,GroundLossAir,GroundLossRelaunch,GroundFallPush,GroundAirFallPush,
    GroundRelaunchFallPush,GroundRecoveryPush,GroundWindupPush,GroundActivePush,GroundRelaunchPush,
    GroundRelaunchEntryPush,GroundWindupEntryPush,GroundSlopeDash,GroundSlopeSeamDash,GroundSlopeCreaseDash,
    GroundSlopeTransitionDash,GroundInclinedDashLedge,GroundWalkLedgeFallPush,GroundWalkLedgeHighAnglePush,
    GroundWalkLedgeExpiryPush,GroundWalkLedgeExpiryAirDash,GroundRecoveryRecontact,GroundRecoveryRecontactDrift,
    GroundRecoveryRecontactSeam,GroundRecoveryFirstContactSeam,GroundWalkLedgeDirectTimedAirDash,
    GroundTimedPlainWindupRelaunch,GroundExpiryPlainWindupRelaunch,GroundWalkExpiryDirectRelaunch,GroundTimedSnapWindupRelaunch,
    GroundPositiveSnapWindupRelaunch,GroundExpiringSnapWindupRelaunch,GroundWalkTimedDirectRelaunch,GroundWalkTimedDirectUpRelaunch,
    GroundTimedWalkReleasePush,GroundTimedWalkReleaseSecondPush,GroundWalkExpiringDirectRelaunch,GroundWalkExpiringDirectRelaunchPush,
    GroundWalkExpiringAdjacentRelaunchPush,GroundWalkExpiringAirDash,GroundWalkExpiringAirDashRelaunch,
    GroundWalkExpiringShortAirDashRelaunch,GroundWalkExpiringAirDashSnap,GroundWalkExpiringAirDashSnapRelaunch,GroundTimedWalkReleaseThirdPush,
    GroundTimedWalkReleaseDownPush,GroundRisingPlainWindupRelaunch,GroundRisingDiagonalWindupRelaunch,GroundWalkExpirySameFacePush
};
}
