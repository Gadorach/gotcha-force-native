#pragma once
#include "engine/gameplay/battle_fusion_handshake.hpp"

namespace gf {
// Catalogue 050F is not the primary battle destination. The exact normal
// handshake instead binds 0511/0512 to 0513/0514 in the existing two slots.
inline bool normal_machine_dragon_match(const std::array<BattleFusionMatchCandidate,2>& candidates,bool admitted_team_list,
                                         BattleFusionMatchPlan* out){
    return battle_fusion_detail::match(battle_fusion_detail::machine_dragon,candidates,admitted_team_list,out);
}
inline bool normal_machine_dragon_stage7_reload(const BattleFusionMatchPlan& linked,const std::array<u8,2>& stages4a1,
                                                bool current_links_observed,BattleFusionReloadPlan* out){
    return battle_fusion_detail::stage7_reload(battle_fusion_detail::machine_dragon,linked,stages4a1,current_links_observed,out);
}
inline bool normal_machine_dragon_restore_request(const BattleFusionRestoreOwner& caller,const BattleFusionRestoreOwner& partner,
                                                   bool current_links_observed,u8 team_active10,u8 global_blocker1f,
                                                   BattleFusionRestorePlan* out){
    return battle_fusion_detail::restore_request(battle_fusion_detail::machine_dragon,caller,partner,current_links_observed,
                                               team_active10,global_blocker1f,out);
}
inline bool normal_machine_dragon_restore_cleanup(const BattleFusionRestoreOwner& owner,const BattleFusionRestoreCleanupFields& in,
                                                   BattleFusionRestoreCleanupPlan* out){
    return battle_fusion_detail::restore_cleanup(battle_fusion_detail::machine_dragon,owner,in,out);
}

inline constexpr s16 kMachineDragonFusedEntryTimeout=120;
struct MachineDragonFusedEntryStep {
    s16 timeout550=0;
    bool advance_entry=false;
};
// Lower state 59's local continuation, independent of the shared team clock.
// The primary reaches this only after its own animation request; neither
// companion nor catalogue ID can use the timeout. No motion/pose is applied.
inline bool normal_machine_dragon_fused_entry_step(u16 model,u8 variant,u8 selector,u8 phase544,s16 timeout550,
                                                    bool channel15_done,MachineDragonFusedEntryStep* out){
    if(!out||model!=battle_fusion_detail::machine_dragon.primary||variant||selector!=26||phase544!=1||
       timeout550<=0||timeout550>kMachineDragonFusedEntryTimeout)return false;
    const s16 remaining=channel15_done?timeout550:s16(timeout550-1);
    *out={remaining,channel15_done||!remaining};return true;
}
}
