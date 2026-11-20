#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_world_visit.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red beam world visit: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    constexpr NormalGRedNeutralBSource red{0x0615,0,4};
    GRedNeutralBReleasePlan paid;paid.request_beam=true;paid.owner_slot=4;paid.object_type=2;paid.argument=43;
    GRedBeamFactoryPlan beam;
    check(normal_gred_beam_factory_plan(red,paid,true,GRedBeamPoolSample::Allocated,&beam),"declared factory fixture");
    GRedBeamWorldVisitInput in;
    in.source_pass_declared=true;in.actor_active00=255;in.carrier_active00=128;
    in.position20_bits={0x80000000,0x40900000,0xc0440000};
    in.angle70_bits={0xffff,0x100,0x8000};
    using R=GRedBeamWorldRoute;
    GRedBeamWorldVisitPlan p;
    in.scene_mask_d1=0x81;
    check(normal_gred_beam_world_visit(red,beam,in,&p)&&p.route==R::Dispatch&&p.visit_shape_group&&
        p.credit_slot==4&&p.carrier_group==13&&p.previous2c_bits==in.position20_bits&&p.previous76_bits==in.angle70_bits,
        "nonmatching D1 bits do not mask the slot; history copies exact words");
    check(normal_gred_beam_world_clears_activation(p,255)&&!normal_gred_beam_world_clears_activation(p,0),
        "post-callback 86 alone selects the reset for a dispatched carrier");
    in.scene_mask_d1=0x10;
    check(normal_gred_beam_world_visit(red,beam,in,&p)&&p.route==R::GroupMasked&&p.visit_shape_group&&
        p.previous2c_bits!=in.position20_bits&&!normal_gred_beam_world_clears_activation(p,1),
        "object mask does not suppress the shape bucket or take a history copy");
    in.scene_mask_d1=0;in.carrier_active00=0;
    check(normal_gred_beam_world_visit(red,beam,in,&p)&&p.route==R::CarrierInactive&&p.visit_shape_group,
        "inactive carrier does not mean its collider bucket is empty");
    in.actor_active00=0;
    check(normal_gred_beam_world_visit(red,beam,in,&p)&&p.route==R::SlotInactive&&!p.visit_shape_group,
        "both slot-biased walks skip inactive actors");
    in.actor_active00=1;in.carrier_active00=1;in.scene_hold66=128;
    check(normal_gred_beam_world_visit(red,beam,in,&p)&&p.route==R::SceneHeld&&!p.visit_shape_group,
        "scene hold skips both walks for this entry");
    beam.call.resolved_group=8;
    check(!normal_gred_beam_world_visit(red,beam,in,&p)&&p.route==R::SceneHeld,
        "no fallback to unowned group or output on refusal");
    std::puts("normal G-Red beam world visit: distinct carrier/collider gates and history PASS");
}
