#pragma once
#include <bit>
#include "engine/gameplay/neutral_shot_release_gate.hpp"
#include "engine/gameplay/weapon_stock_mode_zero.hpp"

namespace gf {
struct NormalSasukeNeutralBSource {
    u16 model=0;
    u8 variant=0,slot=0;
    constexpr bool reviewed()const{return model==0x000a&&!variant&&slot<6;}
};
enum class SasukeNeutralBRoute:u8 {None,Shot,Close};
struct SasukeNeutralBChoice {
    SasukeNeutralBRoute route=SasukeNeutralBRoute::None;
    u8 channel=0,selector=0,class_state580=0;
};
// Shared charged-B pulse cannot admit on this exact normal controller.
// Other targeting buckets or competing class paths are not reconstructed.
inline bool normal_sasuke_neutral_b_choice(NormalSasukeNeutralBSource source,u32 commands5d4,u8 flags58e,u8 near7d2,
                                           bool reviewed_neutral_context,bool no_cached_target,
                                           SasukeNeutralBChoice* out){
    if(!out||!source.reviewed()||!reviewed_neutral_context||(flags58e&1u))return false;
    SasukeNeutralBChoice next;
    if((commands5d4&0x40u)&&!(flags58e&2u)&&(near7d2&8u))next={SasukeNeutralBRoute::Close,1,44,1};
    else if(commands5d4&0x20u){
        if(!no_cached_target)return false;
        next={SasukeNeutralBRoute::Shot,0,44,0};
    }
    *out=next;return true;
}
inline bool normal_sasuke_neutral_b_availability(NormalSasukeNeutralBSource source,const SasukeNeutralBChoice& row,
                                                 const WeaponStockModeZeroState& stock,WeaponStockModeZeroPlan* out){
    return source.reviewed()&&row.route==SasukeNeutralBRoute::Shot&&row.selector==44&&!row.channel&&!row.class_state580&&
        weapon_stock_mode_zero_plan(stock,WeaponStockOperation::CheckOnly,1,out);
}

struct SasukeNeutralBStartup {
    f32 aim_clock560=0;
    u8 phase540=0,attempts6ef=0,group=0,upper_mask=0,upper_program=0,lower_mask=0,lower_program=0;
};
inline bool normal_sasuke_neutral_b_start(NormalSasukeNeutralBSource source,bool fresh_from_idle,u8 local_pose6ee,
                                          SasukeNeutralBStartup* out){
    if(!out||!source.reviewed()||!fresh_from_idle||local_pose6ee)return false;
    *out={30,1,5,2,1,0,2,9};return true;
}
// Sasuke skips the VM call for a pending cue, unlike G-Red's zero step.
inline bool normal_sasuke_neutral_b_should_visit_vm(NormalSasukeNeutralBSource source,u8 pre_vm_event1cef,bool* out){
    if(!out||!source.reviewed())return false;
    *out=!pre_vm_event1cef;return true;
}
using SasukeNeutralBReleaseGate=NeutralShotReleaseGate<NormalSasukeNeutralBSource>;
inline bool normal_sasuke_neutral_b_release_gate(NormalSasukeNeutralBSource source,u8 post_vm_event1cef,
                                                 bool aim_pass,f32 aim_clock560,f32 actor_step,SasukeNeutralBReleaseGate* out){
    return neutral_shot_release_gate(source,post_vm_event1cef,aim_pass,aim_clock560,actor_step,out);
}
struct SasukeNeutralBReleasePlan {
    WeaponStockModeZeroState stock;
    u8 local_pose6ee=0,attempts6ef=0;
    bool request_shuriken=false;
    u8 owner_slot=0,object_type=0,argument=0;
};
// Paid relay only, before allocation. Even a refused debit uses an
// attempt. No projectile fan, refund, live object or hit is supplied.
inline bool normal_sasuke_neutral_b_debit(NormalSasukeNeutralBSource source,const SasukeNeutralBReleaseGate& gate,
                                          u8 attempts6ef,const WeaponStockModeZeroState& channel0,SasukeNeutralBReleasePlan* out){
    WeaponStockModeZeroPlan debit;
    if(!out||!source.reviewed()||!gate.release_due||gate.phase540!=2||gate.slot!=source.slot||attempts6ef<1||attempts6ef>5||
       !weapon_stock_mode_zero_plan(channel0,WeaponStockOperation::Debit,1,&debit))return false;
    SasukeNeutralBReleasePlan next{debit.state,1,u8(attempts6ef-1u)};
    if(debit.accepted){next.request_shuriken=true;next.owner_slot=source.slot;next.object_type=14;next.argument=4;}
    *out=next;return true;
}

struct SasukeNeutralBRepeatPlan {
    bool checked_stock=false,request_repeat=false,request_cleanup=false;
    u8 phase540=2,mask=0,group=0,program=0;
};
// Ordinary grounded phase two, with other exits excluded. A held-B
// availability check is not another debit or another projectile.
inline bool normal_sasuke_neutral_b_repeat(NormalSasukeNeutralBSource source,bool reviewed_grounded_phase,
                                           u32 held5b4,u8 attempts6ef,bool lower_done,
                                           const WeaponStockModeZeroState& channel0,SasukeNeutralBRepeatPlan* out){
    const auto attempts=std::bit_cast<s8>(attempts6ef);
    if(!out||!source.reviewed()||!reviewed_grounded_phase||attempts>5)return false;
    SasukeNeutralBRepeatPlan next;
    if(held5b4&0x200u){
        WeaponStockModeZeroPlan check;
        if(!weapon_stock_mode_zero_plan(channel0,WeaponStockOperation::CheckOnly,1,&check))return false;
        next.checked_stock=true;
        if(check.accepted&&attempts>0){
            next.request_repeat=true;next.phase540=1;next.mask=1;next.group=2;next.program=1;
        }else next.request_cleanup=true;
    }else next.request_cleanup=lower_done;
    *out=next;return true;
}
}
