#pragma once
#include "engine/gameplay/weapon_stock_mode_zero.hpp"

namespace gf {
struct SupportCasterObservation {
    u16 model=0;
    u8 variant=0,slot=0,team=0;
    constexpr bool is_patra()const{return model==0x0902&&!variant&&slot<6&&team<4;}
    constexpr bool is_nurse()const{return model==0x0900&&!variant&&slot<6&&team<4;}
};
struct SupportStockDebit {
    u8 channel=0,amount=0;
};
struct PatraSpellRelease {
    bool clear_program_events=false;
    SupportStockDebit debit;
};

// Big Spell release leaf after the caller has reached the reviewed class
// program. Only exact event (1,1) is consumed; X alone is not a cast.
inline bool normal_patra_spell_release(SupportCasterObservation caster,u8 event1cef,u8 event1cf0,
                                        bool reached_release_leaf,PatraSpellRelease* out){
    if(!out||!caster.is_patra()||!reached_release_leaf)return false;
    *out=event1cef==1&&event1cf0==1?PatraSpellRelease{true,{2,1}}:PatraSpellRelease{};
    return true;
}

struct PatraSpellCastRequest {
    bool requested=false;
    u8 caster_slot=0,object_type=0,argument=0;
    bool operator==(const PatraSpellCastRequest&)const=default;
};
// Request after the reviewed channel-2 debit, not an allocation receipt.
inline bool normal_patra_spell_cast(SupportCasterObservation caster,const WeaponStockModeZeroPlan& channel2_debit,
                                     PatraSpellCastRequest* out){
    if(!out||!caster.is_patra()||channel2_debit.operation!=WeaponStockOperation::Debit||channel2_debit.amount!=1)return false;
    *out=channel2_debit.accepted?PatraSpellCastRequest{true,caster.slot,60,0}:PatraSpellCastRequest{};
    return true;
}

struct NurseFallbackSetupPlan {
    u8 program_group=0,program=0;
    SupportStockDebit debit_after_request;
};
inline bool normal_nurse_fallback_setup(SupportCasterObservation caster,bool reached_fallback_leaf,NurseFallbackSetupPlan* out){
    if(!out||!caster.is_nurse()||!reached_fallback_leaf)return false;
    *out={4,0,{2,1}};return true;
}
struct NurseSetupStockPlan {
    u16 stock_latch548=0;
    bool write_failure6e8=false;
    u8 failure6e8=0;
};
inline bool normal_nurse_setup_stock(SupportCasterObservation caster,const WeaponStockModeZeroPlan& channel2_debit,
                                     NurseSetupStockPlan* out){
    if(!out||!caster.is_nurse()||channel2_debit.operation!=WeaponStockOperation::Debit||channel2_debit.amount!=1)return false;
    *out=channel2_debit.accepted?NurseSetupStockPlan{1,false,0}:NurseSetupStockPlan{0,true,8};
    return true;
}

struct NurseContactShapeRequest {
    bool requested=false;
    u8 caster_slot=0,shape_group=0;
};
// Actual fallback-program opcode, not the later visual cue or an assumed
// continuation after stock refusal. This does not register a collider.
inline bool normal_nurse_fallback_shape(SupportCasterObservation caster,bool reached_fallback_opcode,u8 lane_mask,
                                        NurseContactShapeRequest* out){
    if(!out||!caster.is_nurse()||!reached_fallback_opcode)return false;
    *out=(lane_mask&1u)?NurseContactShapeRequest{true,caster.slot,3}:NurseContactShapeRequest{};
    return true;
}

struct SupportContactRecipient {
    u8 slot=0,team=0,kind83=0;
    // Describes already resolved contact with the exact reviewed hit row.
    // It is not an overlap, registry/lifetime, or original-frame receipt.
    bool resolved_reviewed_contact=false;
};
struct PatraRecipientEffectRequest {
    u8 recipient_slot=0,effect_type=0,argument0=0,argument1=0;
    bool operator==(const PatraRecipientEffectRequest&)const=default;
};
// The reviewed type-60 cast's row 5 uses flag 4 and changes effect owner
// to the actual contacted actor. No enlargement magnitude/timer is supplied.
inline bool normal_patra_recipient_effect(SupportCasterObservation caster,u8 observed_carrier_type,u8 observed_hit_row,
                                           const SupportContactRecipient& recipient,PatraRecipientEffectRequest* out){
    if(!out||!caster.is_patra()||observed_carrier_type!=60||observed_hit_row!=5||
       !recipient.resolved_reviewed_contact||recipient.slot>=6||recipient.team>=4||recipient.kind83)return false;
    *out={recipient.slot,33,1,1};return true;
}

struct NurseRecipientHealth {
    s16 hp1c6=0,max_hp1c4=0;
    bool ordinary_nonfused=false;
};
struct NurseHealingPlan {
    bool write_hp=false;
    u8 recipient_slot=0;
    s16 hp1c6=0,previous_hp1c8=0;
    bool operator==(const NurseHealingPlan&)const=default;
};
// Selected same-team row 3 only, after contact arbitration. No targeting,
// cross-team scaling, fused state, revive or global-party heal is inferred.
inline bool normal_nurse_healing_contact(SupportCasterObservation caster,u8 observed_hit_row,
                                         const SupportContactRecipient& recipient,const NurseRecipientHealth& health,
                                         u8 global_blocker1f,NurseHealingPlan* out){
    if(!out||!caster.is_nurse()||observed_hit_row!=3||!recipient.resolved_reviewed_contact||recipient.slot>=6||
       recipient.team!=caster.team||recipient.kind83||!health.ordinary_nonfused||health.max_hp1c4<=0||
       health.hp1c6<0||health.hp1c6>health.max_hp1c4)return false;
    NurseHealingPlan next;
    if(health.hp1c6&&!global_blocker1f){
        const s32 healed=s32(health.hp1c6)+200;
        if(healed>32767)return false; // no unreviewed signed-halfword overflow
        next={true,recipient.slot,s16(healed>health.max_hp1c4?health.max_hp1c4:healed),health.hp1c6};
    }
    *out=next;return true;
}
}
