#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "engine/gameplay/borgs/normal_machine_dragon_fusion.hpp"
#include "engine/gameplay/borgs/normal_seiryu_battle_fusion.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Machine Dragon fusion: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using Role=BattleFusionRole;
    const BattleFusionMatchCandidate head{2,1,0x0511,0,1,true},body{4,1,0x0512,0,2,false};
    BattleFusionMatchPlan match;
    check(normal_machine_dragon_match({body,head},true,&match)&&match.returned_anchor_slot==4&&
        match.slots[0]==BattleFusionStagedSlot{4,2,0x0513,0,Role::Primary}&&
        match.slots[1]==BattleFusionStagedSlot{2,4,0x0514,0,Role::Companion},
        "human Mechanical pilots the primary; result destinations are exact");
    const auto retained=match;
    auto bad=body;bad.current_model=0x050f;
    check(!normal_machine_dragon_match({bad,head},true,&match)&&
        !normal_seiryu_match({body,head},true,&match)&&retained==match,"catalogue and other fusion identities refuse");
    BattleFusionReloadPlan reload;
    check(normal_machine_dragon_stage7_reload(match,{7,6},true,&reload)&&!reload.stage_pair_reload,"both slots must be ready");
    check(normal_machine_dragon_stage7_reload(match,{7,7},true,&reload)&&reload.stage_pair_reload&&reload.slots==match.slots,
        "ready pair proposes Dragon reload only");
    check(!normal_seiryu_stage7_reload(match,{7,7},true,&reload)&&reload.stage_pair_reload,
        "shared stage-seven implementation still requires exact destinations");

    BattleFusionRestoreOwner h{2,4,1,0,0,Role::Companion,0x0514,0x0511,0,0,7};
    const BattleFusionRestoreOwner b{4,2,1,0,0,Role::Primary,0x0513,0x0512,0,0,19};
    BattleFusionRestorePlan pending;
    check(normal_machine_dragon_restore_request(b,h,true,0,0,&pending)&&pending==BattleFusionRestorePlan{true,4,4,0x0512,0,19},
        "primary restores saved Mechanical Dragon, not Head or catalogue result");
    check(!normal_seiryu_restore_request(b,h,true,0,0,&pending)&&pending.pending_model48c==0x0512,
        "other battle pair cannot restore this owner");
    BattleFusionRestoreCleanupFields fields{pending.pending_model48c,pending.pending_variant48e,pending.mode490,0,true};
    BattleFusionRestoreCleanupPlan cleanup;
    check(normal_machine_dragon_restore_cleanup(b,fields,&cleanup)&&cleanup==BattleFusionRestoreCleanupPlan{true,true,4,0x0512,0},
        "zero 48A reaches role/link clear and single-slot resource request");
    for(u8 skip:{1,128,255}){
        fields.skip48a=skip;
        check(normal_machine_dragon_restore_cleanup(b,fields,&cleanup)&&cleanup==BattleFusionRestoreCleanupPlan{},
            "nonzero 48A skips both cleanup and resource request");
    }
    fields.skip48a=0;fields.current_slot_observed=false;
    check(!normal_machine_dragon_restore_cleanup(b,fields,&cleanup)&&cleanup==BattleFusionRestoreCleanupPlan{},
        "earlier restore prediction is not a current mode-four observation");
    fields.current_slot_observed=true;fields.pending_model48c=0x050f;
    check(!normal_machine_dragon_restore_cleanup(b,fields,&cleanup)&&cleanup==BattleFusionRestoreCleanupPlan{},
        "stale or catalogue tuple refuses instead of resolving a different model");
    h.partner_slot=255;
    check(normal_machine_dragon_restore_cleanup(h,{0x0511,0,4,0,true},&cleanup)&&
        cleanup==BattleFusionRestoreCleanupPlan{true,true,2,0x0511,0},"local cleanup does not require a still-linked partner");

    MachineDragonFusedEntryStep entry;
    check(normal_machine_dragon_fused_entry_step(0x0513,0,26,1,120,false,&entry)&&entry.timeout550==119&&!entry.advance_entry,
        "Dragon-local pending channel decrements the independent timeout");
    check(normal_machine_dragon_fused_entry_step(0x0513,0,26,1,1,false,&entry)&&!entry.timeout550&&entry.advance_entry,
        "expired local timeout advances the entry, not the team clock");
    check(normal_machine_dragon_fused_entry_step(0x0513,0,26,1,29,true,&entry)&&entry.timeout550==29&&entry.advance_entry,
        "channel completion short circuits the decrement");
    for(u16 model:{0x0514,0x050f,0x0602})
        check(!normal_machine_dragon_fused_entry_step(model,0,26,1,1,true,&entry)&&entry.timeout550==29,
            "local animation step only applies to exact Dragon primary");
    std::printf("normal Machine Dragon shared handshake, cleanup and local timeout: %u checks PASS\n",checks);
}
