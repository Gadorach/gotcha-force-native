#pragma once
#include <cmath>
#include "engine/core/types.hpp"

namespace gf {
enum class WeaponStockOperation:u8 {CheckOnly,Debit};
struct WeaponStockModeZeroState {
    s16 current=0,previous=0,refill_preset=0;
    f32 refill_clock=0;
    bool operator==(const WeaponStockModeZeroState&)const=default;
};
struct WeaponStockModeZeroPlan {
    WeaponStockModeZeroState state;
    WeaponStockOperation operation=WeaponStockOperation::CheckOnly;
    s16 amount=0;
    bool accepted=false;
    bool operator==(const WeaponStockModeZeroPlan&)const=default;
};

// Bounded mode-0 operand branch of the shared 6DBE0 stock helper. This
// neither owns the channel nor ticks a reload or mutates any live actor.
inline bool weapon_stock_mode_zero_plan(const WeaponStockModeZeroState& in,WeaponStockOperation operation,
                                       s16 amount,WeaponStockModeZeroPlan* out){
    if(!out||in.current<0||in.previous<0||in.refill_preset<0||!std::isfinite(in.refill_clock)||in.refill_clock<0||
       amount<=0||(operation!=WeaponStockOperation::CheckOnly&&operation!=WeaponStockOperation::Debit))return false;
    WeaponStockModeZeroPlan next{in,operation,amount,in.current>=amount};
    if(next.accepted&&operation==WeaponStockOperation::Debit){
        next.state.previous=in.current;
        next.state.current=s16(in.current-amount);
        if(!next.state.current)next.state.refill_clock=f32(in.refill_preset);
    }
    *out=next;return true;
}
}
