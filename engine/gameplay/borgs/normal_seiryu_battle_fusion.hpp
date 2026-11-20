#pragma once
#include "engine/gameplay/battle_fusion_handshake.hpp"

namespace gf {
using SeiryuBattleRole=BattleFusionRole;
using SeiryuMatchCandidate=BattleFusionMatchCandidate;
using SeiryuStagedSlot=BattleFusionStagedSlot;
using SeiryuMatchPlan=BattleFusionMatchPlan;
using SeiryuReloadPlan=BattleFusionReloadPlan;
using SeiryuTeamClock=BattleFusionTeamClock;
using SeiryuTeamClockStep=BattleFusionTeamClockStep;
using SeiryuRestoreOwner=BattleFusionRestoreOwner;
using SeiryuRestorePlan=BattleFusionRestorePlan;

inline bool seiryu_request_latch(u8 current6fb,u8 team_ready0f,u32 commands5d4,u8* out){
    return battle_fusion_request_latch(current6fb,team_ready0f,commands5d4,out);
}
inline bool normal_seiryu_match(const std::array<SeiryuMatchCandidate,2>& candidates,bool admitted_team_list,SeiryuMatchPlan* out){
    return battle_fusion_detail::match(battle_fusion_detail::seiryu,candidates,admitted_team_list,out);
}
inline bool normal_seiryu_stage7_reload(const SeiryuMatchPlan& linked,const std::array<u8,2>& stages4a1,
                                        bool current_links_observed,SeiryuReloadPlan* out){
    return battle_fusion_detail::stage7_reload(battle_fusion_detail::seiryu,linked,stages4a1,current_links_observed,out);
}
inline bool seiryu_fused_init_clock(SeiryuBattleRole role,SeiryuTeamClock* out){
    return battle_fusion_fused_init_clock(role,out);
}
inline bool seiryu_team_clock_step(const SeiryuTeamClock& in,bool eligible_update,SeiryuTeamClockStep* out){
    return battle_fusion_team_clock_step(in,eligible_update,out);
}
inline bool normal_seiryu_restore_request(const SeiryuRestoreOwner& caller,const SeiryuRestoreOwner& partner,
                                          bool current_links_observed,u8 team_active10,u8 global_blocker1f,SeiryuRestorePlan* out){
    return battle_fusion_detail::restore_request(battle_fusion_detail::seiryu,caller,partner,current_links_observed,
                                               team_active10,global_blocker1f,out);
}
inline bool normal_seiryu_restore_cleanup(const SeiryuRestoreOwner& owner,const BattleFusionRestoreCleanupFields& in,
                                          BattleFusionRestoreCleanupPlan* out){
    return battle_fusion_detail::restore_cleanup(battle_fusion_detail::seiryu,owner,in,out);
}
}
