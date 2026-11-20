#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_neutral_b.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal G-Red neutral B: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using R=GRedNeutralBRoute;
    constexpr NormalGRedNeutralBSource red{0x0615,0,3};
    GRedNeutralBChoice row;
    check(normal_gred_neutral_b_choice(red,0x460,0,8,true,&row)&&row.route==R::Charged&&row.channel==3,
        "charge request preempts close/neutral shot");
    check(normal_gred_neutral_b_choice(red,0x460,8,8,true,&row)&&row.route==R::Close&&row.channel==1,
        "inhibited charge leaves guarded close ahead of shot");
    check(normal_gred_neutral_b_choice(red,0x60,0,0,true,&row)&&row.route==R::Shot&&row.selector==44&&
        !row.channel&&!row.class_state580,"scoped neutral shot action, not continuous held firing");
    check(!normal_gred_neutral_b_choice(red,0x60,1,0,true,&row)&&!normal_gred_neutral_b_choice(red,0x60,0,0,false,&row)&&
        row.route==R::Shot,"unknown context and earlier arbitration don't silently choose the shot");
    WeaponStockModeZeroPlan availability;
    const WeaponStockModeZeroState full{5,1,180,0},last{1,3,180,0},empty{0,7,180,0};
    check(normal_gred_neutral_b_availability(red,row,full,&availability)&&availability.accepted&&availability.state==full,
        "check-only stock doesn't spend");
    check(normal_gred_neutral_b_availability(red,row,empty,&availability)&&!availability.accepted&&availability.state==empty,
        "failed availability is not a shot debit");

    GRedNeutralBStartup start;
    check(normal_gred_neutral_b_start(red,true,0,&start)&&start.aim_clock560==30&&start.phase540==1&&
        !start.repeat745&&start.mask==15&&start.group==2&&!start.program,"exact fresh-idle startup operands");
    check(!normal_gred_neutral_b_start(red,true,1,&start)&&start.aim_clock560==30,"repeat context stays outside this start");
    GRedNeutralBDirectionalPose pose;
    check(normal_gred_neutral_b_pose(red,false,&pose)&&pose.choices==std::array<u8,3>{1,0,2}&&
        normal_gred_neutral_b_pose(red,true,&pose)&&pose.choices==std::array<u8,3>{7,6,8}&&pose.descriptor_group==2,
        "directional descriptors differ between startup and release without inventing body motion");
    f32 vm_step=555;
    check(normal_gred_neutral_b_vm_step(red,1,1.25f,&vm_step)&&vm_step==0&&
        normal_gred_neutral_b_vm_step(red,0,1.25f,&vm_step)&&vm_step==1.25f,"pending cue pauses just the VM call");
    GRedNeutralBReleaseGate gate;
    check(normal_gred_neutral_b_release_gate(red,0,false,0,1,&gate)&&gate.aim_clock560==-1&&!gate.release_due,
        "timeout alone cannot fire");
    check(normal_gred_neutral_b_release_gate(red,1,false,2,1,&gate)&&gate.aim_clock560==1&&!gate.release_due,
        "cue without aim pass or timeout waits");
    check(normal_gred_neutral_b_release_gate(red,1,true,2,1,&gate)&&gate.aim_clock560==2&&gate.release_due&&gate.phase540==2,
        "aim pass and program cue jointly authorize release");
    GRedNeutralBReleasePlan release;
    check(!normal_gred_neutral_b_debit({0x0615,0,2},gate,last,&release)&&!release.request_beam,
        "another actor slot cannot reuse the release");
    check(normal_gred_neutral_b_debit(red,gate,last,&release)&&release.stock==WeaponStockModeZeroState{0,1,180,180}&&
        release.clock560==20&&release.clock558==12&&release.local_pose6ee==1&&release.request_beam&&
        release.owner_slot==3&&release.object_type==2&&release.argument==43,"last stock seeds refill and requests exact beam");
    check(normal_gred_neutral_b_debit(red,gate,empty,&release)&&!release.request_beam&&release.stock==empty&&
        release.clock560==20&&release.clock558==12,"debit refusal still writes the class clocks");

    WeaponStockModeZeroState refill;
    check(weapon_stock_mode_zero_refill(empty,5,1,false,&refill)&&refill==empty,"synthetic empty stock has no automatic reload");
    check(weapon_stock_mode_zero_refill({0,1,180,0.5f},5,1,false,&refill)&&refill==WeaponStockModeZeroState{5,5,180,0},
        "armed mode-zero expiry restores full stock and clears its clock");
    std::printf("normal G-Red neutral B cue, aim gate and stock proposal: %u checks PASS\n",checks);
}
