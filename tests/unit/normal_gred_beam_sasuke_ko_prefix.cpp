#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_ko_prefix.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red/Sasuke KO prefix: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    constexpr NormalGRedNeutralBSource red{0x0615,0,3};
    GRedBeamSasukePostHitPlan hit;
    hit.credit_slot=3;hit.receiver_slot=1;hit.health={19,0,7,true,true};
    GRedBeamSasukeKoInput in;
    in.direct_mode_zero_declared=true;in.credit_slot=3;in.receiver_slot=1;in.credit_team88=0;in.receiver_team88=1;in.context32=0;
    in.receiver_cost4aa=210;in.receiver_team_remaining118=450;
    in.debit108=1;in.scene_credit_b4=2;in.scene_receiver_b8=5;
    in.receiver_losses435=in.credit_kos434=255;in.receiver_loss_cost424=0xffffffffu-209;
    in.credit_ko_cost420=17;in.credit_team_bonus=65500;
    GRedBeamSasukeKoPlan plan;
    check(!normal_gred_beam_sasuke_ko_prefix(red,hit,in,&plan),"zero value without the KO continuation cannot start the ledger");
    hit.request_ko_continuation=true;
    check(normal_gred_beam_sasuke_ko_prefix(red,hit,in,&plan)&&plan.team_debited&&!plan.team_zero_store&&
        plan.receiver_team_remaining118==240&&plan.receiver_loss_cost424==0&&plan.credit_ko_cost420==227&&
        plan.receiver_losses435==0&&plan.credit_kos434==0&&plan.credit_team_bonus==64,
        "runtime cost, not HP or damage amount; source-width counters wrap");
    check(!plan.scene_pair_requested&&plan.scene_credit_b4==2&&plan.scene_receiver_b8==5&&plan.observer_scan_pending,
        "nonterminal ledger and no reserve do not publish a last pair");
    in.receiver_team_remaining118=210;
    check(normal_gred_beam_sasuke_ko_prefix(red,hit,in,&plan)&&plan.receiver_team_remaining118==0&&plan.team_zero_store&&
        plan.scene_pair_requested&&plan.scene_credit_b4==3&&plan.scene_receiver_b8==1&&!plan.reserve_decremented,
        "equal remaining cost requests the exact scene credit and receiver");
    in.receiver_team_remaining118=120;in.reserves10a=1;in.reserve_enabled109=in.receiver_flag1ea=255;
    check(normal_gred_beam_sasuke_ko_prefix(red,hit,in,&plan)&&plan.receiver_team_remaining118==0&&plan.team_zero_store&&
        plan.reserves10a==0&&plan.reserve_decremented&&plan.scene_pair_requested,"over-cost clamp and reserve gate are separate");
    in.debit108=0;in.reserves10a=2;
    check(normal_gred_beam_sasuke_ko_prefix(red,hit,in,&plan)&&!plan.team_debited&&plan.receiver_team_remaining118==120&&
        plan.reserves10a==1&&plan.reserve_decremented&&plan.scene_pair_requested,"reserve path can name pair without ending budget");
    in.reserves10a=-1;
    check(normal_gred_beam_sasuke_ko_prefix(red,hit,in,&plan)&&!plan.reserve_decremented&&!plan.scene_pair_requested&&
        plan.reserves10a==-1,"signed negative reserve is not a huge count");

    in.receiver_slot=2;
    check(!normal_gred_beam_sasuke_ko_prefix(red,hit,in,&plan)&&plan.reserves10a==-1,"no retarget on another slot");
    in.receiver_slot=1;in.receiver_link4a0=1;
    check(!normal_gred_beam_sasuke_ko_prefix(red,hit,in,&plan),"linked receivers are not the reviewed direct KO");
    in.receiver_link4a0=0;in.scene_hold1f=1;
    check(!normal_gred_beam_sasuke_ko_prefix(red,hit,in,&plan),"held scene cannot reuse old proposed KO");
    std::puts("normal G-Red/Sasuke KO prefix: detached ledger, reserve and credit PASS");
}
