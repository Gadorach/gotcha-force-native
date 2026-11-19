#pragma once
#include <array>
#include "engine/gameplay/borg_movement_behaviors.hpp"
#include "engine/gameplay/borgs/normal_gred_movement_admission.hpp"
#include "engine/gameplay/borgs/normal_gred_movement_programs.hpp"
#include "engine/gameplay/character_air_resource.hpp"
#include "engine/gameplay/native_borg_motion_policy.hpp"

namespace gf {
// G-Red's existing source harness modes. Their ordering predates the native
// policy IDs, so the mapping below is explicit.
enum class NormalGRedMovementMode {
    DashOnly,Ground,VerticalJump,Jump,AirDash,Relaunch,DashJump,LandingInterrupt,Surface,Steep,FallingSurface,
    RecoverySurface,WindupSurface,ActiveSurface,RelaunchSurface,RelaunchEntrySurface,RelaunchFloor,AirDashFloor,
    WindupEntryFloor,WindupPositiveFloor,WindupEntryPush,WalkLedge,OrderedGround,DashLedge,GroundLedge,
    GroundJump,GroundLanding,GroundAir,GroundAirFloor,GroundRelaunch,GroundRelaunchFloor,GroundWindupEntryFloor,
    GroundSurface,GroundLossSurface,GroundLossJump,GroundLossLanding,GroundLossAir,GroundLossRelaunch,
    GroundFallPush,GroundAirFallPush,GroundRelaunchFallPush,GroundRecoveryPush,GroundWindupPush,GroundActivePush,
    GroundRelaunchPush,GroundRelaunchEntryPush,GroundWindupEntryPush,GroundSlopeDash,GroundSlopeSeamDash,GroundSlopeCreaseDash,
    GroundSlopeTransitionDash,GroundInclinedDashLedge,GroundWalkLedgeFallPush,GroundWalkLedgeHighAnglePush,
    GroundWalkLedgeExpiryPush,GroundWalkLedgeExpiryAirDash,GroundRecoveryRecontact,GroundRecoveryRecontactDrift,
    GroundRecoveryRecontactSeam,GroundRecoveryFirstContactSeam,GroundWalkLedgeDirectTimedAirDash,
    GroundTimedPlainWindupRelaunch,GroundExpiryPlainWindupRelaunch
};

struct NormalGRedMovementDefinition {
    static constexpr u16 model=0x615;
    // GFMI1 normal G-Red operands, checked again against the qualified pack
    // at source bind. Cached ground state controls refill, not physical Y.
    static constexpr AirMeterRules air_meter{
        AirMeterRecoveryGate::PreviousGroundedCache,10000,0,2000,{150,100,450,50}
    };
    using Mode=NormalGRedMovementMode;
    using Policy=NativeBorgMotionPolicy;
    using Behavior=BorgMovementBehavior;
    using Programs=NormalGRedMovementPrograms;
    using Admission=NormalGRedMovementAdmissionRules;
    struct CombinedRule {Mode mode;Policy policy;Behavior adds;};

