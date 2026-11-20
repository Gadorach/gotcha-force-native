#pragma once
#include "engine/gameplay/model_form_rebind.hpp"

namespace gf {
// 0803 is a runtime form of the same retail selection, never a package or
// variant fallback. These leaves do not resolve the live actor or its assets.
struct NormalGoldHeroSource {
    static constexpr u16 base=0x0802,transformed=0x0803;
    static constexpr u32 action_lock_mask=0x91001003u;
    static constexpr s32 initial_form_ticks=1800;
    u16 model=0;
    u8 variant=0;
    constexpr bool is_base()const{return model==base&&!variant;}
    constexpr bool is_transformed()const{return model==transformed&&!variant;}
};

enum class GoldHeroFormAction:u8 {None=0,SetUp=4,Return=5};
struct GoldHeroFormTraits {
    u16 data_offset=0;
    u8 default_program_group=0,air_point_limit=0;
    bool operator==(const GoldHeroFormTraits&)const=default;
};

inline bool normal_gold_hero_form_traits(NormalGoldHeroSource source,GoldHeroFormTraits* out){
    if(!out)return false;
    if(source.is_base()){*out={0,1,2};return true;}
    if(source.is_transformed()){*out={432,0,4};return true;}
    return false;
}

struct GoldHeroSetupDebit {
    s16 stock_before=0;
    u8 channel=0,amount=0;
};

// Gate at the reviewed 4C8 path. The stock owner must actually handle the
// requested channel/count before any class action can be queued.
inline bool normal_gold_hero_setup_debit(NormalGoldHeroSource source,u32 pressed5bc,u32 flags5e0,
                                         s16 stock784,bool reached_hook,GoldHeroSetupDebit* out){
    if(!out||!source.is_base()||!reached_hook||stock784<0)return false;
    GoldHeroSetupDebit next;
    if((pressed5bc&0x400u)&&!(flags5e0&NormalGoldHeroSource::action_lock_mask)&&stock784>0)
        next={stock784,2,1};
    *out=next;return true;
}

struct GoldHeroStockObservation {
    s16 before=0,after=0;
    u8 channel=0;
    bool declared=false,accepted=false;
};

// Join to the observed stock result. This neither runs the refill/stock
// helper nor claims that the detached observation is a live receipt.
inline bool normal_gold_hero_setup_action(NormalGoldHeroSource source,const GoldHeroSetupDebit& request,
                                          const GoldHeroStockObservation& stock,GoldHeroFormAction* out){
    if(!out||!source.is_base()||request.channel!=2||request.amount!=1||request.stock_before<=0||
       !stock.declared||stock.channel!=request.channel||stock.before!=request.stock_before||
       stock.after!=stock.before-(stock.accepted?1:0))return false;
    *out=stock.accepted?GoldHeroFormAction::SetUp:GoldHeroFormAction::None;
    return true;
}

struct GoldHeroExpiryStep {
    s32 ticks144=0;
    GoldHeroFormAction action=GoldHeroFormAction::None;
    bool operator==(const GoldHeroExpiryStep&)const=default;
};

// One invocation of the transformed 4C8 branch, not a frame or elapsed-time
// clock. A last positive tick may queue return immediately if the guard allows.
// An ordinary zero is inactive; -1 is the observed retry sentinel.
inline bool normal_gold_hero_expiry_step(NormalGoldHeroSource source,s32 ticks144,u32 flags5e0,GoldHeroExpiryStep* out){
    if(!out||!source.is_transformed()||ticks144< -1||ticks144>NormalGoldHeroSource::initial_form_ticks)return false;
    GoldHeroExpiryStep next{ticks144};
    if(next.ticks144>0){
        --next.ticks144;
        if(!next.ticks144)next.ticks144=-1;
    }
    if(next.ticks144==-1&&!(flags5e0&NormalGoldHeroSource::action_lock_mask))
        next={0,GoldHeroFormAction::Return};
    *out=next;return true;
}

struct GoldHeroRebindPlan {
    ModelFormRebindPrefix binding;
    GoldHeroFormTraits target_traits;
    bool operator==(const GoldHeroRebindPlan&)const=default;
};

// Lower-completion gate and in-place rebind prefix only, after the caller
// has reached the corresponding class leaf. A proposal isn't a form commit:
// cleanup, resource/pose/collider rebinding and owner publication remain due.
inline bool normal_gold_hero_rebind(NormalGoldHeroSource source,GoldHeroFormAction current_action,
                                    u8 lower_complete1cee,u8 revision3f0,GoldHeroRebindPlan* out){
    if(!out)return false;
    using S=NormalGoldHeroSource;
    GoldHeroRebindPlan next;
    u16 target_model=0;
    if(source.is_base()&&current_action==GoldHeroFormAction::SetUp){
        if(lower_complete1cee)target_model=S::transformed;
    }else if(source.is_transformed()&&current_action==GoldHeroFormAction::Return)target_model=S::base;
    else return false;
    next.binding=model_form_rebind_prefix(target_model,source.variant,revision3f0);
    if(next.binding.requested&&!normal_gold_hero_form_traits({target_model,source.variant},&next.target_traits))return false;
    *out=next;return true;
}
}
