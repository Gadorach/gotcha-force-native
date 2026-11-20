#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_roach.hpp"
#include "engine/gameplay/weapon_stock_mode_zero.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Roach: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using I=RoachMovementIntent;
    using S=NormalRoachSource;
    constexpr S source{0x0f05,0,0};
    static_assert(S::movement_type==1&&S::resource_policy==0&&S::air_points.limit==1&&!S::air_points.seed_on_dash_windup);
    CharacterMovementInputState input;
    input.commands5d4=1;input.previous_held=0x100;
    RoachMovementRequest move;
    u8 used=0;
    check(normal_roach_movement_request(source,I::IdleJump,0,0,0,used,true,input,&move)&&move.selector==4&&
        move.reset_direction57c&&used==0,"takeoff request spends nothing");
    check(!normal_roach_count_entry(source,4,used,&used)&&used==0,"startup isn't ascent");
    check(normal_roach_count_entry(source,5,used,&used)&&used==1,"ascent spends the only count");
    input.commands5d4=2;
    check(normal_roach_movement_request(source,I::FallRelaunch,6,0x40,1,used,true,input,&move)&&move.selector==6,
        "ordinary used jump cannot relaunch");
    check(normal_roach_movement_request(source,I::FallRelaunch,6,0x40,1,0,true,input,&move)&&move.selector==8&&
        normal_roach_count_entry(source,8,0,&used)&&used==1,"count-zero fall can request one relaunch, spent on entry");
    check(!normal_roach_count_entry(source,15,0,&used)&&used==1,"no dash-based seed");
    input.commands5d4=0;input.directions5d8=0x33;
    check(normal_roach_movement_request(source,I::GroundDash,0,0,0,used,true,input,&move)&&move.selector==0&&
        normal_roach_movement_request(source,I::AirDash,6,0x40,1,used,true,input,&move)&&move.selector==6&&
        move.remaining_directions5d8==0x33,"refused dash rows leave flick with its input owner");
    check(!normal_roach_movement_request({0x0f05,0,1},I::AirDash,6,0x40,1,used,true,input,&move)&&
        !normal_roach_movement_request({0x0c00,0,0},I::AirDash,6,0x40,1,used,true,input,&move)&&move.selector==6,
        "other role and Normal Tank cannot borrow exact Roach rules");

    CharacterAirResourceState resource{2200,1,1};
    check(normal_roach_grounded_bookkeeping(source,resource,1,&resource)&&resource.air_actions747==1&&resource.value736==2200,
        "airborne/count state is not replaced by a floor sample");
    check(normal_roach_grounded_bookkeeping(source,resource,0,&resource)&&!resource.air_actions747&&!resource.delay73a&&
        resource.value736==2200,"grounded bookkeeping resets count even on a delay-only visit");
    check(normal_roach_grounded_bookkeeping(source,resource,0,&resource)&&resource.value736==4200,
        "stock-independent dormant meter storage still has a refill branch");

    RoachControlProgramSample sample;
    check(normal_roach_control_program_sample(source,0,2,&sample)&&sample.wait_mode==9&&sample.wait_ticks==10&&
        !sample.binds_local_clip&&!sample.behaviour_payload,"takeoff has a clipless control wait");
    check(normal_roach_control_program_sample(source,0,4,&sample)&&sample.behaviour_payload==0x40&&sample.wait_ticks==2,
        "ascent requests different controls, not a default clip");
    check(normal_roach_control_program_sample(source,0,8,&sample)&&sample.lane_control1d0f==1&&
        sample.behaviour_payload==0x200&&sample.wait_ticks==4&&!sample.binds_local_clip,"landing control sample");
    check(!normal_roach_control_program_sample(source,1,8,&sample)&&sample.wait_ticks==4,"unknown group cannot acquire guessed asset");

    RoachBombReleasePlan bomb;
    check(normal_roach_bomb_release(source,3,1,{1,5,300,7.5f},&bomb)&&bomb.next_phase540==2&&bomb.clock558==10&&
        bomb.request_bomb&&bomb.object_type==16&&bomb.firing_slot==3&&bomb.stock.state==WeaponStockState{0,1,300,0},
        "class release debits stock and clears generic refill clock");
    WeaponStockState stock=bomb.stock.state;
    check(!normal_roach_bomb_retirement_stock(source,{16,2,3,3,false},stock,1,&stock)&&!stock.current,
        "release/failed allocation isn't a retirement receipt");
    check(!normal_roach_bomb_retirement_stock(source,{16,2,3,4,true},stock,1,&stock)&&!stock.current,
        "stock return requires the bomb's observed owner");
    check(normal_roach_bomb_retirement_stock(source,{16,2,3,3,true},stock,1,&stock)&&stock==WeaponStockState{1,1,300,0},
        "owner return restores one count and its previous sample, not a jump");
    check(normal_roach_bomb_retirement_stock(source,{16,2,3,3,true},stock,1,&stock)&&stock.current==1,
        "numeric return is capped; lifetime caller still owns one event");
    check(normal_roach_bomb_release(source,3,1,{0,7,300,5},&bomb)&&!bomb.request_bomb&&bomb.next_phase540==2&&
        bomb.stock.state==WeaponStockState{0,7,300,5},"failed debit still advances class phase but leaves stock alone");
    WeaponStockModeZeroPlan mode_zero;
    check(weapon_stock_mode_zero_plan({1,1,300,5},WeaponStockOperation::Debit,1,&mode_zero)&&mode_zero.state.refill_clock==300,
        "mode-two clears do not alter mode-zero refill seed");
    std::printf("normal Roach one-use count, clipless metadata and bomb stock: %u checks PASS\n",checks);
}
