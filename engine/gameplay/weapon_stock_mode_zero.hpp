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
}
