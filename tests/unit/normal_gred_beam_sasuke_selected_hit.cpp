#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_selected_hit.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red/Sasuke selected hit: %s\n",why);std::abort();}}
gf::GRedSasukeMovingReceiver receiver(gf::u8 slot,std::array<gf::f32,3> center,gf::f32 radius=22){
    using namespace gf;
    GRedSasukeMovingReceiver r;
    r.opposing_direct_scope_declared=r.lists_declared=true;
    r.attack_list.lists=1u<<unsigned(CollisionRegistrationList::L477C);
    r.receiver_list.lists=1u<<unsigned(CollisionRegistrationList::L417C);
    auto& c=r.contact_fields;
    c.receiver_model=10;c.receiver_slot=c.receiver_credit_slot=slot;c.receiver_descriptor=9;c.receiver_hit_row=0;c.beam_token26c=30;
    r.body={true,1,0,center,radius,radius};return r;
}
}
int main(){
    using namespace gf;
    using V=std::array<f32,3>;
    constexpr NormalGRedNeutralBSource red{0x0615,0,0};
    constexpr GRedBeamShapeOperands beam{{4,48,49,26,0,false},0,15,0,0,true,0,-2,20};
    GRedSasukeMovingScanInput in;
    in.complete_reviewed_list_declared=true;in.contact_update100=0x800c44a8;
    in.beam={true,{-60,0,0},{-60,0,0},{},1};
    in.carrier={{-61,0,0},{3,0,0},{80,0,0},80,64};
    check(normal_gred_beam_scale_seed(red,1,64,&in.stored_scale),"declared stored scales");

    GRedSasukeMovingSelection plan;
    std::array list{receiver(1,{}),receiver(2,{-12,0,0},1)};
    check(normal_gred_beam_sasuke_select_moving(red,beam,in,list,&plan)&&plan.selected&&plan.slot==2&&
        plan.source_distance_sq==48*48&&plan.point==V{-12,0,0},
        "nearest center wins even though another contact point is closer to the start");
    check(plan.end98==V{-12,0,0}&&plan.carrier.position20==V{-9,0,0}&&plan.center64==V{-36,0,0}&&
        plan.moved_to_contact&&plan.refresh114&&plan.carrier.distance180==64&&
        source_basis_detail::column(plan.frame114,3)==V{-9,0,0}&&plan.contact.beam_receiver290==2&&
        plan.contact.beam_vector2a0==plan.point,"displacement and callback precede the selected contact, not a distance refresh");

    list={receiver(1,{-14,4,0}),receiver(2,{-14,-4,0})};
    check(normal_gred_beam_sasuke_select_moving(red,beam,in,list,&plan)&&plan.selected&&plan.order==1&&plan.slot==2,
        "later ordered candidate replaces an exact distance tie");
    std::swap(list[0],list[1]);
    check(normal_gred_beam_sasuke_select_moving(red,beam,in,list,&plan)&&plan.selected&&plan.order==1&&plan.slot==1,
        "tie follows list order, not actor slot number");
    list[1].contact_fields.beam_token26c=999;
    check(!normal_gred_beam_sasuke_select_moving(red,beam,in,list,&plan)&&plan.slot==1,
        "later mixed beam snapshot refuses the complete proposal");

    const std::array zero{receiver(1,{22,0,0})};
    check(normal_gred_beam_sasuke_select_moving(red,beam,in,zero,&plan)&&plan.selected&&plan.point==V{}&&
        !plan.moved_to_contact&&!plan.refresh114&&std::equal(plan.frame114.m,plan.frame114.m+16,in.beam.beam114.m)&&
        plan.carrier.position20==V{3,0,0},
        "zero offset still stages contact but performs no matrix callback");
    const std::array far{receiver(1,{})};
    in.beam.prior8c=in.beam.prior98={-20000,0,0};
    check(normal_gred_beam_sasuke_select_moving(red,beam,in,far,&plan)&&plan.selected&&
        plan.source_distance_sq==400000000.f,"source initial nearest distance is inclusive");
    in.beam.prior8c=in.beam.prior98={-22000,0,0};
    check(normal_gred_beam_sasuke_select_moving(red,beam,in,far,&plan)&&!plan.selected&&!plan.refresh114,
        "overlap beyond source distance sentinel does not select");
    in.complete_reviewed_list_declared=false;
    check(!normal_gred_beam_sasuke_select_moving(red,beam,in,{},&plan),"no unknown-list shortcut");
    std::puts("normal G-Red/Sasuke selected hit: ordered nearest, displacement and prefix PASS");
}
