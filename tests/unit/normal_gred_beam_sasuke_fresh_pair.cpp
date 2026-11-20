#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_fresh_pair.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red/Sasuke spheres: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    using R=GRedSasukeFreshPairRoute;
    using V=std::array<f32,3>;
    constexpr NormalGRedNeutralBSource red{0x0615,0,0};
    constexpr GRedBeamShapeOperands beam{{4,48,49,26,0,false},0,15,0,0,true,0,-2,20};
    GRedSasukeFreshPairInput in;
    in.opposing_direct_scope_declared=in.lists_declared=true;
    auto& c=in.contact_fields;
    c.receiver_model=10;c.receiver_slot=c.receiver_credit_slot=1;
    c.receiver_descriptor=9;c.receiver_hit_row=0;c.beam_token26c=29;c.receiver_index26e=1;
    GRedSasukeFreshPairPlan plan;
    check(normal_gred_beam_sasuke_fresh_pair(red,beam,in,&plan)&&plan.route==R::NotListed&&!plan.narrow_sampled,
        "an empty declared list is not an admitted pair");
    in.attack_list.lists=1u<<unsigned(CollisionRegistrationList::L477C);
    in.receiver_list.lists=1u<<unsigned(CollisionRegistrationList::L417C);
    in.receiver_flags1da=2;c.receiver_tokens20c[47]=29;
    check(normal_gred_beam_sasuke_fresh_pair(red,beam,in,&plan)&&plan.route==R::ReceiverHeld,"outer receiver flag before pair gate");
    in.receiver_flags1da=0;in.receiver_suppressed272=2;
    check(normal_gred_beam_sasuke_fresh_pair(red,beam,in,&plan)&&plan.route==R::RepeatToken,
        "all 48 tokens are read even beyond the write cursor; token gate precedes suppression");
    c.receiver_tokens20c[47]=0;
    check(normal_gred_beam_sasuke_fresh_pair(red,beam,in,&plan)&&plan.route==R::Suppressed,"receiver mode-2 suppression");
    in.receiver_suppressed272=0;
    check(!normal_gred_beam_sasuke_fresh_pair(red,beam,in,&plan)&&plan.route==R::Suppressed,
        "missing sphere records cannot be a no-hit");
    in.beam={true,1,0,{42.25f,0,0},20,20};in.body={true,1,0,{},22,22};
    check(normal_gred_beam_sasuke_fresh_pair(red,beam,in,&plan)&&plan.route==R::BroadMiss&&!plan.narrow_sampled,
        "separation beyond the radii");
    in.beam.center64[0]=42;
    check(normal_gred_beam_sasuke_fresh_pair(red,beam,in,&plan)&&plan.route==R::Overlap&&plan.narrow_sampled&&
        plan.point==V{22,0,0}&&plan.prefix.receiver_tokens20c[1]==29&&plan.prefix.beam_receiver290==1,
        "inclusive sphere tangency stages the actual receiver");
    in.beam.center64=in.body.center64={2,3,4};
    check(normal_gred_beam_sasuke_fresh_pair(red,beam,in,&plan)&&plan.route==R::Overlap&&plan.point==V{2,3,4},
        "coincident spheres use the receiving center without normalization");
    in.beam.radius88=in.beam.broad_radius7c=.1f;
    check(normal_gred_beam_sasuke_fresh_pair(red,beam,in,&plan)&&plan.route==R::BroadMiss,
        "39F6C rejects a broad radius exactly on the floor");
    in.beam.radius88=in.beam.broad_radius7c=20;in.beam.effective_kind58=2;
    check(!normal_gred_beam_sasuke_fresh_pair(red,beam,in,&plan)&&plan.route==R::BroadMiss,
        "moving shape 2 is not silently a sphere");
    std::puts("normal G-Red/Sasuke fresh spheres: filters, inclusive overlap and shape refusal PASS");
}
