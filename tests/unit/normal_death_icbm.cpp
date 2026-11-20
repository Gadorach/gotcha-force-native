#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_death_icbm.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Death ICBM game controller: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using P=IcbmImpactProbeResult;
    using Q=IcbmImpactQuery;
    constexpr NormalDeathIcbmSource source{0x0d06,0,3};
    CharacterAirResourceState fuel{25,0,3};
    check(normal_death_icbm_preselect_refill(source,fuel,0x40,&fuel)&&fuel.value736==75,
        "nonboost flight can refill without a grounded state");
    CharacterMovementInputState input;
    Type3BoostRequestFields fields{27,0,0x40,true};
    u8 selector=255;
    check(normal_death_icbm_boost_request(source,input,fields,fuel,&selector)&&selector==27,"ordinary cruise takes no held-only boost");
    input.commands5d4=2;
    check(normal_death_icbm_boost_request(source,input,fields,fuel,&selector)&&selector==28,"conditioned type-3 boost request");
    Type3BoostDebit debit;
    check(normal_death_icbm_boost_debit(source,fuel,0x40,1,&debit)&&debit.flags5e0==0xc0&&debit.resource.value736==25&&
        debit.resource.delay73a==16,"shared fuel debit and inhibit flag");
    check(!normal_death_icbm_boost_debit({0x0d07,0,3},fuel,0x40,1,&debit)&&debit.resource.value736==25,
        "Death Bomb cannot inherit this flight controller");

    IcbmGameImpactInput impact{true,0,1,2,0,0,0,P::Unresolved,P::Unresolved,true,{9,18,-5}};
    IcbmGameImpactPlan plan;
    check(normal_death_icbm_game_impact(source,impact,&plan)&&plan.next_query==Q::Floor&&!plan.request_area_object,
        "unresolved floor is not a hit or a clear scene");
    impact.floor=P::Clear;
    check(normal_death_icbm_game_impact(source,impact,&plan)&&plan.next_query==Q::Scene&&!plan.request_area_object,
        "clear floor places the separate scene query next");
    impact.scene=P::Clear;
    check(normal_death_icbm_game_impact(source,impact,&plan)&&plan==IcbmGameImpactPlan{},
        "no eligible contact, no impact at zero hold");
    impact.floor=P::Contact;impact.scene=P::Unresolved;
    check(normal_death_icbm_game_impact(source,impact,&plan)&&plan.next_query==Q::None&&plan.request_area_object&&
        plan.write_pending_terminal&&plan.owner_slot==3&&plan.object_type==73&&plan.argument==0&&plan.hp1c6==1&&
        plan.pending663==1&&plan.mask272==255&&plan.request_position==impact.matrix_translation,
        "eligible contact uses bound matrix translation and queues terminal separately");
    const auto accepted=plan;
    impact.observed_matrix_translation=false;
    check(!normal_death_icbm_game_impact(source,impact,&plan)&&plan==accepted,"no fabricated effect origin");
    impact.observed_matrix_translation=true;
    impact.role493=1;
    check(normal_death_icbm_game_impact(source,impact,&plan)&&plan==IcbmGameImpactPlan{},"alternate setup blocks ordinary impact");
    impact.role493=0;impact.entry_hold720=1;
    check(normal_death_icbm_game_impact(source,impact,&plan)&&!plan.request_area_object,"positive hold blocks even a contact");
    impact.entry_hold720=0;impact.lifecycle18=2;
    check(normal_death_icbm_game_impact(source,impact,&plan)&&!plan.request_area_object,"already committed defeat blocks impact");
    impact.lifecycle18=1;impact.floor=impact.scene=P::Unresolved;impact.contact1d9=1;
    check(normal_death_icbm_game_impact(source,impact,&plan)&&plan==accepted,"observed per-pass latch precedes both queries");
    check(!normal_death_icbm_game_impact({0x0d06,1,3},impact,&plan)&&plan==accepted,"alternate actor not admitted");

    IcbmGameDefeatPlan defeat;
    check(normal_death_icbm_consume_terminal(source,1,plan.pending663,0x80,true,&defeat)&&defeat==IcbmGameDefeatPlan{true,3,0x81,0},
        "terminal consumer can request self defeat without allocation receipt");
    check(normal_death_icbm_consume_terminal(source,2,plan.pending663,0x80,true,&defeat)&&!defeat.request_self_defeat,
        "subsequent nonactive lifecycle cannot repeat pending write");
    std::printf("normal Death ICBM bounded type-3 and game impact gates: %u checks PASS\n",checks);
}
