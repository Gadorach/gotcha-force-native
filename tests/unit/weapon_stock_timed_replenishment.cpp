#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "engine/gameplay/weapon_stock_mode_zero.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"weapon stock timed replenishment: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using Op=WeaponStockOperation;
    WeaponStockModeZeroState a{2,2,3,0},b{2,2,7,0};
    WeaponStockModeZeroPlan debit;
    // Independent declared channels, not balance constants or a live clock.
    for(auto* stock:{&a,&b}){
        check(weapon_stock_mode_zero_plan(*stock,Op::Debit,1,&debit)&&debit.accepted&&
              debit.state.current==1&&debit.state.refill_clock==0,"partial spend does not arm replenishment");
        *stock=debit.state;
        for(unsigned i=0;i<8;++i)
            check(weapon_stock_mode_zero_refill(*stock,2,1,false,stock)&&stock->current==1&&
                  stock->refill_clock==0,"waiting with a round remaining does not refill");
        check(weapon_stock_mode_zero_plan(*stock,Op::Debit,1,&debit)&&debit.accepted&&
              debit.state.current==0&&debit.state.refill_clock==stock->refill_preset,"last spend arms this channel's full delay");
        *stock=debit.state;
        check(weapon_stock_mode_zero_plan(*stock,Op::Debit,1,&debit)&&!debit.accepted&&
              debit.state==*stock,"attempting to spend empty stock cannot restart the delay");
    }
    check(weapon_stock_mode_zero_refill(a,2,3,true,&a)&&a.current==0&&a.refill_clock==3&&
          weapon_stock_mode_zero_refill(b,2,3,true,&b)&&b.current==0&&b.refill_clock==7,"paused visits do not consume either delay");
    for(unsigned elapsed=1;elapsed<=7;++elapsed){
        check(weapon_stock_mode_zero_refill(a,2,1,false,&a)&&weapon_stock_mode_zero_refill(b,2,1,false,&b),
              "advance each declared clock independently");
        check(a==(elapsed<3?WeaponStockModeZeroState{0,1,3,f32(3-elapsed)}:WeaponStockModeZeroState{2,2,3,0}),
              "short delay restores full stock exactly when due");
        check(b==(elapsed<7?WeaponStockModeZeroState{0,1,7,f32(7-elapsed)}:WeaponStockModeZeroState{2,2,7,0}),
              "long delay remains empty after the short delay has expired");
    }
    WeaponStockState returned;
    check(weapon_stock_return_one({1,2,7,0},2,&returned)&&returned==WeaponStockState{2,2,7,0},
          "an explicitly requested stock return is separate from timed replenishment");
    std::puts("weapon stock: depletion triggers, independent delays, pause and explicit return PASS");
}
