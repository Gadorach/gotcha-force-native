#pragma once
#include "engine/gameplay/weapon_stock_plan.hpp"

namespace gf {
struct KnightSamuraiSource {
    u16 model=0;
    u8 variant=0,slot=0;
    constexpr bool knight()const{return model==0x020a&&!variant&&slot<6;}
    constexpr bool samurai()const{return model==0x0700&&!variant&&slot<6;}
};
struct KnightSamuraiActionRequest {
    bool requested=false;
    u8 channel=0,class_state580=0,substate581=0,selector=0;
};
// The caller supplies the already conditioned ordinary route. Near target
// classification is only for Samurai's request; it is not a contact.
inline bool normal_knight_samurai_action_request(KnightSamuraiSource source,u32 commands5d4,u8 flags58e,u8 near7d2,
                                                bool reviewed_ordinary_route,KnightSamuraiActionRequest* out){
    if(!out||!reviewed_ordinary_route||(!source.knight()&&!source.samurai()))return false;
    KnightSamuraiActionRequest next;
    if(source.knight()&&(commands5d4&0x80u)&&!(flags58e&4u))next={true,2,0,0,44};
    if(source.samurai()&&(commands5d4&0x40u)&&!(flags58e&2u)&&(near7d2&8u))next={true,1,1,0,44};
    *out=next;return true;
}

struct KnightShieldEventPlan {
    WeaponStockState stock;
    u8 event1cef=0,latch144=0;
    bool attempted_debit=false,request_factory=false;
    u8 owner_slot=0,object_type=0,argument=0;
    bool write_latch_after_factory=false;
    u8 post_factory_latch144=0;
};
// Selected signed event leaf, not a successful block. The latch-2 write
// follows the factory call even when the allocator refuses that call.
inline bool normal_knight_shield_event(KnightSamuraiSource source,u8 event1cef,u8 latch144,
                                      const WeaponStockState& channel2,KnightShieldEventPlan* out){
    if(!out||!source.knight()||latch144>2)return false;
    WeaponStockPlan stock;
    if(!weapon_stock_plan(channel2,WeaponStockRefillMode::ClearClockOnDebit,WeaponStockOperation::CheckOnly,1,&stock))return false;
    KnightShieldEventPlan next{channel2,event1cef,latch144};
    const s8 event=std::bit_cast<s8>(event1cef);
    if(event<0&&!latch144){next.event1cef=0;next.latch144=1;}
    else if(event>0&&latch144<=1){
        if(!weapon_stock_plan(channel2,WeaponStockRefillMode::ClearClockOnDebit,WeaponStockOperation::Debit,1,&stock))return false;
        next.attempted_debit=true;next.stock=stock.state;
        if(!stock.accepted)next.latch144=0;
        else{
            next.event1cef=0;
            next.request_factory=true;next.owner_slot=source.slot;next.object_type=24;next.argument=1;
            next.write_latch_after_factory=true;next.post_factory_latch144=2;
        }
    }
    *out=next;return true;
}
struct KnightShieldOwnerStock {
    WeaponStockState stock;
    u8 default_group582=0;
    bool return_depleted_stock=false;
};
// Inspected owner hook, not stock restored by any generic hit, guard action
// or failed allocation.
inline bool normal_knight_shield_owner_hook(KnightSamuraiSource source,u8 latch144,bool reached_hook,
                                           const WeaponStockState& channel2,s16 max_stock,KnightShieldOwnerStock* out){
    WeaponStockPlan checked;
    if(!out||!source.knight()||!reached_hook||latch144>2||max_stock<=0||
       !weapon_stock_plan(channel2,WeaponStockRefillMode::ClearClockOnDebit,WeaponStockOperation::CheckOnly,1,&checked))return false;
    KnightShieldOwnerStock next{channel2,u8(latch144!=0),!latch144&&!channel2.current};
    if(next.return_depleted_stock&&!weapon_stock_return_one(channel2,max_stock,&next.stock))return false;
    *out=next;return true;
}

struct SamuraiMeleeShapeRequest {
    bool requested=false;
    u8 carrier_slot=0,credited_owner_slot=0,group=0;
};
// Scoped first close-B group-3/program-0 command, not an actual registered
// shape, overlap, damage or combo. There is no stock debit in this request.
inline bool normal_samurai_melee_shape(KnightSamuraiSource source,u8 lane_mask,bool reached_reviewed_opcode,
                                      SamuraiMeleeShapeRequest* out){
    if(!out||!source.samurai()||!reached_reviewed_opcode)return false;
    *out=(lane_mask&1u)?SamuraiMeleeShapeRequest{true,source.slot,source.slot,1}:SamuraiMeleeShapeRequest{};
    return true;
}
struct SamuraiPresentationRequest {
    bool requested=false;
    u8 owner_slot=0,visual_key=0;
};
inline bool normal_samurai_melee_presentation(KnightSamuraiSource source,u8 lane_mask,bool class_hook_bound,
                                             bool reached_key1_opcode,SamuraiPresentationRequest* out){
    if(!out||!source.samurai()||!reached_key1_opcode)return false;
    *out=((lane_mask&1u)&&class_hook_bound)?SamuraiPresentationRequest{true,source.slot,6}:SamuraiPresentationRequest{};
    return true;
}
}
