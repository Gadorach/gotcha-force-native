#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_flight.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"normal G-Red beam flight: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using V=std::array<f32,3>;
    using S=GRedBeamStopSample;
    using R=GRedBeamActiveRoute;
    constexpr NormalGRedNeutralBSource source{0x0615,0,3};
    constexpr GRedBeamMuzzlePlan muzzle{{12,-7,40},{0,0,-1}};
    GRedBeamFlightSample flight;
    check(normal_gred_beam_flight_seed(source,muzzle,&flight)&&flight.origin174==muzzle.origin174&&
        flight.position20==V{12,-7,39}&&flight.motion38==V{0,0,-80}&&flight.speed44==80&&flight.distance180==1,
        "initializer adds one bearing, not one speed");
    check(!normal_gred_beam_flight_prefix(source,false,flight,&flight)&&flight.distance180==1,
        "no targeted fallback in this numeric branch");
    check(normal_gred_beam_flight_prefix(source,true,flight,&flight)&&flight.position20==V{12,-7,-41}&&
        flight.motion38==V{0,0,-80}&&flight.speed44==80.5f&&flight.distance180==81,
        "first active visit moves at old speed before acceleration");
    flight.speed44=159.75f;
    check(normal_gred_beam_flight_prefix(source,true,flight,&flight)&&flight.motion38==V{0,0,-159.75f}&&flight.speed44==160,
        "last increase clamps only the next speed");
    auto zero=flight;zero.motion38={};
    check(!normal_gred_beam_flight_prefix(source,true,zero,&flight)&&flight.speed44==160,"zero direction refuses without writes");
    check(!normal_gred_beam_flight_seed(source,{{},{0,0,80}},&flight),"initializer cannot mistake a speed vector for a unit bearing");

    GRedBeamActivePlan plan;
    check(normal_gred_beam_active_plan(source,{300,0xa2},&plan)&&plan.route==R::AwaitProximity&&plan.lifetime1c==300,
        "unobserved proximity is not a clear result");
    check(!normal_gred_beam_active_plan(source,{300,0,S::Unobserved,S::Clear},&plan)&&plan.route==R::AwaitProximity,
        "a later query cannot provide an earlier result");
    check(normal_gred_beam_active_plan(source,{300,0xa2,S::Stop},&plan)&&plan.route==R::ProximityStop&&
        plan.phase18==2&&plan.lifetime1c==300&&plan.marker19==1&&plan.request_remove_shapes&&!plan.refresh_stage_distance,
        "proximity stop wins before latch and lifetime");
    check(normal_gred_beam_active_plan(source,{300,0x02,S::Clear},&plan)&&plan.route==R::LatchStop&&plan.lifetime1c==300,
        "latch mask stops after proximity, before stage");
    check(normal_gred_beam_active_plan(source,{300,1,S::Clear},&plan)&&plan.route==R::AwaitStage,
        "unrelated latch bit does not stop");
    check(normal_gred_beam_active_plan(source,{300,0,S::Clear,S::Stop},&plan)&&plan.route==R::StageStop&&
        plan.lifetime1c==300&&plan.marker19==1&&plan.request_remove_shapes&&plan.refresh_stage_distance,
        "only the stage stop refreshes distance after the stage result");
    check(normal_gred_beam_active_plan(source,{300,0,S::Clear,S::Clear},&plan)&&plan.route==R::AwaitBounds&&plan.lifetime1c==300,
        "unobserved bounds do not tick lifetime");
    check(normal_gred_beam_active_plan(source,{1,0,S::Clear,S::Clear,S::Stop},&plan)&&plan.route==R::BoundsStop&&
        plan.lifetime1c==1&&plan.marker19==2&&!plan.refresh_stage_distance,"bounds stop wins before the decrement");
    check(normal_gred_beam_active_plan(source,{2,0,S::Clear,S::Clear,S::Clear},&plan)&&plan.route==R::Continue&&
        plan.lifetime1c==1&&plan.phase18==1&&!plan.marker19&&!plan.request_remove_shapes,"all-clear visit spends one count");
    check(normal_gred_beam_active_plan(source,{1,0,S::Clear,S::Clear,S::Clear},&plan)&&plan.route==R::Expired&&
        !plan.lifetime1c&&plan.phase18==2&&plan.marker19==2&&plan.request_remove_shapes,"last count expires after all checks");
    std::puts("normal G-Red beam flight: detached seed, movement and stop ordering PASS");
}
