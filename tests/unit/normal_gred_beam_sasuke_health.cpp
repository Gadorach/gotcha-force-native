#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_health.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red beam/Sasuke health: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    constexpr NormalGRedNeutralBSource red{0x0615,0,3};
    constexpr GRedBeamShapeOperands beam{{4,48,52,26,0,false},0,15,0,3,true,0,-2,20};
    GRedBeamSasukeContactInput ordered;
    ordered.admitted_pair_declared=true;ordered.receiver_model=10;ordered.receiver_slot=ordered.receiver_credit_slot=1;
    ordered.receiver_descriptor=9;ordered.receiver_hit_row=0;ordered.beam_token26c=1;
    GRedBeamSasukeContactPrefix contact;
    check(normal_gred_beam_sasuke_contact_prefix(red,beam,ordered,&contact),"exact contact candidate");

    GRedBeamSasukeHealthInput in;
    in.reviewed_ordinary=true;in.receiver_slot=1;in.beam_relative_c4=in.receiver_b4=1;
    in.current1c6=in.max1c4=180;in.previous1c8=29;
    GRedBeamSasukeHealthPlan plan;
    check(!normal_gred_beam_sasuke_health_plan(red,contact,in,&plan),"missing factors do not default to unity");
    in.factors={true,1,1,1,1,1,1,1,1,1};
    check(normal_gred_beam_sasuke_health_plan(red,contact,in,&plan)&&plan.amount==20&&plan.write_health&&
        plan.previous1c8==180&&plan.current1c6==160&&!plan.reaches_zero,"declared unit factors are only a fixture");
    // Resolved operands from the neutral source comparison, not defaults for
    // tables or an assertion that a live duel uses this scene snapshot.
    in.factors={true,1.02f,1,1.16f,.85f,.90f,1,.93f,1.15f,1};
    check(normal_gred_beam_sasuke_health_plan(red,contact,in,&plan)&&plan.amount==19&&
        plan.previous1c8==180&&plan.current1c6==161,"neutral source fixture does not spend the raw row amount");
    in.factors={true,1,1,1,1,1,1,1,1,1};
    in.beam_relative_c4=2;in.receiver_b4=3;in.receiver_behaviour5e0=0x04000000;
    check(normal_gred_beam_sasuke_health_plan(red,contact,in,&plan)&&plan.amount==7&&plan.current1c6==173,
        "beam adjustment, inverse body scale and half bit before truncation");
    in.current1c6=2;
    check(normal_gred_beam_sasuke_health_plan(red,contact,in,&plan)&&plan.reaches_zero&&plan.current1c6==0&&
        plan.previous1c8==2,"lethal proposed subtraction clamps zero");
    in.current1c6=0;
    check(normal_gred_beam_sasuke_health_plan(red,contact,in,&plan)&&!plan.write_health&&!plan.reaches_zero&&
        plan.previous1c8==29,"already-zero HP does not produce a new zero crossing");
    in.scene_hp_hold1f=true;in.current1c6=180;
    check(normal_gred_beam_sasuke_health_plan(red,contact,in,&plan)&&plan.amount==7&&!plan.write_health&&
        plan.current1c6==180&&plan.previous1c8==29,"scene hold belongs to writer, not scalar amount");
    in.scene_hp_hold1f=false;in.receiver_mask5a0=4;in.factors={};
    check(normal_gred_beam_sasuke_health_plan(red,contact,in,&plan)&&!plan.amount&&plan.write_health&&
        plan.current1c6==180&&plan.previous1c8==180,"masked body selects zero before the table read");
    in.receiver_mask5a0=0;in.receiver_flags1da=2;
    check(normal_gred_beam_sasuke_health_plan(red,contact,in,&plan)&&!plan.amount&&plan.write_health,
        "ordinary special receiver flag bypasses the amount helper");
    in.receiver_slot=2;in.receiver_flags1da=0;
    check(!normal_gred_beam_sasuke_health_plan(red,contact,in,&plan)&&plan.current1c6==180,"foreign receiver refused");
    std::puts("normal G-Red beam/Sasuke health: detached amount and separate HP writer PASS");
}
