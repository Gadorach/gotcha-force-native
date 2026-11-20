#pragma once
#include "engine/core/types.hpp"

namespace gf {
struct CopyManModel {
    u16 model=0;
    u8 variant=0;
    bool operator==(const CopyManModel&)const=default;
};
inline constexpr CopyManModel kNormalCopyMan{0x0806,0},kReviewedCopyDonor{0x000a,0};
inline constexpr s16 kCopyManCopiedTicks=900;

struct CopyManHeadRequest {
    bool advance_phase=false,request_head=false;
    u8 owner_slot=0,object_type=0,argument=0;
};
// The reviewed charged-X event does not debit ordinary stock. A live
// allocator owns the head latch, not this class proposal.
inline bool normal_copy_man_head_request(CopyManModel caster,u8 slot,u8 event1cef,u8 event1cf0,u8 head_latch144,
                                         bool reached_release_leaf,CopyManHeadRequest* out){
    if(!out||caster!=kNormalCopyMan||slot>=6||!reached_release_leaf)return false;
    CopyManHeadRequest next;
    if(event1cef==1&&event1cf0==1){
        next.advance_phase=true;
        if(!head_latch144)next={true,true,slot,85,0};
    }
    *out=next;return true;
}

struct CopyManContactObservation {
    u8 caster_slot=0,recipient_slot=0,carrier_type=0,hit_row=0,recipient_fused4a0=0;
    u16 recipient_mask59e=0;
    bool reviewed_contact_resolved=false;
};
struct CopyManContactDonor {
    bool recorded=false;
    u8 caster_slot=0,donor_slot=0;
};
// The contact scratch request stores the resolved recipient slot, not its
// model, aim snapshot, or a pointer with authority over future updates.
inline bool normal_copy_man_record_donor(CopyManModel caster,const CopyManContactObservation& in,
                                         CopyManContactDonor* out){
    if(!out||caster!=kNormalCopyMan||!in.reviewed_contact_resolved||in.carrier_type!=85||in.hit_row||
       in.caster_slot>=6||in.recipient_slot>=6||in.caster_slot==in.recipient_slot)return false;
    *out=(!(in.recipient_mask59e&0x2000u)&&!in.recipient_fused4a0)?
        CopyManContactDonor{true,in.caster_slot,in.recipient_slot}:CopyManContactDonor{};
    return true;
}

struct CopyManOwnerObservation {
    u8 slot=0,battle_state18=0,special48a=0,current_level_index=0,original_level_index=0;
    CopyManModel current,original;
    s16 current_hp1c6=0;
    bool observed_current_owner=false;
};
struct CopyManDonorObservation {
    u8 slot=0,special48a=0,fused4a0=0;
    CopyManModel current;
    bool observed_current_donor=false;
};
struct CopyManReloadPlan {
    bool requested=false;
    u8 slot=0,mode490=0,battle_state18=0,phase19=0,copy_state489=0;
    CopyManModel pending;
    u8 level_index48f=0;
    s16 saved_current_hp494=0,seed49a=0;
    bool operator==(const CopyManReloadPlan&)const=default;
};
namespace copy_man_detail {
inline bool valid(const CopyManOwnerObservation& owner){
    return owner.observed_current_owner&&owner.slot<6&&owner.original==kNormalCopyMan&&!owner.special48a&&
        owner.current_level_index<10&&owner.original_level_index<10&&owner.current_hp1c6>=0;
}
}

// Scope is the exact normal Copy Man -> Sasuke data witness only. The
// donor form tuple must still be current when the caster consumes contact.
// No live objects, pointers, movement state or donor stats are copied.
inline bool normal_copy_man_reload_request(const CopyManOwnerObservation& owner,const CopyManContactDonor& contact,
                                           const CopyManDonorObservation& donor,bool selected_post_hit_route,CopyManReloadPlan* out){
    if(!out||!copy_man_detail::valid(owner)||owner.current!=kNormalCopyMan||!selected_post_hit_route||
       !contact.recorded||contact.caster_slot!=owner.slot||contact.donor_slot!=donor.slot||donor.slot>=6||
       donor.slot==owner.slot||!donor.observed_current_donor||donor.special48a||donor.fused4a0||
       donor.current!=kReviewedCopyDonor)return false;
    CopyManReloadPlan next;
    if(owner.battle_state18<2&&owner.current_hp1c6>0)
        next={true,owner.slot,1,2,2,1,donor.current,owner.current_level_index,owner.current_hp1c6,kCopyManCopiedTicks};
    *out=next;return true;
}

struct CopyManExpiryInput {
    s16 ticks49a=0;
    u8 mode490=0,transition4a1=0,fused4a0=0,hyper6fc=0;
    u32 flags5e0=0;
};
struct CopyManExpiryStep {
    s16 ticks49a=0;
    bool request_return=false;
    u8 slot=0;
    bool operator==(const CopyManExpiryStep&)const=default;
};
inline bool normal_copy_man_expiry_step(const CopyManOwnerObservation& owner,const CopyManExpiryInput& in,CopyManExpiryStep* out){
    if(!out||!copy_man_detail::valid(owner)||owner.current!=kReviewedCopyDonor||in.ticks49a<0||
       in.ticks49a>kCopyManCopiedTicks)return false;
    CopyManExpiryStep next{in.ticks49a,false,owner.slot};
    if(in.mode490==2&&!in.transition4a1&&!in.fused4a0&&!in.hyper6fc&&next.ticks49a>0){
        --next.ticks49a;
        if(!next.ticks49a){
            if(in.flags5e0&0x91001000u)next.ticks49a=1;
            else next.request_return=true;
        }
    }
    *out=next;return true;
}
inline bool normal_copy_man_return_request(const CopyManOwnerObservation& owner,const CopyManExpiryStep& expiry,
                                           CopyManReloadPlan* out){
    if(!out||!copy_man_detail::valid(owner)||owner.current!=kReviewedCopyDonor||!owner.current_hp1c6||
       !expiry.request_return||expiry.ticks49a||expiry.slot!=owner.slot)return false;
    *out={true,owner.slot,4,2,3,4,owner.original,owner.original_level_index,owner.current_hp1c6,0};
    return true;
}

struct CopyManInstalledStats {
    u8 slot=0,level_index=0;
    CopyManModel installed;
    s16 max_hp1c4=0;
    bool observed_reloaded_slot=false;
};
struct CopyManHealthPlan {
    s16 hp1c6=0,previous_hp1c8=0;
    bool operator==(const CopyManHealthPlan&)const=default;
};
// Scalar non-fused re-entry prefix. The matching resource/owner is supplied
// separately. Absolute caster HP is capped; no donor HP or ratio is read.
inline bool normal_copy_man_reload_health(const CopyManReloadPlan& plan,const CopyManInstalledStats& stats,CopyManHealthPlan* out){
    if(!out||!stats.observed_reloaded_slot||!plan.requested||plan.slot>=6||stats.slot!=plan.slot||
       stats.installed!=plan.pending||stats.level_index!=plan.level_index48f||plan.level_index48f>=10||
       (plan.mode490!=1&&plan.mode490!=4)||plan.pending!=(plan.mode490==1?kReviewedCopyDonor:kNormalCopyMan)||
       stats.max_hp1c4<=0||plan.saved_current_hp494<=0)return false;
    const s16 hp=plan.saved_current_hp494<stats.max_hp1c4?plan.saved_current_hp494:stats.max_hp1c4;
    *out={hp,hp};return true;
}
}
