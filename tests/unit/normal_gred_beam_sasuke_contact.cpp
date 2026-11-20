#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_contact.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_flight.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"normal G-Red beam/Sasuke contact: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    constexpr NormalGRedNeutralBSource red{0x0615,0,3};
    constexpr GRedBeamShapeOperands beam{{4,48,52,26,0,false},0,15,0,3,true,0,-2,20};
    GRedBeamSasukeContactInput in;
    in.receiver_model=0x000a;in.receiver_slot=in.receiver_credit_slot=1;in.receiver_descriptor=9;in.receiver_hit_row=0;
    in.beam_token26c=65535;in.contact_vector={1,4,-2};in.receiver_index26e=47;
    in.receiver_tokens20c.fill(5);in.receiver_flags6fd=0x80;in.credit_latch1d9=2;
    GRedBeamSasukeContactPrefix plan;
    check(!normal_gred_beam_sasuke_contact_prefix(red,beam,in,&plan),"prepared geometry is not an admitted ordered pair");
    in.admitted_pair_declared=true;
    check(normal_gred_beam_sasuke_contact_prefix(red,beam,in,&plan)&&plan.beam_receiver290==1&&plan.credit_receiver290==1&&
        plan.beam_latch1d9==0x10&&plan.credit_latch1d9==2&&plan.beam_vector2a0==in.contact_vector&&
        plan.receiver_vector2a0==in.contact_vector,"actual declared receiver; latch stays on the carrier by default");
    check(plan.receiver_tokens20c[47]==65535&&plan.receiver_index26e==0&&plan.receiver_tokens20c[0]==5&&
        plan.receiver_flags6fd==0x81&&plan.receiver_rank71c==1&&plan.receiver_impact704==0&&
        plan.receiver_reaction702==4&&plan.receiver_binding71a==2,"48-entry token write wraps; exact row and body fields");

    GRedBeamActivePlan active;
    check(normal_gred_beam_active_plan(red,{299,plan.beam_latch1d9,GRedBeamStopSample::Clear},&active)&&
        active.route==GRedBeamActiveRoute::AwaitStage,"ordinary receiver latch is not phase-one retirement by itself");
    in.beam_flags2ac=1;in.beam_latch1d9=0x20;in.receiver_rank71c=7;in.receiver_index26e=0;
    check(normal_gred_beam_sasuke_contact_prefix(red,beam,in,&plan)&&plan.beam_latch1d9==0x30&&
        plan.credit_latch1d9==0x32&&plan.receiver_rank71c==7&&plan.receiver_tokens20c[0]==65535&&
        plan.receiver_tokens20c[47]==5&&plan.receiver_index26e==1,"opt-in propagation and rank maximum");
    in.receiver_index26e=48;in.receiver_slot=in.receiver_credit_slot=2;
    check(!normal_gred_beam_sasuke_contact_prefix(red,beam,in,&plan)&&plan.beam_receiver290==1&&plan.receiver_index26e==1,
        "invalid ring cursor refuses without repeating writes");
    in.receiver_index26e=1;in.receiver_credit_slot=1;
    check(!normal_gred_beam_sasuke_contact_prefix(red,beam,in,&plan),"foreign receiver lineage refused");
    in.receiver_credit_slot=2;in.receiver_descriptor=8;
    check(!normal_gred_beam_sasuke_contact_prefix(red,beam,in,&plan),"another receiving descriptor is not this leaf");
    std::puts("normal G-Red beam/Sasuke contact: distinct receiver, latch and history proposal PASS");
}
