#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_patra_nurse_support.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Patra/Nurse support: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    const SupportCasterObservation patra{0x0902,0,2,1},nurse{0x0900,0,3,1};
    PatraSpellRelease release;
    check(normal_patra_spell_release(patra,1,0,true,&release)&&!release.clear_program_events&&!release.debit.amount,
        "one program byte alone cannot release Big Spell");
    check(normal_patra_spell_release(patra,1,1,true,&release)&&release.clear_program_events&&
        release.debit.channel==2&&release.debit.amount==1,"exact Patra release requests one debit");
    check(!normal_patra_spell_release(patra,1,1,false,&release)&&
        !normal_patra_spell_release(nurse,1,1,true,&release)&&release.clear_program_events,"wrong class or unreached event is not a cast");
    WeaponStockModeZeroPlan stock;
    PatraSpellCastRequest cast;
    check(weapon_stock_mode_zero_plan({2,1,180,0},WeaponStockOperation::CheckOnly,1,&stock)&&
        !normal_patra_spell_cast(patra,stock,&cast)&&!cast.requested,"stock availability is not a release debit");
    check(weapon_stock_mode_zero_plan({2,1,180,0},WeaponStockOperation::Debit,release.debit.amount,&stock)&&
        normal_patra_spell_cast(patra,stock,&cast)&&cast==PatraSpellCastRequest{true,2,60,0},"cast request retains caster ownership");
    check(weapon_stock_mode_zero_plan({0,1,180,30},WeaponStockOperation::Debit,1,&stock)&&
        normal_patra_spell_cast(patra,stock,&cast)&&!cast.requested,"failed debit creates no cast");

    SupportContactRecipient contact{4,1,0,true};
    PatraRecipientEffectRequest effect;
    check(normal_patra_recipient_effect(patra,60,5,contact,&effect)&&effect==PatraRecipientEffectRequest{4,33,1,1},
        "later contact requests effect on actual recipient, not the caster");
    const auto prior_effect=effect;
    contact.resolved_reviewed_contact=false;
    check(!normal_patra_recipient_effect(patra,60,5,contact,&effect)&&effect==prior_effect,"no effect from a target hint");
    contact.resolved_reviewed_contact=true;
    check(!normal_patra_recipient_effect(patra,60,3,contact,&effect)&&effect==prior_effect,"Nurse hit row cannot request Patra effect");

    NurseFallbackSetupPlan setup;
    check(normal_nurse_fallback_setup(nurse,true,&setup)&&setup.program_group==4&&!setup.program&&
        setup.debit_after_request.channel==2&&setup.debit_after_request.amount==1,"Nurse's fallback requests program and setup debit");
    NurseSetupStockPlan setup_stock;
    check(normal_nurse_setup_stock(nurse,stock,&setup_stock)&&!setup_stock.stock_latch548&&
        setup_stock.write_failure6e8&&setup_stock.failure6e8==8,"failed setup debit writes the exact latch and failure prefix");
    check(weapon_stock_mode_zero_plan({1,0,180,0},WeaponStockOperation::Debit,1,&stock)&&
        normal_nurse_setup_stock(nurse,stock,&setup_stock)&&setup_stock.stock_latch548==1&&!setup_stock.write_failure6e8,
        "success does not invent a failure-field write");
    NurseContactShapeRequest shape;
    check(normal_nurse_fallback_shape(nurse,true,2,&shape)&&!shape.requested,"other pose lane is not the healing opcode lane");
    check(normal_nurse_fallback_shape(nurse,true,3,&shape)&&shape.requested&&shape.caster_slot==3&&shape.shape_group==3,
        "Nurse-owned shape request is separate from stock and HP effects");
    check(!normal_nurse_fallback_shape(nurse,false,1,&shape)&&shape.requested,"visual event cannot substitute the opcode");

    NurseHealingPlan heal;
    check(normal_nurse_healing_contact(nurse,3,contact,{10,250,true},0,&heal)&&heal==NurseHealingPlan{true,4,210,10},
        "actual ally receives capped amount and keeps previous HP");
    check(normal_nurse_healing_contact(nurse,3,contact,{140,250,true},0,&heal)&&heal==NurseHealingPlan{true,4,250,140},
        "healing clamps to this recipient's own maximum");
    check(normal_nurse_healing_contact(nurse,3,contact,{250,250,true},0,&heal)&&heal==NurseHealingPlan{true,4,250,250},
        "full HP still has the ordinary source write");
    check(normal_nurse_healing_contact(nurse,3,contact,{0,250,true},0,&heal)&&!heal.write_hp,
        "zero HP is not revived");
    check(normal_nurse_healing_contact(nurse,3,contact,{1,250,true},1,&heal)&&!heal.write_hp,
        "the ordinary mode gate can still block a resolved contact");
    check(!normal_nurse_healing_contact(nurse,3,contact,{140,250,false},0,&heal)&&
        !normal_nurse_healing_contact(nurse,3,contact,{32700,32767,true},0,&heal)&&!heal.write_hp,
        "fused recipients and unreviewed signed overflow refuse");
    contact.team=2;
    check(!normal_nurse_healing_contact(nurse,3,contact,{140,250,true},0,&heal)&&!heal.write_hp,
        "this branch does not invent cross-team healing");
    std::printf("normal Patra/Nurse support stock and contact recipients: %u checks PASS\n",checks);
}
