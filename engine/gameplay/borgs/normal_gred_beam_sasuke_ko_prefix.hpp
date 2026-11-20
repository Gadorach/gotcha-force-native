#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_post_hit.hpp"

namespace gf {
struct GRedBeamSasukeKoInput {
    bool direct_mode_zero_declared=false;
    u8 credit_slot=255,receiver_slot=255,credit_team88=255,receiver_team88=255;
    u8 context32=255,scene_hold1f=0,receiver_selector3e6=0,receiver_link4a0=0;
    // Declared runtime cost; neither an HP amount nor a catalogue fallback.
    s16 receiver_cost4aa=-1;
    s32 receiver_team_remaining118=0;
    u8 debit108=0,reserve_enabled109=0,receiver_flag1ea=0;
    s8 reserves10a=0;
    u8 scene_credit_b4=255,scene_receiver_b8=255;
    u8 receiver_losses435=0,credit_kos434=0;
    u32 receiver_loss_cost424=0,credit_ko_cost420=0;
    u16 credit_team_bonus=0;
};
struct GRedBeamSasukeKoPlan {
    s32 receiver_team_remaining118=0;
    s8 reserves10a=0;
    u8 scene_credit_b4=255,scene_receiver_b8=255;
    u8 receiver_losses435=0,credit_kos434=0;
    u32 receiver_loss_cost424=0,credit_ko_cost420=0;
    u16 credit_team_bonus=0;
    bool team_debited=false,team_zero_store=false,reserve_decremented=false,scene_pair_requested=false;
    bool observer_scan_pending=true;
};

// Ordinary mode-0 2F8DC..2FB14 prefix, after a declared zero crossing.
// Slots below represent distinct actors in the caller's fixture, not
// source pointers, native leases, or permission to update a scene.
inline bool normal_gred_beam_sasuke_ko_prefix(NormalGRedNeutralBSource source,
    const GRedBeamSasukePostHitPlan& hit,const GRedBeamSasukeKoInput& in,GRedBeamSasukeKoPlan* out){
    const auto slot_or_empty=[](u8 slot){return slot<6||slot==255;};
    if(!out||!source.reviewed()||!in.direct_mode_zero_declared||in.credit_slot!=source.slot||in.receiver_slot>=6||
       in.receiver_slot==source.slot||hit.credit_slot!=in.credit_slot||hit.receiver_slot!=in.receiver_slot||
       !hit.request_ko_continuation||!hit.health.write_health||!hit.health.reaches_zero||
       hit.health.current1c6||hit.health.previous1c8<=0||hit.health.amount<hit.health.previous1c8||
       in.credit_team88||in.receiver_team88!=1||in.context32||in.scene_hold1f||in.receiver_selector3e6||
       in.receiver_link4a0||in.receiver_cost4aa<0||in.receiver_team_remaining118<0||
       !slot_or_empty(in.scene_credit_b4)||!slot_or_empty(in.scene_receiver_b8))return false;
    GRedBeamSasukeKoPlan next;
    next.receiver_team_remaining118=in.receiver_team_remaining118;next.reserves10a=in.reserves10a;
    next.scene_credit_b4=in.scene_credit_b4;next.scene_receiver_b8=in.scene_receiver_b8;
    if(in.debit108){
        next.team_debited=true;next.receiver_team_remaining118-=s32(in.receiver_cost4aa);
    }
    if(in.reserve_enabled109&&in.receiver_flag1ea&&in.reserves10a>0){
        next.reserves10a=s8(in.reserves10a-1);next.reserve_decremented=true;next.scene_pair_requested=true;
    }
    if(next.receiver_team_remaining118<=0){
        next.receiver_team_remaining118=0;next.team_zero_store=true;next.scene_pair_requested=true;
    }
    if(next.scene_pair_requested){next.scene_credit_b4=in.credit_slot;next.scene_receiver_b8=in.receiver_slot;}
    next.receiver_losses435=u8(in.receiver_losses435+1u);next.credit_kos434=u8(in.credit_kos434+1u);
    next.receiver_loss_cost424=in.receiver_loss_cost424+u32(in.receiver_cost4aa);
    next.credit_ko_cost420=in.credit_ko_cost420+u32(in.receiver_cost4aa);
    next.credit_team_bonus=u16(in.credit_team_bonus+100u);
    *out=next;return true;
}
}
