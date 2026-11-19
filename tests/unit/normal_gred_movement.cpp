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
    static_assert(unsigned(Mode::OrderedGround)==22&&unsigned(Mode::GroundSlopeCreaseDash)==49);
    static_assert(unsigned(Policy::OrderedGround)==18&&unsigned(Policy::GroundSlopeCreaseDash)==45);
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
    check(!Definition::allows(Mode::GroundLossRelaunch,Behavior::FallPush),"positive timer coverage does not grant generic push");
    check(!Definition::allows(Mode::GroundSlopeCreaseDash,Behavior(255))&&
        !Definition::behaviors(Policy(255)).has(Behavior::OrderedGround),"unknown IDs cannot grant behavior");
    std::puts("normal G-Red movement: stable legacy/combined capability boundaries PASS");
}
