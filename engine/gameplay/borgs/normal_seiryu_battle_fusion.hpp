#pragma once
#include <array>
#include "engine/core/types.hpp"

namespace gf {
enum class SeiryuBattleRole:u8 {None,Primary,Companion};
namespace seiryu_fusion_detail {
inline constexpr u16 red=0x0600,blue=0x0601,primary=0x0602,companion=0x0603;
inline constexpr s16 reference_ticks=1800;
constexpr bool original(u16 model){return model==red||model==blue;}
constexpr u16 destination(SeiryuBattleRole role){
    return role==SeiryuBattleRole::Primary?primary:role==SeiryuBattleRole::Companion?companion:0;
}
}

// Shared input leaf on the reviewed battle route. The current latch is not
// aged here, and its value establishes neither a partner nor actor admission.
inline bool seiryu_request_latch(u8 current6fb,u8 team_ready0f,u32 commands5d4,u8* out){
    if(!out||current6fb>6)return false;
    *out=team_ready0f&&(commands5d4&0x200u)?6:current6fb;return true;
}

struct SeiryuMatchCandidate {
    u8 slot=0,team=0;
    u16 current_model=0;
    u8 selected_variant=0,role_hint4a8=0;
    bool ai=false;
};
struct SeiryuStagedSlot {
    u8 slot=0,partner_slot=0;
    u16 pending_model48c=0;
    u8 pending_variant48e=0;
    SeiryuBattleRole role4a9=SeiryuBattleRole::None;
    bool operator==(const SeiryuStagedSlot&)const=default;
};
struct SeiryuMatchPlan {
    std::array<SeiryuStagedSlot,2> slots{};
    u8 returned_anchor_slot=0;
    bool operator==(const SeiryuMatchPlan&)const=default;
};
// Two-entry matcher only, after the caller's actual team/list filtering.
// No generic Hyper fallback and no substitution of a third entry or variant.
inline bool normal_seiryu_match(const std::array<SeiryuMatchCandidate,2>& candidates,bool admitted_team_list,
                                SeiryuMatchPlan* out){
    using namespace seiryu_fusion_detail;
    if(!out||!admitted_team_list||candidates[0].slot==candidates[1].slot||
       candidates[0].team!=candidates[1].team||candidates[0].current_model==candidates[1].current_model)return false;
    for(const auto& c:candidates)if(c.slot>=6||c.team>=4||!original(c.current_model)||
        c.selected_variant||c.role_hint4a8!=(c.current_model==red?1:2))return false;
    const unsigned red_index=candidates[0].current_model==red?0:1;
    const unsigned blue_index=1-red_index;
    const unsigned primary_index=candidates[red_index].ai&&!candidates[blue_index].ai?blue_index:red_index;
    const unsigned anchor=candidates[0].ai&&!candidates[1].ai?1:0;
    SeiryuMatchPlan next;next.returned_anchor_slot=candidates[anchor].slot;
    for(unsigned i=0;i<2;++i){
        const auto role=i==primary_index?SeiryuBattleRole::Primary:SeiryuBattleRole::Companion;
        next.slots[i]={candidates[i].slot,candidates[1-i].slot,destination(role),candidates[red_index].selected_variant,role};
    }
    *out=next;return true;
}

struct SeiryuReloadPlan {
    bool stage_pair_reload=false;
    std::array<SeiryuStagedSlot,2> slots{};
};
// Stage-seven gate on still-current, already-linked slots, not a resource
// receipt. The slot lifecycle and primary-owned paired reload remain due.
inline bool normal_seiryu_stage7_reload(const SeiryuMatchPlan& linked,const std::array<u8,2>& stages4a1,
                                        bool current_links_observed,SeiryuReloadPlan* out){
    using namespace seiryu_fusion_detail;
    const auto& a=linked.slots[0];const auto& b=linked.slots[1];
    if(!out||!current_links_observed||a.slot>=6||b.slot>=6||a.slot==b.slot||a.partner_slot!=b.slot||b.partner_slot!=a.slot||
       a.role4a9==b.role4a9||!destination(a.role4a9)||!destination(b.role4a9)||
       a.pending_model48c!=destination(a.role4a9)||b.pending_model48c!=destination(b.role4a9)||
       a.pending_variant48e||b.pending_variant48e||stages4a1[0]>7||stages4a1[1]>7)return false;
    SeiryuReloadPlan next;
    if(stages4a1[0]==7&&stages4a1[1]==7)next={true,linked.slots};
    *out=next;return true;
}

struct SeiryuTeamClock {
    u8 ready0f=0,active10=0;
    s16 remaining18=0,reference1a=0;
    bool operator==(const SeiryuTeamClock&)const=default;
};
inline bool seiryu_fused_init_clock(SeiryuBattleRole role,SeiryuTeamClock* out){
    using namespace seiryu_fusion_detail;
    if(!out||!destination(role))return false;
    *out={0,2,reference_ticks,reference_ticks};return true;
}
struct SeiryuTeamClockStep {
    SeiryuTeamClock clock;
    bool clear_meter32=false;
};
// Caller qualifies the team update (including pause/mode/mask). This clock
// never changes either owner; only expiry's zero-meter write is proposed.
inline bool seiryu_team_clock_step(const SeiryuTeamClock& in,bool eligible_update,SeiryuTeamClockStep* out){
    if(!out||in.active10!=2||in.remaining18<=0||in.remaining18>in.reference1a||
       in.reference1a!=seiryu_fusion_detail::reference_ticks)return false;
    SeiryuTeamClockStep next{in,false};
    if(eligible_update){
        --next.clock.remaining18;
        if(!next.clock.remaining18){next.clock.active10=0;next.clear_meter32=true;}
    }
    *out=next;return true;
}

struct SeiryuRestoreOwner {
    u8 slot=0,partner_slot=0,battle_state18=0,action_state584=0;
    u32 flags5e0=0;
    SeiryuBattleRole role4a0=SeiryuBattleRole::None;
    u16 current_model=0,saved_model3ea=0;
    u8 current_variant=0,saved_variant3ed=0,saved_ancillary3ef=0;
};
struct SeiryuRestorePlan {
    bool requested=false;
    u8 slot=0,mode490=0;
    u16 pending_model48c=0;
    u8 pending_variant48e=0,pending_ancillary48f=0;
    bool operator==(const SeiryuRestorePlan&)const=default;
};
// Per-caller mode-4 request prefix after the owner's live-link check. A
// partner can be waiting for its own lifecycle; no two-slot atomic split,
// HP transfer, role/link clear or final model publication comes from this.
inline bool normal_seiryu_restore_request(const SeiryuRestoreOwner& caller,const SeiryuRestoreOwner& partner,
                                          bool current_links_observed,u8 team_active10,u8 global_blocker1f,
                                          SeiryuRestorePlan* out){
    using namespace seiryu_fusion_detail;
    if(!out||!current_links_observed||caller.slot>=6||partner.slot>=6||caller.slot==partner.slot||
       caller.partner_slot!=partner.slot||partner.partner_slot!=caller.slot||caller.role4a0==partner.role4a0||
       !destination(caller.role4a0)||!destination(partner.role4a0)||caller.current_model!=destination(caller.role4a0)||
       partner.current_model!=destination(partner.role4a0)||!original(caller.saved_model3ea)||!original(partner.saved_model3ea)||
       caller.saved_model3ea==partner.saved_model3ea||caller.current_variant||partner.current_variant||
       caller.saved_variant3ed||partner.saved_variant3ed)return false;
    SeiryuRestorePlan next;
    if(!team_active10&&!global_blocker1f&&caller.battle_state18==1&&
       !((caller.flags5e0&3u)&&caller.action_state584==5)&&!((caller.flags5e0|partner.flags5e0)&0x91001000u))
        next={true,caller.slot,4,caller.saved_model3ea,caller.saved_variant3ed,caller.saved_ancillary3ef};
    *out=next;return true;
}
}
