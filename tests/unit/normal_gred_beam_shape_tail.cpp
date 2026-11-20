#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/borgs/normal_gred_beam_shape_tail.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red beam shape tail: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    constexpr NormalGRedNeutralBSource source{0x0615,0,3};
    constexpr GRedBeamShapeOperands shape{{4,48,52,26,0,false},0,15,0,3,true,0,-2,20};
    constexpr ColliderGeometryProfile geometry{{0,0x3f800000u,0x42480000u,0x3c8efa35u,0x42652ee1u,
        0x3fc90fdbu,0x3f060a92u,0x3c23d70au},true};
    GRedBeamShapeTailInputs in;
    in.after_shape0_init=in.null_copied_target_cc=true;
    in.carrier.lifecycle_declared=true;in.carrier.registration={true,true,1,16};
    in.credit={true,true,0,0};
    in.activation={65535,21,0xffffffffu,0,90};
    GRedBeamShapeTailPlan plan;
    std::string error;
    constexpr auto list=CollisionRegistrationList::L477C;
    check(normal_gred_beam_shape_tail(source,shape,in,geometry,{},&plan,&error)&&plan.activation_write&&
        plan.activation==GRedBeamActivationFields{1,1,0,1,0}&&plan.phase10==1&&
        plan.registration_sampled&&plan.membership.contains(list)&&!plan.hit_index11&&!plan.interval4e,
        "nonzero token wrap, shot credit count wrap and exact list sample");
    in.activation=plan.activation;
    in.carrier.freeze281=1;in.carrier.registration.disabled276=1;
    check(normal_gred_beam_shape_tail(source,shape,in,geometry,{},&plan,&error)&&!plan.activation_write&&
        plan.activation==in.activation&&plan.phase10==2&&plan.registration_sampled&&!plan.membership.lists,
        "once guard preserves bookkeeping while the freeze and list gates stay separate");
    in.activation={9,11,23,0,17};
    auto flat=Mat4{};flat.at(2,2)=0;
    in.credit.timer720=std::numeric_limits<f32>::quiet_NaN();
    check(normal_gred_beam_shape_tail(source,shape,in,geometry,flat,&plan,&error)&&plan.activation_write&&
        plan.degenerate&&!plan.registration_sampled&&plan.activation==GRedBeamActivationFields{10,10,24,1,0},
        "degenerate basis still has earlier effect writes but must not sample registration");
    check(!normal_gred_beam_shape_tail(source,shape,in,geometry,{},&plan,&error)&&plan.degenerate&&
        plan.activation.credit_count404==24,"unavailable registration timer leaves whole output untouched");

    in.credit.timer720=0;in.carrier.retire1d8=1;
    check(!normal_gred_beam_shape_tail(source,shape,in,geometry,{},&plan,&error),"retirement is outside fresh tail");
    in.carrier.retire1d8=0;in.null_copied_target_cc=false;
    check(!normal_gred_beam_shape_tail(source,shape,in,geometry,{},&plan,&error),"targeted bookkeeping is not this leaf");
    in.null_copied_target_cc=true;in.after_shape0_init=false;
    check(!normal_gred_beam_shape_tail(source,shape,in,geometry,{},&plan,&error),"prepared operands cannot stand in for the shape visit");
    std::puts("normal G-Red beam shape tail: detached activation, frame gate and membership PASS");
}
