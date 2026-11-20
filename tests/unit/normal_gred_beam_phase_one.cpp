#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/borgs/normal_gred_beam_phase_one.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"normal G-Red phase one: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using V=std::array<f32,3>;
    using R=GRedBeamActiveRoute;
    constexpr NormalGRedNeutralBSource source{0x0615,0,3};
    GRedBeamSlotSnapshot slots;
    GRedBeamProximityPlan prox;
    check(!normal_gred_beam_proximity_plan(source,slots,{},0,&prox),"unknown six slots are not empty slots");
    slots.declared=true;
    slots.slots[0]={true,true,{350,0,0}};
    slots.slots[4]={true,true,{0,0,300}};
    check(normal_gred_beam_proximity_plan(source,slots,{},0,&prox)&&prox.examined_mask==0x11&&
        prox.stop_slot==255&&prox.weight29c==3&&prox.position20==V{3,0,3},
        "two far slots use one weight increase and the same old position");
    slots.slots[2]={true,true,{0,150,0}};
    check(normal_gred_beam_proximity_plan(source,slots,{},0,&prox)&&prox.examined_mask==5&&
        prox.stop_slot==2&&prox.weight29c==3&&prox.position20==V{},
        "inclusive near stop discards the earlier nudge but not the weight increase");
    slots.selection_eb=4;
    check(normal_gred_beam_proximity_plan(source,slots,{},5,&prox)&&prox.examined_mask==4&&
        prox.stop_slot==2&&prox.weight29c==5,"bit mask skips the first far slot");
    slots.selection_eb=0x10;
    check(normal_gred_beam_proximity_plan(source,slots,{},99,&prox)&&prox.examined_mask==0x10&&
        prox.stop_slot==255&&prox.weight29c==100&&prox.position20==V{0,0,100},"first far weight caps at 100");
    slots.selection_eb=0x80;
    slots.slots[0].position20[0]=std::numeric_limits<f32>::quiet_NaN();
    check(normal_gred_beam_proximity_plan(source,slots,{},5,&prox)&&!prox.examined_mask&&
        prox.weight29c==5&&prox.position20==V{},"high-bit-only nonzero mask is not an unrestricted scene");
    slots.selection_eb=1;slots.slots[0].present68=false;
    check(normal_gred_beam_proximity_plan(source,slots,{},5,&prox)&&!prox.examined_mask,"null slot pointer skips position");
    slots.slots[0].present68=true;
    check(!normal_gred_beam_proximity_plan(source,slots,{},5,&prox)&&prox.weight29c==5,"bad sampled position refuses");

    constexpr StageProjectileBounds extent{{-500,300,-430,900,500,true},400,true};
    bool outside=true;
    check(stage_projectile_outside(extent,{-1250,-1000,-1180},&outside)&&!outside&&
        stage_projectile_outside(extent,{1050,1150,1650},&outside)&&!outside,
        "extended X/Z and asymmetric vertical endpoints are inclusive");
    for(const V point:{V{-1251,0,0},V{1051,0,0},V{0,-1001,0},V{0,1151,0},V{0,0,-1181},V{0,0,1651}})
        check(stage_projectile_outside(extent,point,&outside)&&outside,"strictly outside one face");
    auto unknown=extent;unknown.top_declared=false;
    check(!stage_projectile_outside(unknown,{},&outside)&&outside,"unobserved stage ceiling leaves result unchanged");

    GRedBeamPhaseOneInputs in;
    in.flight={{},{},{0,0,80},80,0};in.lifetime1c=300;in.null_target_c8=in.previous_declared=true;
    in.slots.declared=true;in.slots.slots[3]={true,true,{0,0,200}};
    GRedBeamPhaseOnePlan phase;
    std::string error;
    check(normal_gred_beam_phase_one_plan(source,in,{},{},&phase,&error)&&
        phase.flight.position20==V{0,0,80}&&phase.flight.speed44==80.5f&&phase.proximity.stop_slot==3&&
        phase.active.route==R::ProximityStop&&phase.active.lifetime1c==300&&!phase.stage_visited&&!phase.bounds_visited,
        "post-motion proximity stop needs no stage results");
    in.slots.slots[3].position20={300,0,80};in.flags1d9=2;
    check(normal_gred_beam_phase_one_plan(source,in,{},{},&phase,&error)&&phase.active.route==R::LatchStop&&
        phase.flight.position20==V{3,0,80}&&phase.flight.distance180==80&&phase.proximity.weight29c==3&&
        !phase.stage_visited&&!phase.bounds_visited,"far nudge precedes latch without refreshing the prefix distance");
    in.flags1d9=0;
    check(!normal_gred_beam_phase_one_plan(source,in,{},{},&phase,&error)&&phase.active.route==R::LatchStop,
        "clear early gates cannot stand in for missing geometry");
    std::puts("normal G-Red phase one: detached slot order, bounds and early stops PASS");
}
