#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_swept_pair.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red/Sasuke moving pair: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    using R=GRedSasukeSweptRoute;
    using V=std::array<f32,3>;
    constexpr NormalGRedNeutralBSource red{0x0615,0,0};
    constexpr GRedBeamShapeOperands beam{{4,48,49,26,0,false},0,15,0,0,true,0,-2,20};
    GRedSasukeSweptPairInput in;
    in.opposing_direct_scope_declared=in.lists_declared=true;
    in.attack_list.lists=1u<<unsigned(CollisionRegistrationList::L477C);
    in.receiver_list.lists=1u<<unsigned(CollisionRegistrationList::L417C);
    auto& c=in.contact_fields;
    c.receiver_model=10;c.receiver_slot=c.receiver_credit_slot=1;c.receiver_descriptor=9;c.receiver_hit_row=0;c.beam_token26c=29;
    in.body={true,1,0,{},22,22};
    auto sweep=[&](V from,V to,f32 c0=1){
        in.beam={true,from,from,mat4_translation(to.data()),c0};
    };
    GRedBeamSweptShape shape;
    sweep({3,0,0},{3.5f,0,0});
    check(!normal_gred_beam_swept_shape(red,beam,in.beam,&shape),"travel exactly .5 remains outside the moving update");
    sweep({-60,0,0},{60,0,0},1.25f);in.beam.prior8c={-70,0,0};
    check(normal_gred_beam_swept_shape(red,beam,in.beam,&shape)&&shape.previous_d4==V{-70,0,0}&&
        shape.previous_e0==V{-60,0,0}&&shape.start8c==V{-60,0,0}&&shape.end98==V{60,0,0}&&shape.center64==V{}&&
        shape.radius88==25&&shape.broad_radius7c==85&&shape.unit_bc==V{1,0,0}&&shape.reverse_unit_c8==V{-1,0,0},
        "two endpoints, midpoint, explicit physical scale and persistent history");

    GRedSasukeSweptPairPlan plan;
    sweep({-60,0,0},{0,0,0});
    check(normal_gred_beam_sasuke_swept_pair(red,beam,in,&plan)&&plan.route==R::RayCrossing&&plan.ray_mask==2&&
        plan.point==V{-22,0,0},"one on-segment root goes directly to the geometric candidate");
    sweep({-60,0,0},{60,0,0});
    check(normal_gred_beam_sasuke_swept_pair(red,beam,in,&plan)&&plan.route==R::RadiusFallback&&
        plan.ray_mask==0&&plan.point==V{},"both roots on segment take projection fallback");
    sweep({-60,41,0},{60,41,0});
    check(normal_gred_beam_sasuke_swept_pair(red,beam,in,&plan)&&plan.route==R::RadiusFallback&&
        plan.ray_mask==255&&plan.point==V{0,22,0},"no ray root can still pass the strict summed-radius gate");
    sweep({-60,42,0},{60,42,0});
    check(normal_gred_beam_sasuke_swept_pair(red,beam,in,&plan)&&plan.route==R::NarrowMiss,
        "moving fallback tangency misses even when broad phase passes");
    in.contact_fields.receiver_tokens20c[47]=29;in.beam.prior_phase_one_declared=false;
    check(normal_gred_beam_sasuke_swept_pair(red,beam,in,&plan)&&plan.gate==GRedSasukePairGate::RepeatToken&&
        plan.route==R::NotQueried,"shared token gate prevents querying unavailable sweep");
    std::puts("normal G-Red/Sasuke moving pair: detached shape, ray and fallback PASS");
}
