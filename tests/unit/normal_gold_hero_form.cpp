#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "engine/gameplay/borgs/normal_gold_hero_form.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Gold Hero form: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using S=NormalGoldHeroSource;
    using A=GoldHeroFormAction;
    constexpr S base{S::base,0},form{S::transformed,0};
    GoldHeroFormTraits traits;
    check(normal_gold_hero_form_traits(base,&traits)&&traits==GoldHeroFormTraits{0,1,2},"base data, pose default and count limit");
    check(normal_gold_hero_form_traits(form,&traits)&&traits==GoldHeroFormTraits{432,0,4},"same retail variant with changed form data");
    check(!normal_gold_hero_form_traits({S::transformed,1},&traits)&&traits==GoldHeroFormTraits{432,0,4},
        "variant not interchangeable with form");

    GoldHeroSetupDebit request;
    check(normal_gold_hero_setup_debit(base,0,0,2,true,&request)&&!request.amount,"held is not a fresh press");
    check(normal_gold_hero_setup_debit(base,0x400,0,0,true,&request)&&!request.amount,"empty stock produces no request");
    for(u32 bit:{1u,2u,0x1000u,0x1000000u,0x10000000u,0x80000000u})
        check(normal_gold_hero_setup_debit(base,0x400,bit,2,true,&request)&&!request.amount,"action guard blocks entry");
    check(normal_gold_hero_setup_debit(base,0x400,0x40,2,true,&request)&&request.channel==2&&request.amount==1&&request.stock_before==2,
        "fresh unlocked press requests one channel-2 debit, not a form swap");
    check(!normal_gold_hero_setup_debit(base,0x400,0,2,false,&request)&&request.stock_before==2,
        "unreached hook cannot request stock");
    check(!normal_gold_hero_setup_debit(form,0x400,0,2,true,&request)&&request.stock_before==2,
        "this entry leaf is base-only, not the transformed X action");
    A action=A::None;
    check(!normal_gold_hero_setup_action(base,request,{2,1,2,false,true},&action)&&action==A::None,"no declared debit observation");
    check(!normal_gold_hero_setup_action(base,request,{3,2,2,true,true},&action)&&action==A::None,"wrong starting stock is stale");
    check(!normal_gold_hero_setup_action(base,request,{2,2,2,true,true},&action)&&action==A::None,"accepted observation must debit one");
    check(!normal_gold_hero_setup_action(base,request,{2,1,1,true,true},&action)&&action==A::None,"wrong stock channel refuses");
    check(!normal_gold_hero_setup_action(base,request,{2,1,2,true,false},&action)&&action==A::None,"declined debit cannot mutate stock");
    check(normal_gold_hero_setup_action(base,request,{2,2,2,true,false},&action)&&action==A::None,"declined stock queues nothing");
    check(normal_gold_hero_setup_action(base,request,{2,1,2,true,true},&action)&&action==A::SetUp,"observed one-charge debit queues setup");

    GoldHeroRebindPlan rebind;
    check(normal_gold_hero_rebind(base,action,0,7,&rebind)&&!rebind.binding.requested,"setup without lower completion cannot change model");
    check(normal_gold_hero_rebind(base,action,1,7,&rebind)&&rebind.binding==ModelFormRebindPrefix{true,S::transformed,0,8}&&
        rebind.target_traits==GoldHeroFormTraits{432,0,4},
        "completed lower lane requests in-place resource rebind");
    const auto entered=rebind;
    check(!normal_gold_hero_rebind(base,A::Return,1,7,&rebind)&&rebind==entered,"return cannot be invoked from base");
    check(!normal_gold_hero_rebind({S::base,1},A::SetUp,1,7,&rebind)&&rebind==entered,"edited or alternate bindings cannot borrow the plan");
    check(normal_gold_hero_rebind(base,A::SetUp,128,255,&rebind)&&rebind.binding==ModelFormRebindPrefix{true,S::transformed,0,0},
        "any nonzero observed completion qualifies this leaf; raw byte is not an actor generation");

    GoldHeroExpiryStep expiry;
    check(normal_gold_hero_expiry_step(form,0,0,&expiry)&&expiry==GoldHeroExpiryStep{0,A::None},"ordinary zero does not retry");
    check(normal_gold_hero_expiry_step(form,1,0,&expiry)&&expiry==GoldHeroExpiryStep{0,A::Return},"last unblocked tick can queue immediately");
    check(normal_gold_hero_expiry_step(form,1,0x1000,&expiry)&&expiry==GoldHeroExpiryStep{-1,A::None},"blocked last tick defers");
    check(normal_gold_hero_expiry_step(form,expiry.ticks144,1,&expiry)&&expiry==GoldHeroExpiryStep{-1,A::None},"blocked retry preserves sentinel");
    check(normal_gold_hero_expiry_step(form,expiry.ticks144,0,&expiry)&&expiry==GoldHeroExpiryStep{0,A::Return},"unblocked retry queues return");
    check(normal_gold_hero_rebind(form,expiry.action,0,33,&rebind)&&rebind.binding==ModelFormRebindPrefix{true,S::base,0,34}&&
        rebind.target_traits==GoldHeroFormTraits{0,1,2},
        "accepted return leaf does not use the entry's lower-completion gate");
    check(!normal_gold_hero_expiry_step(base,1,0,&expiry)&&!normal_gold_hero_expiry_step(form,-2,0,&expiry)&&
        expiry==GoldHeroExpiryStep{0,A::Return},"wrong form or unreviewed negative counter cannot publish");
    s32 ticks=S::initial_form_ticks;
    while(ticks>1){
        check(normal_gold_hero_expiry_step(form,ticks,0,&expiry)&&expiry.ticks144==ticks-1&&expiry.action==A::None,
            "each explicitly qualified callback decrements once before expiry");
        ticks=expiry.ticks144;
    }
    std::printf("normal Gold Hero: stock-gated request, rebind boundaries, and guarded expiry: %u checks PASS\n",checks);
}
