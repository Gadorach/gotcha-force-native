#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_movement.hpp"

namespace {
void check(bool ok,const char* message){
    if(!ok){std::fprintf(stderr,"normal G-Red movement: %s\n",message);std::abort();}
}
}

int main(){
    using Definition=gf::NormalGRedMovementDefinition;
    using Mode=Definition::Mode;
    using Policy=Definition::Policy;
    using Behavior=Definition::Behavior;
    using Programs=Definition::Programs;
    static_assert(Programs::idle==0&&Programs::walk==1&&Programs::walk_stop==31);
    static_assert(Programs::jump_takeoff==2&&Programs::jump_lift==4&&Programs::jump_fall==6&&Programs::landing==8&&
        Programs::landing_prefix==2&&Programs::momentum_fall==13);
    static_assert(Programs::air_dash_windup==10&&Programs::air_dash_active==11&&Programs::air_dash_recovery==12);
    static_assert(Programs::relaunch_windup==14&&Programs::relaunch_active==15);
    static_assert(Programs::ground_dash==std::array<gf::u8,4>{27,28,29,30});
    static_assert(Programs::relaunch_fall==std::array<gf::u8,4>{21,22,23,24});
    static_assert(unsigned(Mode::OrderedGround)==22&&unsigned(Mode::GroundSlopeCreaseDash)==49);
    static_assert(unsigned(Policy::OrderedGround)==18&&unsigned(Policy::GroundSlopeCreaseDash)==45);
    static_assert(unsigned(Mode::GroundSlopeTransitionDash)==50&&unsigned(Policy::GroundSlopeTransitionDash)==46);
    static_assert(unsigned(Mode::GroundInclinedDashLedge)==51&&unsigned(Policy::GroundInclinedDashLedge)==47);
    static_assert(unsigned(Mode::GroundWalkLedgeFallPush)==52&&unsigned(Policy::GroundWalkLedgeFallPush)==48);
    static_assert(unsigned(Mode::GroundWalkLedgeHighAnglePush)==53&&unsigned(Policy::GroundWalkLedgeHighAnglePush)==49);
    static_assert(unsigned(Mode::GroundWalkLedgeExpiryPush)==54&&unsigned(Policy::GroundWalkLedgeExpiryPush)==50);
    static_assert(unsigned(Mode::GroundWalkLedgeExpiryAirDash)==55&&unsigned(Policy::GroundWalkLedgeExpiryAirDash)==51&&
        !Definition::allows(Mode::GroundWalkLedgeExpiryPush,Behavior::WalkLedgeExpiryAirDash)&&
        Definition::allows(Mode::GroundWalkLedgeExpiryAirDash,Behavior::WalkLedgeExpiryAirDash));
    static_assert(unsigned(Mode::GroundRecoveryRecontact)==56&&unsigned(Policy::GroundRecoveryRecontact)==52&&
        !Definition::allows(Mode::GroundWalkLedgeExpiryAirDash,Behavior::RecoveryFloorRecontact)&&
        Definition::allows(Mode::GroundRecoveryRecontact,Behavior::RecoveryFloorRecontact));
    static_assert(unsigned(Mode::GroundRecoveryRecontactDrift)==57&&unsigned(Policy::GroundRecoveryRecontactDrift)==53&&
        !Definition::allows(Mode::GroundRecoveryRecontact,Behavior::RecoveryRecontactDrift)&&
        Definition::allows(Mode::GroundRecoveryRecontactDrift,Behavior::RecoveryRecontactDrift));
    static_assert(unsigned(Mode::GroundRecoveryRecontactSeam)==58&&unsigned(Policy::GroundRecoveryRecontactSeam)==54&&
        !Definition::allows(Mode::GroundRecoveryRecontactDrift,Behavior::RecoveryRecontactSeam)&&
        Definition::allows(Mode::GroundRecoveryRecontactSeam,Behavior::RecoveryRecontactSeam));
    static_assert(unsigned(Mode::GroundRecoveryFirstContactSeam)==59&&unsigned(Policy::GroundRecoveryFirstContactSeam)==55&&
        !Definition::allows(Mode::GroundRecoveryRecontactSeam,Behavior::RecoveryFirstContactSeam)&&
        Definition::allows(Mode::GroundRecoveryFirstContactSeam,Behavior::RecoveryFirstContactSeam));
    static_assert(unsigned(Mode::GroundWalkLedgeDirectTimedAirDash)==60&&unsigned(Policy::GroundWalkLedgeDirectTimedAirDash)==56&&
        !Definition::allows(Mode::GroundWalkLedgeExpiryAirDash,Behavior::WalkLedgeDirectTimedAirDash)&&
        Definition::allows(Mode::GroundWalkLedgeDirectTimedAirDash,Behavior::WalkLedgeDirectTimedAirDash));
    static_assert(unsigned(Mode::GroundTimedPlainWindupRelaunch)==61&&unsigned(Policy::GroundTimedPlainWindupRelaunch)==57&&
        !Definition::allows(Mode::GroundWalkLedgeDirectTimedAirDash,Behavior::TimedPlainWindupRelaunch)&&
        Definition::allows(Mode::GroundTimedPlainWindupRelaunch,Behavior::TimedPlainWindupRelaunch));
    static_assert(unsigned(Behavior::WalkLedgeExpiryPush)==32&&
        !Definition::allows(Mode::GroundWalkLedgeHighAnglePush,Behavior::WalkLedgeExpiryPush)&&
        Definition::allows(Mode::GroundWalkLedgeExpiryPush,Behavior::WalkLedgeExpiryPush));
    static_assert(Definition::walk_ledge_push_angle_cap(Definition::behaviors(Policy::GroundWalkLedgeFallPush))==10240&&
        Definition::walk_ledge_push_angle_cap(Definition::behaviors(Policy::GroundWalkLedgeHighAnglePush))==13653);
    constexpr Policy legacy[]{
        Policy::GroundOnly,Policy::GroundOnly,Policy::VerticalJump,Policy::Jump,Policy::AirDash,Policy::Relaunch,
        Policy::Relaunch,Policy::Relaunch,Policy::Relaunch,Policy::StaticPush,Policy::FallHistoryPush,
        Policy::RecoveryPush,Policy::WindupPush,Policy::ActivePush,Policy::RelaunchPush,Policy::RelaunchEntryPush,
        Policy::RelaunchFloor,Policy::AirDashFloor,Policy::WindupEntryFloor,Policy::WindupPositiveFloor,
        Policy::WindupEntryPush,Policy::WalkLedge
    };
    static_assert(std::size(legacy)==unsigned(Mode::OrderedGround));
    for(unsigned i=0;i<std::size(legacy);++i){
        const auto mode=Mode(i);
        check(Definition::native_policy(mode)==legacy[i],"historical mode mapping");
        for(unsigned b=0;b<unsigned(Behavior::Count);++b)
            check(!Definition::allows(mode,Behavior(b)),"legacy modes do not inherit combined contact authority");
    }
    for(unsigned i=0;i<Definition::combined.size();++i){
        const auto mode=Mode(unsigned(Mode::OrderedGround)+i);
        const auto policy=Policy(unsigned(Policy::OrderedGround)+i);
        check(Definition::native_policy(mode)==policy,"combined native policy IDs stay stable");
        for(unsigned b=0;b<unsigned(Behavior::Count);++b)
            check(Definition::allows(mode,Behavior(b))==(b<=i),"each lab adds exactly one reviewed behavior");
    }
    check(!Definition::allows(Mode::GroundSlopeSeamDash,Behavior::ShallowCreaseDash),"exact seams do not permit a crease");
    const auto crease=Definition::behaviors(Policy::GroundSlopeCreaseDash);
    const auto mixed=Definition::behaviors(Policy::GroundSlopeTransitionDash);
    check(crease.allows_shallow_dash_join(false,false)&&!crease.allows_shallow_dash_join(true,false)&&
        !crease.allows_shallow_dash_join(false,true),"the earlier crease policy only allows pairs of inclined faces");
    check(mixed.allows_shallow_dash_join(false,false)&&mixed.allows_shallow_dash_join(true,false)&&
        mixed.allows_shallow_dash_join(false,true)&&!mixed.allows_shallow_dash_join(true,true),
        "the opt-in transition allows either mixed direction but cannot use slope proof on two flat faces");
    check(!mixed.has(Behavior::InclinedDashLedge)&&Definition::allows(Mode::GroundInclinedDashLedge,Behavior::InclinedDashLedge),
        "the new departure permission is independent of continuous slope joins");
    check(!Definition::allows(Mode::GroundLossRelaunch,Behavior::FallPush),"positive timer coverage does not grant generic push");
    check(!Definition::allows(Mode::GroundSlopeCreaseDash,Behavior(255))&&
        !Definition::behaviors(Policy(255)).has(Behavior::OrderedGround),"unknown IDs cannot grant behavior");
    constexpr auto& meter=Definition::air_meter;
    static_assert(meter.matches_operands(10000,0,2000,{150,100,450,50}));
    static_assert(meter.lift_cost(true)==150&&meter.lift_cost(false)==100);
    static_assert(meter.cost(gf::AirMeterCharge::DashHandoff)==450&&meter.cost(gf::AirMeterCharge::DashActive)==50);
    static_assert(!meter.matches_operands(10000,0,2000,{150,100,50,450}));
    static_assert(!meter.matches_operands(10000,1,2000,{150,100,450,50}));
    gf::CharacterAirResourceState state{9850,0,1},next;
    check(gf::character_air_resource_recover(state,meter,0,1,&next)&&next.value736==9850&&next.air_actions747==1,
        "airborne cache prevents refill");
    check(gf::character_air_resource_recover(state,meter,0x80,0,&next)&&next.value736==10000&&next.air_actions747==0,
        "previous grounded cache refills to the cap independently of behavior80");
    check(gf::character_air_resource_consume(state,meter,gf::u8(gf::AirMeterCharge::DashHandoff),1,&next)&&next.value736==9400,
        "shared spend uses the defined handoff amount");
    check(gf::character_air_resource_consume({25,0,1},meter,gf::u8(gf::AirMeterCharge::DashActive),1,&next)&&next.value736==0,
        "shared spend saturates on depletion");
    check(!gf::character_air_resource_consume(state,meter,255,1,&next),"an unknown cost slot cannot spend");
    for(gf::u8 movement_type:{2,3,6}){
        check(gf::character_air_resource_recover(state,movement_type,0x80,0,meter.limit,meter.increment,&next)&&
            next.value736==9850&&next.air_actions747==1,"legacy special gate refuses refill with bit 0x80 set");
        check(gf::character_air_resource_recover(state,movement_type,0,1,meter.limit,meter.increment,&next)&&
            next.value736==10000&&next.air_actions747==1,"legacy special gate ignores ground cache and preserves action count");
    }
    std::puts("normal G-Red movement: stable program IDs, legacy/combined capabilities and meter boundaries PASS");
}
