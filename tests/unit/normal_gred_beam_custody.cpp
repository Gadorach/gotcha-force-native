#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_custody.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"normal G-Red beam custody: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using R=GRedBeamPoolSample;
    using I=GRedBeamInitRoute;
    constexpr NormalGRedNeutralBSource red{0x0615,0,3};
    GRedNeutralBReleasePlan paid,empty;
    check(normal_gred_neutral_b_debit(red,{3,2,30,true},{1,1,180,0},&paid)&&
        normal_gred_neutral_b_debit(red,{3,2,30,true},{0,0,180,0},&empty),"separate debit cases");
    GRedBeamFactoryPlan factory;
    check(!normal_gred_beam_factory_plan(red,empty,true,R::Allocated,&factory),"no factory on failed debit");
    check(normal_gred_beam_factory_plan(red,paid,true,R::Unobserved,&factory)&&
        factory.call==GRedBeamPoolCall{3,8,12,16,1,false}&&!factory.propose_header,"class-3 head request with one descriptor");
    check(normal_gred_beam_factory_plan(red,paid,true,R::Refused,&factory)&&!factory.propose_header&&
        paid.stock==WeaponStockModeZeroState{0,1,180,180},"pool refusal does not undo last stock");
    GRedBeamInitPlan init;
    check(!normal_gred_beam_first_dispatch(red,factory,1,0x0615,R::Unobserved,&init),"no dispatch after refusal");
    check(normal_gred_beam_factory_plan(red,paid,true,R::Allocated,&factory)&&factory.propose_header&&
        factory.active00==1&&factory.type10==2&&factory.argument11==43&&factory.parent_slot==3&&factory.credit_slot==3&&
         factory.copied_model94==0x0615&&factory.update0c==0x800c3cdc&&factory.draw10c==0x800c4468,
         "header proposal only on allocated sample");
    GRedBeamFactoryParentFields parent{0x0615,5,2,0,0xa5,.75f,1.25f,1.5f,.25f,true};
    GRedBeamFactoryBirthFields birth;
    check(normal_gred_beam_factory_birth_fields(red,factory,parent,&birth)&&
          birth.copied.flags_d0==0xa5&&birth.copied.render_index96==5&&birth.copied.resource_index97==2&&
          birth.copied.scale_b8==.75f&&birth.copied.physical_c0==1.5f&&birth.copied.relative_c4==.25f&&
          birth.scale58==std::array<f32,3>{1.25f,1.25f,1.25f}&&birth.targets_c8_cc_null&&
          birth.parent8c_slot==3&&birth.credit90_slot==3,"exact direct constructor copies, with B4 alone filling the visual scale");
    parent.declared_direct_no_target=false;
    check(!normal_gred_beam_factory_birth_fields(red,factory,parent,&birth)&&birth.copied.flags_d0==0xa5,
          "undeclared target custody leaves the prior proposal intact");
    check(normal_gred_beam_first_dispatch(red,factory,2,0x0615,R::Unobserved,&init)&&
        init.route==I::RetireCredit&&!init.request_shape&&init.phase18==2&&!init.lifetime1c,"defeated credit before init");
    check(normal_gred_beam_first_dispatch(red,factory,1,0x000a,R::Unobserved,&init)&&
        init.route==I::RetireCredit&&!init.request_shape,"changed credit model before init");
    check(!normal_gred_beam_first_dispatch(red,factory,1,0x000a,R::Allocated,&init)&&
        init.route==I::RetireCredit,"cannot already have a shape in the before-init branch");
    check(normal_gred_beam_first_dispatch(red,factory,1,0x0615,R::Unobserved,&init)&&
        init.route==I::AwaitRegistration&&init.request_shape&&init.owner_slot==3&&init.phase18==1&&init.lifetime1c==300,
        "valid first dispatch seeds lifetime before shape request");
    GRedBeamShapeOperands shape;
    std::array<u8,7840> blank{};
    check(!normal_gred_beam_shape_operands(red,init,{},&shape)&&
        !normal_gred_beam_shape_operands(red,init,{blank.data(),blank.size()},&shape),"no shape from absent or foreign bank");
    check(normal_gred_beam_first_dispatch(red,factory,1,0x0615,R::Refused,&init)&&
        init.route==I::RetireRegistration&&init.phase18==2&&init.lifetime1c==300&&init.request_shape,
        "registration refusal retires without a muzzle read");
    check(normal_gred_beam_first_dispatch(red,factory,1,0x0615,R::Allocated,&init)&&
        init.route==I::MuzzleBranch&&init.phase18==1,"success is a muzzle branch, not a hit");
    std::puts("normal G-Red beam custody: detached factory, first dispatch and refusal PASS");
}
