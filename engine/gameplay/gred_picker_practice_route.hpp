#pragma once
#include <array>
#include <string_view>
#include "engine/core/types.hpp"

namespace gf {
// Read-only training choices for the qualified normal G-Red lab, not
// variants or readiness/admission receipts.
enum class GRedPickerPracticeRoute : u8 {
    Standard,RecoverySeam,TimedRelaunchFloor,InclineDashJump,TimedLandingDash,DirectExpiryRelaunch,
    PositiveFloorHandoff,ExpiredFloorHandoff,TimedSnapDash,
    ExpiryStepHandoff,DirectTimedRelaunch,DirectTimedUpRelaunch,ActiveSnapRelaunch,
    ExpiringDirectRelaunch,ExpiringDirectRelaunchPush,
    ExpiringAdjacentRelaunchPush,ExpiringAirDash,ExpiringAirDashRelaunch,
    InclinedLedgeRelaunch,TimedReleaseThreePushes,
    ExpiringAirDashSnap,ExpiringAirDashSnapRelaunch,
    ExpiringShortAirDashRelaunch,
    TimedReleasePush,TimedReleaseTwoPushes,
    SlopeJoin,InclinedWalk,InclinedDash,LedgePush,HighAnglePush,
    TimedHandoff,ExpiryHandoff,ExpiryAirDash,ExpiryPush,Count
};
inline constexpr std::array<std::string_view,unsigned(GRedPickerPracticeRoute::Count)> kGRedPickerPracticeRouteNames{
    "STANDARD","RECOVERY SEAM","TIMED RELAUNCH FLOOR","INCLINE DASH JUMP","TIMED LANDING DASH","DIRECT EXPIRY RELAUNCH",
    "POSITIVE FLOOR HANDOFF","EXPIRED FLOOR HANDOFF","TIMED SNAP DASH",
    "EXPIRY STEP HANDOFF","DIRECT TIMED RELAUNCH","DIRECT TIMED UP RELAUNCH","ACTIVE SNAP RELAUNCH",
    "RELAUNCH ON EXPIRY","ENTRY PUSH ON EXPIRY",
    "ADJACENT ENTRY PUSH","AIR DASH ON EXPIRY","EXPIRY WIND-UP RELAUNCH",
    "INCLINED LEDGE RELAUNCH","THREE TIMED RELEASE PUSHES",
    "EXPIRY WIND-UP SNAP","EXPIRY SNAP RELAUNCH",
    "DIAGONAL EXPIRY RELAUNCH",
    "TIMED RELEASE PUSH","TWO TIMED RELEASE PUSHES",
    "SLOPE JOIN","INCLINED WALK","INCLINED DASH","WALK LEDGE PUSH","STEEPER LEDGE PUSH",
    "TIMED HANDOFF","EXPIRY HANDOFF","EXPIRY AIR DASH","LEDGE TIMER EXPIRY"
};
inline constexpr std::string_view gred_picker_practice_route_name(GRedPickerPracticeRoute route){
    return unsigned(route)<kGRedPickerPracticeRouteNames.size()?kGRedPickerPracticeRouteNames[unsigned(route)]:std::string_view{};
}
inline constexpr GRedPickerPracticeRoute cycle_gred_picker_practice_route(GRedPickerPracticeRoute route,bool previous){
    constexpr auto count=unsigned(GRedPickerPracticeRoute::Count);
    return GRedPickerPracticeRoute((unsigned(route)+(previous?count-1:1))%count);
}
}
