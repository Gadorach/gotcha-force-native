#pragma once
#include <array>
#include "engine/gameplay/neutral_shot_release_gate.hpp"
#include "engine/gameplay/weapon_stock_mode_zero.hpp"

namespace gf {
struct NormalGRedNeutralBSource {
    u16 model=0;
    u8 variant=0,slot=0;
    constexpr bool reviewed()const{return model==0x0615&&!variant&&slot<6;}
};
enum class GRedNeutralBRoute:u8 {None,Shot,Close,Charged};
struct GRedNeutralBChoice {
    GRedNeutralBRoute route=GRedNeutralBRoute::None;
    u8 channel=0,selector=0,class_state580=0;
};
// Reviewed neutral rows only, with competing non-B action paths excluded.
// Charge and close are requests, not executions of their later controllers.
inline bool normal_gred_neutral_b_choice(NormalGRedNeutralBSource source,u32 commands5d4,u8 flags58e,u8 near7d2,
                                         bool reviewed_neutral_context,GRedNeutralBChoice* out){
    if(!out||!source.reviewed()||!reviewed_neutral_context||(flags58e&1u))return false;
    GRedNeutralBChoice next;
    if((commands5d4&0x400u)&&!(flags58e&8u))next={GRedNeutralBRoute::Charged,3,44,3};
    else if((commands5d4&0x40u)&&!(flags58e&2u)&&(near7d2&8u))next={GRedNeutralBRoute::Close,1,44,1};
    else if(commands5d4&0x20u)next={GRedNeutralBRoute::Shot,0,44,0};
    *out=next;return true;
}
inline bool normal_gred_neutral_b_availability(NormalGRedNeutralBSource source,const GRedNeutralBChoice& row,
                                               const WeaponStockModeZeroState& stock,WeaponStockModeZeroPlan* out){
    return source.reviewed()&&row.route==GRedNeutralBRoute::Shot&&row.selector==44&&!row.channel&&!row.class_state580&&
        weapon_stock_mode_zero_plan(stock,WeaponStockOperation::CheckOnly,1,out);
}

struct GRedNeutralBStartup {
    f32 aim_clock560=0;
    u8 phase540=0,repeat745=0,mask=0,group=0,program=0;
};
inline bool normal_gred_neutral_b_start(NormalGRedNeutralBSource source,bool fresh_from_idle,u8 local_pose6ee,
                                        GRedNeutralBStartup* out){
    if(!out||!source.reviewed()||!fresh_from_idle||local_pose6ee)return false;
    *out={30,1,0,15,2,0};return true;
}
struct GRedNeutralBDirectionalPose {
    u8 descriptor_group=0;
    std::array<u8,3> choices{};
};
inline bool normal_gred_neutral_b_pose(NormalGRedNeutralBSource source,bool observed_release_command,GRedNeutralBDirectionalPose* out){
    if(!out||!source.reviewed())return false;
    *out={2,observed_release_command?std::array<u8,3>{7,6,8}:std::array<u8,3>{1,0,2}};return true;
}

inline bool normal_gred_neutral_b_vm_step(NormalGRedNeutralBSource source,u8 pre_vm_event1cef,f32 actor_step,f32* vm_step){
    if(!vm_step||!source.reviewed()||!std::isfinite(actor_step)||actor_step<0||actor_step>16)return false;
    *vm_step=pre_vm_event1cef?0.f:actor_step;return true;
}
using GRedNeutralBReleaseGate=NeutralShotReleaseGate<NormalGRedNeutralBSource>;
// After the VM/aim visit; uses the restored actor step and observes the
// current cue. A timeout alone cannot fire. This isn't a VM or hit receipt.
inline bool normal_gred_neutral_b_release_gate(NormalGRedNeutralBSource source,u8 post_vm_event1cef,
                                               bool aim_pass,f32 aim_clock560,f32 actor_step,GRedNeutralBReleaseGate* out){
    return neutral_shot_release_gate(source,post_vm_event1cef,aim_pass,aim_clock560,actor_step,out);
}
struct GRedNeutralBReleasePlan {
    WeaponStockModeZeroState stock;
    f32 clock560=0,clock558=0;
    u8 local_pose6ee=0;
    bool request_beam=false;
    u8 owner_slot=0,object_type=0,argument=0;
};
// Scope is argument 43's request, not an allocation or collision. Positive
// gate always services its setup clocks even if the debit then refuses.
inline bool normal_gred_neutral_b_debit(NormalGRedNeutralBSource source,const GRedNeutralBReleaseGate& gate,
                                        const WeaponStockModeZeroState& channel0,GRedNeutralBReleasePlan* out){
    WeaponStockModeZeroPlan debit;
    if(!out||!source.reviewed()||!gate.release_due||gate.phase540!=2||gate.slot!=source.slot||
       !weapon_stock_mode_zero_plan(channel0,WeaponStockOperation::Debit,1,&debit))return false;
    GRedNeutralBReleasePlan next{debit.state,20,12,1};
    if(debit.accepted){next.request_beam=true;next.owner_slot=source.slot;next.object_type=2;next.argument=43;}
    *out=next;return true;
}
}