    // Each opt-in combined lab inherits the preceding behaviors plus its own
    // explicitly added permission. Legacy labs never inherit this list.
    static constexpr std::array combined{
        CombinedRule{Mode::OrderedGround,Policy::OrderedGround,Behavior::OrderedGround},
        CombinedRule{Mode::DashLedge,Policy::DashLedge,Behavior::DashLedge},
        CombinedRule{Mode::GroundLedge,Policy::GroundLedge,Behavior::WalkLedge},
        CombinedRule{Mode::GroundJump,Policy::GroundJump,Behavior::Jump},
        CombinedRule{Mode::GroundLanding,Policy::GroundLanding,Behavior::LandingInterrupt},
        CombinedRule{Mode::GroundAir,Policy::GroundAir,Behavior::AirDash},
        CombinedRule{Mode::GroundAirFloor,Policy::GroundAirFloor,Behavior::AirDashFloor},
        CombinedRule{Mode::GroundRelaunch,Policy::GroundRelaunch,Behavior::Relaunch},
        CombinedRule{Mode::GroundRelaunchFloor,Policy::GroundRelaunchFloor,Behavior::RelaunchFloor},
        CombinedRule{Mode::GroundWindupEntryFloor,Policy::GroundWindupEntryFloor,Behavior::WindupEntryFloor},
        CombinedRule{Mode::GroundSurface,Policy::GroundSurface,Behavior::PairedSurfaceLanding},
        CombinedRule{Mode::GroundLossSurface,Policy::GroundLossSurface,Behavior::GroundLossSurface},
        CombinedRule{Mode::GroundLossJump,Policy::GroundLossJump,Behavior::RetainedTimerLandingJump},
        CombinedRule{Mode::GroundLossLanding,Policy::GroundLossLanding,Behavior::RetainedTimerLandingDash},
        CombinedRule{Mode::GroundLossAir,Policy::GroundLossAir,Behavior::RetainedTimerAirDash},
        CombinedRule{Mode::GroundLossRelaunch,Policy::GroundLossRelaunch,Behavior::RetainedTimerRelaunch},
        CombinedRule{Mode::GroundFallPush,Policy::GroundFallPush,Behavior::FallPush},
        CombinedRule{Mode::GroundAirFallPush,Policy::GroundAirFallPush,Behavior::PostAirFallPush},
        CombinedRule{Mode::GroundRelaunchFallPush,Policy::GroundRelaunchFallPush,Behavior::ReleasedRelaunchPush},
        CombinedRule{Mode::GroundRecoveryPush,Policy::GroundRecoveryPush,Behavior::RecoveryPush},
        CombinedRule{Mode::GroundWindupPush,Policy::GroundWindupPush,Behavior::WindupPush},
        CombinedRule{Mode::GroundActivePush,Policy::GroundActivePush,Behavior::ActivePush},
        CombinedRule{Mode::GroundRelaunchPush,Policy::GroundRelaunchPush,Behavior::RelaunchPush},
        CombinedRule{Mode::GroundRelaunchEntryPush,Policy::GroundRelaunchEntryPush,Behavior::RelaunchEntryPush},
        CombinedRule{Mode::GroundWindupEntryPush,Policy::GroundWindupEntryPush,Behavior::WindupEntryPush},
        CombinedRule{Mode::GroundSlopeDash,Policy::GroundSlopeDash,Behavior::SlopeDash},
        CombinedRule{Mode::GroundSlopeSeamDash,Policy::GroundSlopeSeamDash,Behavior::CoplanarSlopeDash},
        CombinedRule{Mode::GroundSlopeCreaseDash,Policy::GroundSlopeCreaseDash,Behavior::ShallowCreaseDash},
        CombinedRule{Mode::GroundSlopeTransitionDash,Policy::GroundSlopeTransitionDash,Behavior::FlatInclineDash},
        CombinedRule{Mode::GroundInclinedDashLedge,Policy::GroundInclinedDashLedge,Behavior::InclinedDashLedge},
        CombinedRule{Mode::GroundWalkLedgeFallPush,Policy::GroundWalkLedgeFallPush,Behavior::WalkLedgeFallPush},
        CombinedRule{Mode::GroundWalkLedgeHighAnglePush,Policy::GroundWalkLedgeHighAnglePush,Behavior::WalkLedgeHighAnglePush},
        CombinedRule{Mode::GroundWalkLedgeExpiryPush,Policy::GroundWalkLedgeExpiryPush,Behavior::WalkLedgeExpiryPush},
        CombinedRule{Mode::GroundWalkLedgeExpiryAirDash,Policy::GroundWalkLedgeExpiryAirDash,Behavior::WalkLedgeExpiryAirDash},
        CombinedRule{Mode::GroundRecoveryRecontact,Policy::GroundRecoveryRecontact,Behavior::RecoveryFloorRecontact},
        CombinedRule{Mode::GroundRecoveryRecontactDrift,Policy::GroundRecoveryRecontactDrift,Behavior::RecoveryRecontactDrift},
        CombinedRule{Mode::GroundRecoveryRecontactSeam,Policy::GroundRecoveryRecontactSeam,Behavior::RecoveryRecontactSeam},
        CombinedRule{Mode::GroundRecoveryFirstContactSeam,Policy::GroundRecoveryFirstContactSeam,Behavior::RecoveryFirstContactSeam},
        CombinedRule{Mode::GroundWalkLedgeDirectTimedAirDash,Policy::GroundWalkLedgeDirectTimedAirDash,Behavior::WalkLedgeDirectTimedAirDash},
        CombinedRule{Mode::GroundTimedPlainWindupRelaunch,Policy::GroundTimedPlainWindupRelaunch,Behavior::TimedPlainWindupRelaunch},
        CombinedRule{Mode::GroundExpiryPlainWindupRelaunch,Policy::GroundExpiryPlainWindupRelaunch,Behavior::ExpiryPlainWindupRelaunch},
    };

    static constexpr Policy native_policy(Mode mode){
        const auto index=unsigned(mode)-unsigned(Mode::OrderedGround);
        if(index<combined.size()&&combined[index].mode==mode)return combined[index].policy;
        switch(mode){
        case Mode::VerticalJump:return Policy::VerticalJump;
        case Mode::Jump:return Policy::Jump;
        case Mode::AirDash:return Policy::AirDash;
        case Mode::Relaunch:case Mode::DashJump:case Mode::LandingInterrupt:case Mode::Surface:return Policy::Relaunch;
        case Mode::Steep:return Policy::StaticPush;
        case Mode::FallingSurface:return Policy::FallHistoryPush;
        case Mode::RecoverySurface:return Policy::RecoveryPush;
        case Mode::WindupSurface:return Policy::WindupPush;
        case Mode::ActiveSurface:return Policy::ActivePush;
        case Mode::RelaunchSurface:return Policy::RelaunchPush;
        case Mode::RelaunchEntrySurface:return Policy::RelaunchEntryPush;
        case Mode::RelaunchFloor:return Policy::RelaunchFloor;
        case Mode::AirDashFloor:return Policy::AirDashFloor;
        case Mode::WindupEntryFloor:return Policy::WindupEntryFloor;
        case Mode::WindupPositiveFloor:return Policy::WindupPositiveFloor;
        case Mode::WindupEntryPush:return Policy::WindupEntryPush;
        case Mode::WalkLedge:return Policy::WalkLedge;
        default:return Policy::GroundOnly;
        }
    }
    static constexpr BorgMovementBehaviors behaviors(Policy policy){
        const auto index=unsigned(policy)-unsigned(Policy::OrderedGround);
        return index<combined.size()&&combined[index].policy==policy?cumulative_[index]:BorgMovementBehaviors{};
    }
    static constexpr bool allows(Mode mode,Behavior behavior){return behaviors(native_policy(mode)).has(behavior);}
    // Native ceiling for this continued walking response. 13653 is the
    // steepest witnessed face, not a replacement for either source angle gate.
    static constexpr s16 walk_ledge_push_angle_cap(BorgMovementBehaviors flags){
        return flags.has(Behavior::WalkLedgeHighAnglePush)?13653:10240;
    }
private:
    static constexpr auto cumulative_=[]{
        std::array<BorgMovementBehaviors,combined.size()> table{};
        BorgMovementBehaviors result;
        for(unsigned i=0;i<combined.size();++i){result.add(combined[i].adds);table[i]=result;}
        return table;
    }();
};
}
