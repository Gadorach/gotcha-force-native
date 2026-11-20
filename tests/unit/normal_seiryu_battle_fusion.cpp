#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "engine/gameplay/borgs/normal_seiryu_battle_fusion.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Seiryu fusion: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using Role=SeiryuBattleRole;
    u8 latch=99;
    check(seiryu_request_latch(5,1,0,&latch)&&latch==5,"no pulse does not age a prior latch");
    check(seiryu_request_latch(5,0,0x200,&latch)&&latch==5,"unready team cannot seed");
    check(seiryu_request_latch(0,1,0x200,&latch)&&latch==6,"conditioned pulse seeds six");
    for(bool reversed:{false,true})for(bool red_ai:{false,true})for(bool blue_ai:{false,true}){
        const SeiryuMatchCandidate red{2,1,0x600,0,1,red_ai},blue{4,1,0x601,0,2,blue_ai};
        const std::array inputs{reversed?blue:red,reversed?red:blue};
        SeiryuMatchPlan plan;
        check(normal_seiryu_match(inputs,true,&plan),"exact two-entry team list can stage assignments");
        const u8 primary=red_ai&&!blue_ai?4:2;
        const u8 anchor=inputs[0].ai&&!inputs[1].ai?inputs[1].slot:inputs[0].slot;
        check(plan.returned_anchor_slot==anchor,"orchestration anchor follows list/human rule, not primary role");
        for(unsigned i=0;i<2;++i){
            const bool owns_primary=inputs[i].slot==primary;
            check(plan.slots[i]==SeiryuStagedSlot{inputs[i].slot,inputs[1-i].slot,u16(owns_primary?0x602:0x603),0,
                owns_primary?Role::Primary:Role::Companion},"reciprocal links and exact pending identities");
        }
        const auto retained=plan;
        auto bad=inputs;bad[1].selected_variant=1;
        check(!normal_seiryu_match(bad,true,&plan)&&plan==retained,"variant not recovered");
        bad=inputs;bad[1].slot=bad[0].slot;
        check(!normal_seiryu_match(bad,true,&plan)&&plan==retained,"one slot cannot stand in for two");
        bad=inputs;bad[1].team=2;
        check(!normal_seiryu_match(bad,true,&plan)&&!normal_seiryu_match(inputs,false,&plan)&&plan==retained,
            "requires owner team and prior scan");
        SeiryuReloadPlan reload;
        check(normal_seiryu_stage7_reload(plan,{7,6},true,&reload)&&!reload.stage_pair_reload,"a single ready slot cannot commit pair");
        check(normal_seiryu_stage7_reload(plan,{7,7},true,&reload)&&reload.stage_pair_reload&&reload.slots==retained.slots,
            "ready pair stages lifecycle work, not model writes");
        auto stale=retained;stale.slots[0].pending_model48c=0x600;
        check(!normal_seiryu_stage7_reload(stale,{7,7},true,&reload)&&
            !normal_seiryu_stage7_reload(plan,{7,7},false,&reload)&&reload.slots==retained.slots,
            "stale bindings and unresolved live links cannot publish a reload plan");
    }

    SeiryuTeamClock clock;
    check(seiryu_fused_init_clock(Role::Companion,&clock)&&clock==SeiryuTeamClock{0,2,1800,1800},
        "either applicable slot can seed the same team clock");
    check(!seiryu_fused_init_clock(Role::None,&clock)&&clock.remaining18==1800,"ordinary role doesn't seed fusion");
    SeiryuTeamClockStep step;
    check(seiryu_team_clock_step(clock,false,&step)&&step.clock==clock&&!step.clear_meter32,"paused/ineligible update is unchanged");
    check(seiryu_team_clock_step(clock,true,&step)&&step.clock.remaining18==1799&&!step.clear_meter32,"one eligible pass");
    clock.remaining18=1;
    check(seiryu_team_clock_step(clock,true,&step)&&step.clock==SeiryuTeamClock{0,0,0,1800}&&step.clear_meter32,
        "expiry clears only team activity and requests zero team meter");
    check(!seiryu_team_clock_step(step.clock,true,&step)&&step.clear_meter32,"inactive clock cannot double-tick");

    SeiryuRestoreOwner red{2,4,1,0,0,Role::Companion,0x603,0x600,0,0,7};
    SeiryuRestoreOwner blue{4,2,1,0,0,Role::Primary,0x602,0x601,0,0,19};
    SeiryuRestorePlan restore;
    check(normal_seiryu_restore_request(blue,red,true,2,0,&restore)&&!restore.requested,"active timer blocks restoration");
    check(normal_seiryu_restore_request(blue,red,true,0,1,&restore)&&!restore.requested,"global blocker after expiry");
    red.flags5e0=0x1000;
    check(normal_seiryu_restore_request(blue,red,true,0,0,&restore)&&!restore.requested,"partner guard can block caller");
    red.flags5e0=0;blue.action_state584=5;blue.flags5e0=1;
    check(normal_seiryu_restore_request(blue,red,true,0,0,&restore)&&!restore.requested,"caller's low-bit/state-five guard");
    blue.flags5e0=0;
    check(normal_seiryu_restore_request(blue,red,true,0,0,&restore)&&restore==SeiryuRestorePlan{true,4,4,0x601,0,19},
        "primary queues saved Blue, not the role's default Red");
    blue.battle_state18=2;
    check(normal_seiryu_restore_request(red,blue,true,0,0,&restore)&&restore==SeiryuRestorePlan{true,2,4,0x600,0,7},
        "partner's pending lifecycle doesn't turn caller's own restore into atomic split");
    const auto retained=restore;
    blue.partner_slot=3;
    check(!normal_seiryu_restore_request(red,blue,true,0,0,&restore)&&restore==retained,"reciprocal current links mandatory");
    std::printf("normal Seiryu two-slot fusion and per-owner restore: %u checks PASS\n",checks);
}
