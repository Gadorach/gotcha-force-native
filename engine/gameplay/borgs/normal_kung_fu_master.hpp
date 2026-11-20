#pragma once
#include "engine/gameplay/weapon_stock_plan.hpp"

namespace gf {
struct NormalKungFuSource {
    u16 model=0;
    u8 variant=0,slot=0;
    constexpr bool reviewed()const{return model==0x0800&&!variant&&slot<6;}
};
enum class KungFuBRoute:u8 {None,Close,Shot};
struct KungFuBRequest {
    KungFuBRoute route=KungFuBRoute::None;
    u8 selector=0,action_channel=0,class_state580=0;
};
// Ordinary context, after earlier arbitration, with the scoped shot row
// separately known. Near classification only selects a request, not a hit.
inline bool normal_kung_fu_b_request(NormalKungFuSource source,u32 commands5d4,u8 flags58e,u8 near7d2,
                                     bool reviewed_ordinary_context,KungFuBRequest* out){
    if(!out||!source.reviewed()||!reviewed_ordinary_context)return false;
    KungFuBRequest next;
    if((commands5d4&0x40u)&&!(flags58e&2u)&&((near7d2&8u)||(flags58e&1u)))next={KungFuBRoute::Close,44,1,1};
    else if(commands5d4&0x20u){
        if(flags58e&1u)return false; // alternative channel is not the neutral shot
        next={KungFuBRoute::Shot,44,0,0};
    }
    *out=next;return true;
}
inline bool normal_kung_fu_shot_availability(NormalKungFuSource source,const KungFuBRequest& selected,
                                            const WeaponStockState& stock,WeaponStockPlan* out){
    return source.reviewed()&&selected.route==KungFuBRoute::Shot&&selected.selector==44&&
        !selected.action_channel&&!selected.class_state580&&
        weapon_stock_plan(stock,WeaponStockRefillMode::AddOnDebit,WeaponStockOperation::CheckOnly,1,out);
}

struct KungFuShotRelease {
    WeaponStockState stock;
    u32 flags5e0=0;
    u8 subphase541=0;
    bool attempted_debit=false,request_projectile=false,clear_action73f=false;
    u8 owner_slot=0,projectile_key=0;
};
// Ordinary shot class leaf. The exact event pair spends stock, advances
// subphase even on refusal, and can request an actor-owned projectile.
// A signed -1 in the later subphase is recovery, not an extra shot.
inline bool normal_kung_fu_shot_release(NormalKungFuSource source,u8 subphase541,u8 event1cef,u8 event1cf0,u32 flags5e0,
                                        const WeaponStockState& channel0,KungFuShotRelease* out){
    WeaponStockPlan checked;
    if(!out||!source.reviewed()||subphase541>1||
       !weapon_stock_plan(channel0,WeaponStockRefillMode::AddOnDebit,WeaponStockOperation::CheckOnly,1,&checked))return false;
    KungFuShotRelease next{channel0,flags5e0,subphase541};
    if(!subphase541&&event1cef==3&&event1cf0==1){
        if(!weapon_stock_plan(channel0,WeaponStockRefillMode::AddOnDebit,WeaponStockOperation::Debit,1,&checked))return false;
        next.attempted_debit=true;next.subphase541=1;next.stock=checked.state;
        if(checked.accepted){next.request_projectile=true;next.owner_slot=source.slot;next.projectile_key=12;}
    }else if(subphase541==1&&event1cef==255){
        next.flags5e0&=~3u;next.subphase541=2;next.clear_action73f=true;
    }
    *out=next;return true;
}
struct KungFuHandPresentation {
    bool requested=false;
    u8 owner_slot=0,key=0;
};
// Scoped cues of the ordinary shot stream, not hit geometry or audio.
inline bool normal_kung_fu_shot_hand(NormalKungFuSource source,u8 cue6e9,KungFuHandPresentation* out){
    if(!out||!source.reviewed()||(cue6e9!=0&&cue6e9!=1&&cue6e9!=4))return false;
    *out=cue6e9==1?KungFuHandPresentation{true,source.slot,85}:KungFuHandPresentation{};
    return true;
}
}
