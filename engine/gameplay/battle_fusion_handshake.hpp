#pragma once
#include <array>
#include "engine/core/types.hpp"

namespace gf {
enum class BattleFusionRole:u8 {None,Primary,Companion};
namespace battle_fusion_detail {
struct NormalPair {
    u16 first,second,primary,companion;
    constexpr bool original(u16 model)const{return model==first||model==second;}
    constexpr u16 destination(BattleFusionRole role)const{
        return role==BattleFusionRole::Primary?primary:role==BattleFusionRole::Companion?companion:0;
    }
};
inline constexpr NormalPair seiryu{0x0600,0x0601,0x0602,0x0603};
inline constexpr NormalPair machine_dragon{0x0511,0x0512,0x0513,0x0514};
inline constexpr s16 reference_ticks=1800;
}

// Shared input leaf on the reviewed battle route. The current latch is not
// aged here, and its value establishes neither a partner nor actor admission.
inline bool battle_fusion_request_latch(u8 current6fb,u8 team_ready0f,u32 commands5d4,u8* out){
    if(!out||current6fb>6)return false;
    *out=team_ready0f&&(commands5d4&0x200u)?6:current6fb;return true;
}

struct BattleFusionMatchCandidate {
    u8 slot=0,team=0;
    u16 current_model=0;
    u8 selected_variant=0,role_hint4a8=0;
    bool ai=false;
};
struct BattleFusionStagedSlot {
    u8 slot=0,partner_slot=0;
    u16 pending_model48c=0;
    u8 pending_variant48e=0;
    BattleFusionRole role4a9=BattleFusionRole::None;
    bool operator==(const BattleFusionStagedSlot&)const=default;
};
struct BattleFusionMatchPlan {
    std::array<BattleFusionStagedSlot,2> slots{};
    u8 returned_anchor_slot=0;
    bool operator==(const BattleFusionMatchPlan&)const=default;
};
namespace battle_fusion_detail {
// Two-entry matcher only, after the caller's actual team/list filtering.
// No generic Hyper fallback and no substitution of a third entry or variant.
inline bool match(const NormalPair& pair,const std::array<BattleFusionMatchCandidate,2>& candidates,bool admitted_team_list,
                  BattleFusionMatchPlan* out){
    if(!out||!admitted_team_list||candidates[0].slot==candidates[1].slot||
       candidates[0].team!=candidates[1].team||candidates[0].current_model==candidates[1].current_model)return false;
    for(const auto& c:candidates)if(c.slot>=6||c.team>=4||!pair.original(c.current_model)||
        c.selected_variant||c.role_hint4a8!=(c.current_model==pair.first?1:2))return false;
    const unsigned first_role=candidates[0].current_model==pair.first?0:1;
    const unsigned second_role=1-first_role;
    const unsigned primary_index=candidates[first_role].ai&&!candidates[second_role].ai?second_role:first_role;
    const unsigned anchor=candidates[0].ai&&!candidates[1].ai?1:0;
    BattleFusionMatchPlan next;next.returned_anchor_slot=candidates[anchor].slot;
    for(unsigned i=0;i<2;++i){
        const auto role=i==primary_index?BattleFusionRole::Primary:BattleFusionRole::Companion;
        next.slots[i]={candidates[i].slot,candidates[1-i].slot,pair.destination(role),candidates[first_role].selected_variant,role};
    }
    *out=next;return true;
}
}

struct BattleFusionReloadPlan {
    bool stage_pair_reload=false;
    std::array<BattleFusionStagedSlot,2> slots{};
};
namespace battle_fusion_detail {
// Stage-seven gate on still-current, already-linked slots, not a resource
// receipt. The slot lifecycle and primary-owned paired reload remain due.
inline bool stage7_reload(const NormalPair& pair,const BattleFusionMatchPlan& linked,const std::array<u8,2>& stages4a1,
                          bool current_links_observed,BattleFusionReloadPlan* out){
    const auto& a=linked.slots[0];const auto& b=linked.slots[1];
    if(!out||!current_links_observed||a.slot>=6||b.slot>=6||a.slot==b.slot||a.partner_slot!=b.slot||b.partner_slot!=a.slot||
       a.role4a9==b.role4a9||!pair.destination(a.role4a9)||!pair.destination(b.role4a9)||
       a.pending_model48c!=pair.destination(a.role4a9)||b.pending_model48c!=pair.destination(b.role4a9)||
       a.pending_variant48e||b.pending_variant48e||stages4a1[0]>7||stages4a1[1]>7)return false;
    BattleFusionReloadPlan next;
    if(stages4a1[0]==7&&stages4a1[1]==7)next={true,linked.slots};
    *out=next;return true;
}
}

struct BattleFusionTeamClock {
    u8 ready0f=0,active10=0;
    s16 remaining18=0,reference1a=0;
    bool operator==(const BattleFusionTeamClock&)const=default;
};
inline bool battle_fusion_fused_init_clock(BattleFusionRole role,BattleFusionTeamClock* out){
    if(!out||(role!=BattleFusionRole::Primary&&role!=BattleFusionRole::Companion))return false;
    using battle_fusion_detail::reference_ticks;
    *out={0,2,reference_ticks,reference_ticks};return true;
}
struct BattleFusionTeamClockStep {
    BattleFusionTeamClock clock;
    bool clear_meter32=false;
};
// Caller qualifies the team update (including pause/mode/mask). This clock
// never changes either owner; only expiry's zero-meter write is proposed.
inline bool battle_fusion_team_clock_step(const BattleFusionTeamClock& in,bool eligible_update,BattleFusionTeamClockStep* out){
    if(!out||in.active10!=2||in.remaining18<=0||in.remaining18>in.reference1a||
       in.reference1a!=battle_fusion_detail::reference_ticks)return false;
    BattleFusionTeamClockStep next{in,false};
    if(eligible_update){
        --next.clock.remaining18;
        if(!next.clock.remaining18){next.clock.active10=0;next.clear_meter32=true;}
    }
    *out=next;return true;
}

struct BattleFusionRestoreOwner {
    u8 slot=0,partner_slot=0,battle_state18=0,action_state584=0;
    u32 flags5e0=0;
    BattleFusionRole role4a0=BattleFusionRole::None;
    u16 current_model=0,saved_model3ea=0;
    u8 current_variant=0,saved_variant3ed=0,saved_ancillary3ef=0;
};
struct BattleFusionRestorePlan {
    bool requested=false;
    u8 slot=0,mode490=0;
    u16 pending_model48c=0;
    u8 pending_variant48e=0,pending_ancillary48f=0;
    bool operator==(const BattleFusionRestorePlan&)const=default;
};
namespace battle_fusion_detail {
inline bool valid_restore_owner(const NormalPair& pair,const BattleFusionRestoreOwner& in){
    return in.slot<6&&pair.destination(in.role4a0)&&in.current_model==pair.destination(in.role4a0)&&
        pair.original(in.saved_model3ea)&&!in.current_variant&&!in.saved_variant3ed;
}
// Per-caller mode-4 request prefix after the owner's live-link check. A
// partner can be waiting for its own lifecycle; no two-slot atomic split,
// HP transfer, role/link clear or final model publication comes from this.
inline bool restore_request(const NormalPair& pair,const BattleFusionRestoreOwner& caller,const BattleFusionRestoreOwner& partner,
                            bool current_links_observed,u8 team_active10,u8 global_blocker1f,BattleFusionRestorePlan* out){
    if(!out||!current_links_observed||!valid_restore_owner(pair,caller)||!valid_restore_owner(pair,partner)||caller.slot==partner.slot||
       caller.partner_slot!=partner.slot||partner.partner_slot!=caller.slot||caller.role4a0==partner.role4a0||
       caller.saved_model3ea==partner.saved_model3ea)return false;
    BattleFusionRestorePlan next;
    if(!team_active10&&!global_blocker1f&&caller.battle_state18==1&&
       !((caller.flags5e0&3u)&&caller.action_state584==5)&&!((caller.flags5e0|partner.flags5e0)&0x91001000u))
        next={true,caller.slot,4,caller.saved_model3ea,caller.saved_variant3ed,caller.saved_ancillary3ef};
    *out=next;return true;
}
}

struct BattleFusionRestoreCleanupFields {
    u16 pending_model48c=0;
    u8 pending_variant48e=0,mode490=0,skip48a=0;
    bool current_slot_observed=false;
};
struct BattleFusionRestoreCleanupPlan {
    bool clear_role_and_partner=false,request_single_slot_reload=false;
    u8 slot=0;
    u16 reload_model=0;
    u8 reload_variant=0;
    bool operator==(const BattleFusionRestoreCleanupPlan&)const=default;
};
namespace battle_fusion_detail {
// Mode-4 block after the ledger writes; those writes are not modeled here.
// 48A != 0 bypasses cleanup. Only the observed slot is considered: a partner
// may already have cleared its side. No resource load or link is mutated.
inline bool restore_cleanup(const NormalPair& pair,const BattleFusionRestoreOwner& owner,
                            const BattleFusionRestoreCleanupFields& in,BattleFusionRestoreCleanupPlan* out){
    if(!out||!in.current_slot_observed||!valid_restore_owner(pair,owner)||in.mode490!=4||
       in.pending_model48c!=owner.saved_model3ea||in.pending_variant48e!=owner.saved_variant3ed)return false;
    BattleFusionRestoreCleanupPlan next;
    if(!in.skip48a)next={true,true,owner.slot,in.pending_model48c,in.pending_variant48e};
    *out=next;return true;
}
}
}
