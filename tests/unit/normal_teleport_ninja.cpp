#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>
#include "engine/gameplay/borgs/normal_teleport_ninja.hpp"
#include "engine/gameplay/weapon_stock_mode_zero.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Teleport Ninja: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    constexpr NormalTeleportNinjaSource source{5,0},switching{9,0};
    constexpr TeleportPosition position{3,4,5},history{3,2,8};
    TeleportSetupPlan setup;
    check(normal_teleport_history_setup(source,position,history,0x40,0,true,&setup)&&
        setup.displacement==TeleportPosition{0,2,-3}&&setup.goal==TeleportPosition{3,104,-145}&&
        setup.first_clock558==4.f&&setup.saved_air54a==0x40,"history goal is not an immediate root write");
    const auto prior=setup;
    check(!normal_teleport_history_setup(source,position,history,0x40,1,true,&setup)&&setup==prior,
        "directed branch refuses instead of borrowing the history goal");
    check(!normal_teleport_history_setup(switching,position,history,0,1,false,&setup)&&
        !normal_teleport_history_setup({5,1},position,history,0,1,false,&setup)&&setup==prior,
        "no Switching Ninja or alternate substitution");
    check(normal_teleport_history_setup(source,position,position,0,1,false,&setup)&&
        setup.displacement==TeleportPosition{}&&setup.goal==position&&setup.saved_air54a==0,
        "absent snapshot leaves history path; zero displacement stays at caster");
    check(normal_teleport_history_setup(source,position,history,0,0.0f,false,&setup),"restage ordinary goal");

    TeleportMotionProposal first,travel;
    check(normal_teleport_first_step(source,position,setup.displacement,0x40,&first)&&
        first.position==TeleportPosition{3,6,2}&&first.suppressed272==0x42&&first.contact_due,
        "first step proposes caster displacement and the selective mask before contact");
    check(normal_teleport_travel_step(source,{0,0,0},{10,20,-30},1.f,0x42,&travel)&&
        travel.position==TeleportPosition{1,2,-3}&&travel.suppressed272==0x42&&travel.contact_due,
        "travel converges without swapping with another actor");
    check(!normal_teleport_travel_step(source,first.position,setup.goal,std::numeric_limits<f32>::infinity(),0,&travel)&&
        travel.position==TeleportPosition{1,2,-3},"invalid clock cannot publish a path");

    WeaponStockPlan debit;
    const WeaponStockState stock{2,3,180,4.25f};
    check(normal_teleport_setup_debit(source,stock,&debit)&&debit.accepted&&
        debit.state==WeaponStockState{1,2,180,184.25f},"mode one adds the preset on a nonfinal debit");
    const auto pending=debit.state;
    check(normal_teleport_setup_debit(source,pending,&debit)&&debit.accepted&&
        debit.state==WeaponStockState{0,1,180,364.25f},"last debit adds rather than resets timer");
    const auto empty=debit.state;
    check(normal_teleport_setup_debit(source,empty,&debit)&&!debit.accepted&&debit.state==empty,
        "failed debit doesn't invent a refill or abort other setup work");
    static_assert(!std::is_convertible_v<WeaponStockPlan,WeaponStockModeZeroPlan>);
    WeaponStockModeZeroPlan mode_zero;
    check(weapon_stock_mode_zero_plan(stock,WeaponStockOperation::Debit,1,&mode_zero)&&mode_zero.state.refill_clock==4.25f,
        "mode zero behavior unchanged");
    u8 inhibit=0;
    check(normal_teleport_stock_inhibitor(source,0,0xa0,&inhibit)&&inhibit==0xa4&&
        normal_teleport_stock_inhibitor(source,1,inhibit,&inhibit)&&inhibit==0xa0,"stock hook changes only the X inhibit bit");

    TeleportFinishPlan finish;
    check(normal_teleport_finish(source,0x10000043,0x156,true,&finish)&&finish.suppressed272==0x154&&
        finish.flags5e0==0x10000040&&finish.field73f==0&&finish.next==TeleportExit::Fall,
        "completion clears only reviewed flags and chooses fall from current airborne state");
    check(!normal_teleport_finish(source,0,0,false,&finish)&&finish.next==TeleportExit::Fall,
        "interruption cannot reuse ordinary completion");
    check(normal_teleport_finish(source,3,2,true,&finish)&&!finish.suppressed272&&!finish.flags5e0&&finish.next==TeleportExit::Idle,
        "grounded current state chooses idle");
    std::printf("normal Teleport Ninja bounded self-movement stock and finish: %u checks PASS\n",checks);
}
