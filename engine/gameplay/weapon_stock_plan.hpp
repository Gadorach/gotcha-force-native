#pragma once
#include <cmath>
#include "engine/core/types.hpp"

namespace gf {
enum class WeaponStockOperation:u8 {CheckOnly,Debit};
enum class WeaponStockRefillMode:u8 {SeedOnDepletion,AddOnDebit,ClearClockOnDebit};
struct WeaponStockState {
    s16 current=0,previous=0,refill_preset=0;
    f32 refill_clock=0;
    bool operator==(const WeaponStockState&)const=default;
};
struct WeaponStockPlan {
    WeaponStockState state;
    WeaponStockOperation operation=WeaponStockOperation::CheckOnly;
    s16 amount=0;
    bool accepted=false;
    bool operator==(const WeaponStockPlan&)const=default;
};

// Bounded modes 0/1 and the shared mode-2/3 clear branch of 6DBE0. The caller owns the
// declared channel/mode. This does not tick a reload or mutate any live actor.
inline bool weapon_stock_plan(const WeaponStockState& in,WeaponStockRefillMode mode,WeaponStockOperation operation,
                              s16 amount,WeaponStockPlan* out){
    if(!out||in.current<0||in.previous<0||in.refill_preset<0||!std::isfinite(in.refill_clock)||
       in.refill_clock<(mode==WeaponStockRefillMode::AddOnDebit?-16.f:0.f)||
       (mode!=WeaponStockRefillMode::SeedOnDepletion&&mode!=WeaponStockRefillMode::AddOnDebit&&
           mode!=WeaponStockRefillMode::ClearClockOnDebit)||
       amount<=0||(operation!=WeaponStockOperation::CheckOnly&&operation!=WeaponStockOperation::Debit))return false;
    WeaponStockPlan next{in,operation,amount,in.current>=amount};
    if(next.accepted&&operation==WeaponStockOperation::Debit){
        next.state.previous=in.current;
        next.state.current=s16(in.current-amount);
        if(mode==WeaponStockRefillMode::AddOnDebit)next.state.refill_clock=f32(in.refill_clock+f32(in.refill_preset));
        else if(mode==WeaponStockRefillMode::ClearClockOnDebit)next.state.refill_clock=0.f;
        else if(!next.state.current)next.state.refill_clock=f32(in.refill_preset);
    }
    *out=next;return true;
}

// Bounded 6DE10 count writer, not a stock-return trigger. The caller must
// own the channel, actual object/lifetime and one accepted return event.
inline bool weapon_stock_return_one(const WeaponStockState& in,s16 max_stock,WeaponStockState* out){
    if(!out||in.current<0||in.current==32767||in.previous<0||in.refill_preset<0||
       !std::isfinite(in.refill_clock)||in.refill_clock<0||max_stock<=0)return false;
    auto next=in;
    const s32 raised=s32(in.current)+1;
    next.current=next.previous=s16(raised>max_stock?max_stock:raised);
    *out=next;return true;
}

// Bounded 6DD74 mode-1 visit. Unlike mode 0, crossing zero restores stock
// without zeroing the negative remainder. No generic owner clock is driven.
inline bool weapon_stock_mode_one_refill(const WeaponStockState& in,s16 max_stock,f32 actor_step,
                                         bool paused,WeaponStockState* out){
    if(!out||max_stock<=0||in.current<0||in.current>max_stock||in.previous<0||in.refill_preset<=0||
       !std::isfinite(in.refill_clock)||in.refill_clock< -16.f||
       in.refill_clock>f32(max_stock)*f32(in.refill_preset)||!std::isfinite(actor_step)||actor_step<0||actor_step>16.f)return false;
    auto next=in;
    if(!paused&&in.refill_clock>0){
        next.refill_clock=f32(in.refill_clock-actor_step);
        if(next.refill_clock<=0)next.current=next.previous=max_stock;
        else{
            const f32 intervals=next.refill_clock/f32(in.refill_preset);
            const s32 count=s32(max_stock)-s32(intervals)-1;
            if(count<0)return false; // don't widen the domain to out-of-range stock
            if(count!=in.current)next.current=next.previous=s16(count);
        }
    }
    *out=next;return true;
}
}
