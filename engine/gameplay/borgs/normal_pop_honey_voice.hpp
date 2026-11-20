#pragma once
#include "engine/gameplay/weapon_stock_mode_zero.hpp"

namespace gf {
struct NormalPopHoneySource {
    u16 model=0;
    u8 variant=0,slot=0;
    constexpr bool reviewed()const{return model==0x0904&&!variant&&slot<6;}
};
struct PopHoneyVoiceSetupPlan {
    WeaponStockModeZeroState stock;
    u8 paid544=0,refill_hold597=0;
};
// Exact neutral setup debit. A failed debit does not imply that the
// separate startup motion/hit was skipped.
inline bool normal_pop_honey_voice_setup(NormalPopHoneySource source,bool reached_neutral_setup,u8 refill_hold597,
                                         const WeaponStockModeZeroState& channel2,PopHoneyVoiceSetupPlan* out){
    WeaponStockModeZeroPlan next;
    if(!out||!source.reviewed()||!reached_neutral_setup||
       !weapon_stock_mode_zero_plan(channel2,WeaponStockOperation::Debit,1,&next))return false;
    *out={next.state,u8(next.accepted),next.accepted?u8(refill_hold597|4u):refill_hold597};return true;
}

struct PopHoneyPaidVoiceProgram {
    bool requested=false;
    u8 mask=0,group=0,program=0;
};
inline bool normal_pop_honey_paid_voice_program(NormalPopHoneySource source,u8 paid544,bool startup_complete,
                                                PopHoneyPaidVoiceProgram* out){
    if(!out||!source.reviewed()||paid544>1)return false;
    *out=paid544&&startup_complete?PopHoneyPaidVoiceProgram{true,15,4,1}:PopHoneyPaidVoiceProgram{};
    return true;
}

struct PopHoneyNoteShapeRequest {
    u8 credited_caster_slot=0,shape_group=0,hit_row=0;
};
// The observed note object is the geometric carrier; its lineage supplies
// credit. These record choices do not create a note, collider or audio.
inline bool normal_pop_honey_note_shape(NormalPopHoneySource source,u8 object_type,u8 record_index,
                                        bool observed_owned_note,PopHoneyNoteShapeRequest* out){
    if(!out||!source.reviewed()||!observed_owned_note||object_type!=62||record_index>=14)return false;
    *out={source.slot,8,5};return true;
}

inline constexpr s16 kPopHoneyStatusTicks=641;
struct PopHoneyStatusSnapshot {
    u8 slot=0;
    s16 timer6b8=0;
    bool observed_current_recipient=false;
    constexpr bool reviewed()const{return observed_current_recipient&&slot<6&&timer6b8>=0&&timer6b8<=kPopHoneyStatusTicks;}
};
struct PopHoneyNoteContact {
    u8 object_type=0,credited_slot=0,hit_row=0,recipient_kind83=0,recipient_hyper6fc=0,recipient_fused4a0=0;
    u16 recipient_mask59e=0;
    bool resolved_reviewed_contact=false;
    PopHoneyStatusSnapshot recipient;
};
struct PopHoneyStatusApplication {
    PopHoneyStatusSnapshot updated;
    bool writes_timer=false,request_recipient_presentation=false;
};
// Ordinary row-5 contact only. Caller owns collision admission/recipient
// lifetime; there is no lock-target override, team sweep, or full inversion.
inline bool normal_pop_honey_note_status(NormalPopHoneySource source,const PopHoneyNoteContact& in,
                                         PopHoneyStatusApplication* out){
    if(!out||!source.reviewed()||!in.resolved_reviewed_contact||in.object_type!=62||
       in.credited_slot!=source.slot||in.hit_row!=5||!in.recipient.reviewed())return false;
    PopHoneyStatusApplication next{in.recipient,false,false};
    if(!(in.recipient_mask59e&0x20u)&&!in.recipient_kind83&&!in.recipient_hyper6fc&&!in.recipient_fused4a0){
        next.writes_timer=true;
        next.request_recipient_presentation=!in.recipient.timer6b8;
        next.updated.timer6b8=kPopHoneyStatusTicks;
    }
    *out=next;return true;
}

inline bool pop_honey_status_bookkeeping(const PopHoneyStatusSnapshot& in,u8 entry4a1,u8 hold71c,
                                         PopHoneyStatusSnapshot* out){
    if(!out||!in.reviewed())return false;
    auto next=in;
    if(next.timer6b8){
        if(entry4a1)next.timer6b8=0;
        else if(!hold71c)--next.timer6b8;
    }
    *out=next;return true;
}

struct PopHoneyTargetingPlan {
    u8 recipient_slot=0,sector=0;
    bool clear_active_aim_snapshot=false;
};
// Only the inspected aim and sector consumers. No input-axis changes,
// steering, team change, or removal of unrelated selected-target state.
inline bool pop_honey_status_targeting(const PopHoneyStatusSnapshot& in,u8 sector,PopHoneyTargetingPlan* out){
    if(!out||!in.reviewed()||(sector!=1&&sector!=2&&sector!=4&&sector!=8))return false;
    PopHoneyTargetingPlan next{in.slot,sector,in.timer6b8!=0};
    if(in.timer6b8&&sector==2)next.sector=8;
    else if(in.timer6b8&&sector==8)next.sector=2;
    *out=next;return true;
}
}
