#pragma once
#include "engine/gameplay/weapon_stock_plan.hpp"

namespace gf {
using WeaponStockModeZeroState=WeaponStockState;
// Hold the existing mode-zero consumers to their reviewed stock result.
struct WeaponStockModeZeroPlan:WeaponStockPlan {};
inline bool weapon_stock_mode_zero_plan(const WeaponStockModeZeroState& in,WeaponStockOperation operation,
                                       s16 amount,WeaponStockModeZeroPlan* out){
    WeaponStockPlan next;
    if(!out||!weapon_stock_plan(in,WeaponStockRefillMode::SeedOnDepletion,operation,amount,&next))return false;
    *out=WeaponStockModeZeroPlan{next};return true;
}

// Bounded 6DD38 mode-0 visit. Only a previously armed positive clock can
// restore the cap; reaching zero clears the clock (unlike mode 1).
inline bool weapon_stock_mode_zero_refill(const WeaponStockModeZeroState& in,s16 max_stock,f32 actor_step,
                                          bool paused,WeaponStockModeZeroState* out){
    if(!out||max_stock<=0||in.current<0||in.current>max_stock||in.previous<0||in.refill_preset<0||
       !std::isfinite(in.refill_clock)||in.refill_clock<0||!std::isfinite(actor_step)||actor_step<0||actor_step>16)return false;
    auto next=in;
    if(!paused&&in.refill_clock>0){
        next.refill_clock=f32(in.refill_clock-actor_step);
        if(next.refill_clock<=0){next.refill_clock=0;next.current=next.previous=max_stock;}
    }
    *out=next;return true;
}
}
