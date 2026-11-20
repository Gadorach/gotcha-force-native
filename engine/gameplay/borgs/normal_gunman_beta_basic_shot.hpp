#pragma once
#include "engine/gameplay/weapon_stock_mode_zero.hpp"

namespace gf {
struct NormalGunmanBetaShotSource {
    static constexpr u16 gunman=0x0100,beta_ii=0x0105;
    u16 model=0;
    u8 variant=0;
    constexpr bool reviewed()const{return !variant&&(model==gunman||model==beta_ii);}
};
struct GunmanBetaShotTraits {
    u8 program_group=0,program=0,base_projectile=0;
    s16 level0_stock=0,level0_refill_preset=0;
};
inline bool normal_gunman_beta_shot_traits(NormalGunmanBetaShotSource source,GunmanBetaShotTraits* out){
    if(!out||!source.reviewed())return false;
    *out=source.model==NormalGunmanBetaShotSource::gunman?GunmanBetaShotTraits{2,4,0,20,120}:
        GunmanBetaShotTraits{2,0,56,5,60};
    return true;
}

struct GunmanBetaShotAction {
    bool request=false;
    u8 group=0,class_state580=0,substate581=0;
};
struct GunmanBetaShotRequestFields {
    u32 commands5d4=0;
    u8 action_mode748=0,flags58e=0;
    bool reached_idle_no_target_leaf=false;
};
// Already conditioned request on the reviewed no-target idle path only.
// Neither this leaf nor a held button owns action arbitration or a debit.
inline bool normal_gunman_beta_shot_request(NormalGunmanBetaShotSource source,const GunmanBetaShotRequestFields& fields,
                                           GunmanBetaShotAction* out){
    if(!out||!source.reviewed()||!fields.reached_idle_no_target_leaf||fields.action_mode748==2||(fields.flags58e&1u))return false;
    *out={(fields.commands5d4&0x20u)!=0,0,0,0};return true;
}

struct GunmanBetaShotStockRequest {
    u8 channel=0;
    s16 amount=0;
    WeaponStockOperation operation=WeaponStockOperation::CheckOnly;
};
struct GunmanBetaShotReleasePlan {
    u8 next_substate540=0;
    GunmanBetaShotStockRequest stock;
};
// The actual animation event is signed. Advancing the substate and asking
// for one debit does not imply that the stock or projectile request succeeds.
inline bool normal_gunman_beta_shot_release(NormalGunmanBetaShotSource source,u8 substate540,u8 event1cef,
                                           GunmanBetaShotReleasePlan* out){
    if(!out||!source.reviewed()||substate540!=1)return false;
    if(std::bit_cast<s8>(event1cef)>0)*out={2,{0,1,WeaponStockOperation::Debit}};
    else *out={1,{}};
    return true;
}
// Reviewed repeat tail is a new conditioned pulse and a check-only request.
inline bool normal_gunman_beta_shot_repeat(NormalGunmanBetaShotSource source,u32 commands5d4,
                                          GunmanBetaShotStockRequest* out){
    if(!out||!source.reviewed())return false;
    *out={0,s16((commands5d4&0x20u)!=0),WeaponStockOperation::CheckOnly};return true;
}

struct GunmanBetaProjectileRequest {
    bool requested=false;
    u8 firing_slot=0,projectile_key=0;
    bool operator==(const GunmanBetaProjectileRequest&)const=default;
};
// Consumes the scoped release's channel-0 debit result. This is a request
// with a supplied owner slot, not an allocated object or a live owner receipt.
// Does not refund or predict an allocation, collision, or hit outcome.
inline bool normal_gunman_beta_projectile_request(NormalGunmanBetaShotSource source,u8 firing_slot,u8 side1b04,
                                                 const WeaponStockModeZeroPlan& channel0_debit,
                                                 GunmanBetaProjectileRequest* out){
    GunmanBetaShotTraits traits;
    if(!out||firing_slot>=6||!normal_gunman_beta_shot_traits(source,&traits)||
       channel0_debit.operation!=WeaponStockOperation::Debit||channel0_debit.amount!=1)return false;
    GunmanBetaProjectileRequest next;
    if(channel0_debit.accepted)next={true,firing_slot,u8(traits.base_projectile+(side1b04?1u:0u))};
    *out=next;return true;
}
}
