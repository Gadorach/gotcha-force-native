#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/borgs/normal_gred_beam_muzzle.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"normal G-Red beam muzzle: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    constexpr NormalGRedNeutralBSource red{0x0615,0,3};
    GRedNeutralBReleasePlan paid;
    check(normal_gred_neutral_b_debit(red,{3,2,10,true},{5,5,180,0},&paid),"detached debit");
    GRedBeamNoTargetBasis basis{3,3,
        {{0,0,2,100, 0,3,0,-10, -4,0,0,30, 0,0,0,1}},
        {{-4,0,3,-500, 3,0,4,-600, 0,1,0,-700, 0,0,0,1}}};
    GRedBeamMuzzlePlan plan;
    check(normal_gred_beam_no_target_muzzle(red,paid,true,basis,&plan),"separate declared parent and credit bases");
    check(plan.origin174[0]==160.f&&std::abs(plan.origin174[1]+20.8f)<1e-5f&&
        std::abs(plan.origin174[2]-457.2f)<1e-4f,"local offset is transformed by all parent rows and translation");
    check(plan.unit_bearing==std::array<f32,3>{.6f,.8f,0.f},"credit Z is normalized, without its translation");

    const auto accepted=plan;
    auto unqualified=basis;unqualified.credit_slot=2;
    check(!normal_gred_beam_no_target_muzzle(red,paid,true,unqualified,&plan),"wrong credited slot refused");
    unqualified=basis;unqualified.parent_slot=2;
    check(!normal_gred_beam_no_target_muzzle(red,paid,true,unqualified,&plan),"wrong parent slot refused");
    check(!normal_gred_beam_no_target_muzzle(red,paid,false,basis,&plan)&&
        !normal_gred_beam_no_target_muzzle({0x0615,1,3},paid,true,basis,&plan),"targeted and alternate contexts refused");
    auto wrong=paid;wrong.argument=4;
    check(!normal_gred_beam_no_target_muzzle(red,wrong,true,basis,&plan),"foreign object argument refused");
    wrong=paid;wrong.request_beam=false;
    check(!normal_gred_beam_no_target_muzzle(red,wrong,true,basis,&plan),"no debit proposal refused");
    unqualified=basis;for(unsigned i=0;i<3;++i)unqualified.credit_key0.at(i,2)=0;
    check(!normal_gred_beam_no_target_muzzle(red,paid,true,unqualified,&plan),"zero credit axis does not invent a direction");
    unqualified=basis;unqualified.parent_key7.at(2,1)=std::numeric_limits<f32>::quiet_NaN();
    check(!normal_gred_beam_no_target_muzzle(red,paid,true,unqualified,&plan),"nonfinite parent refused");
    unqualified=basis;unqualified.credit_key0.at(3,3)=2;
    check(!normal_gred_beam_no_target_muzzle(red,paid,true,unqualified,&plan)&&
        plan.origin174==accepted.origin174&&plan.unit_bearing==accepted.unit_bearing,
        "invalid candidate leaves both outputs untouched");
    std::puts("normal G-Red beam muzzle: detached origin, credit bearing and refusals PASS");
}
