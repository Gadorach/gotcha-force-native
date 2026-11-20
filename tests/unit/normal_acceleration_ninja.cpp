#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_acceleration_ninja.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Acceleration Ninja: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using P=AccelerationProgramPath;
    using E=AccelerationEdgeExit;
    using G=AccelerationHitGate;
    constexpr NormalAccelerationSource source{4,0,3};
    AccelerationStanceEntry entry;
    check(normal_acceleration_start(source,true,&entry)&&entry.sustained558==3600&&entry.lockout55c==20&&
        entry.stored_speed38==39&&entry.vertical_change50==-1.2f&&entry.phase540==1&&entry.presentation6ea==1&&
        entry.mask==15&&entry.group==4&&!entry.program,"same-actor startup proposes class constants and exact program");
    check(!normal_acceleration_start({5,0,3},true,&entry)&&!normal_acceleration_start({4,1,3},true,&entry)&&
        entry.sustained558==3600,"Teleport and alternate variants cannot borrow stance");
    AccelerationSlowStep slow;
    check(normal_acceleration_slow_step(source,9,1,true,&slow)&&slow.slow560==10&&!slow.request_recovery,
        "low-progress threshold is strictly greater than ten");
    check(normal_acceleration_slow_step(source,10,0.5f,false,&slow)&&slow.slow560==10&&!slow.request_recovery,
        "ordinary successful progress doesn't invent a counter reset");
    check(normal_acceleration_slow_step(source,10,0.5f,true,&slow)&&slow.request_recovery,"over-threshold slow step recovers");
    AccelerationLockoutStep lock;
    check(normal_acceleration_lockout(source,2,1,0x500,&lock)&&lock.lockout55c==1&&lock.exit==E::None,
        "positive lock prevents exit edges");
    check(normal_acceleration_lockout(source,1,1,0x500,&lock)&&!lock.lockout55c&&lock.exit==E::Recovery,
        "fresh X wins over same-update A at the opening");
    check(normal_acceleration_lockout(source,0,1,0x100,&lock)&&lock.exit==E::JumpOrFallRequest,"A requests other admission");
    check(normal_acceleration_lockout(source,0,1,0,&lock)&&lock.exit==E::None,"held/released X doesn't create a fresh request");
    AccelerationSustainStep sustained;
    check(normal_acceleration_sustain(source,2,1,3,&sustained)&&!sustained.request_recovery&&!sustained.request_trail_update,
        "presentation threshold alone has no hit");
    check(normal_acceleration_sustain(source,2,1,39,&sustained)&&sustained.request_trail_update&&!sustained.request_recovery,
        "sustained high-speed visit can request visual update");
    check(normal_acceleration_sustain(source,1,1,39,&sustained)&&sustained.request_recovery&&!sustained.request_trail_update,
        "timeout takes recovery ahead of presentation");

    AccelerationHitShapeRequest hit;
    check(normal_acceleration_hit_shape(source,P::StanceStartup,2,true,&hit)&&!hit.requested,"other lane can't register stance shapes");
    check(normal_acceleration_hit_shape(source,P::StanceStartup,15,true,&hit)&&hit.requested&&hit.actor_slot==3&&
        hit.credited_slot==3&&hit.shape_group==10&&hit.hit_row==9&&hit.start==4&&hit.end==-2,"actor is carrier and credited owner");
    AccelerationHitObservation observed{3,4,0,true,false,true,3.9f};
    AccelerationHitGate gate=G::Retire;
    check(normal_acceleration_hit_gate(source,hit,observed,&gate)&&gate==G::Waiting,"truncated pre-threshold animation waits");
    observed.lane0_position1cdc=4.0f;
    check(normal_acceleration_hit_gate(source,hit,observed,&gate)&&gate==G::EligibleToActivate,"pose threshold is enough for this branch");
    observed.program_slot=1;
    check(normal_acceleration_hit_gate(source,hit,observed,&gate)&&gate==G::Retire,"end -2 retains program-key retirement");
    check(normal_acceleration_hit_shape(source,P::StanceRecovery,15,true,&hit)&&!hit.requested,"recovery doesn't register a new stance group");
    check(normal_acceleration_hit_shape(source,P::OrdinaryAirDash,13,true,&hit)&&hit.requested&&hit.shape_group==11&&
        hit.hit_row==11&&hit.start==5,"air dash uses a separate source group and threshold");
    observed.program_group=0;observed.program_slot=11;observed.lane0_position1cdc=5;observed.retire1d8=true;
    check(normal_acceleration_hit_gate(source,hit,observed,&gate)&&gate==G::Retire,"retirement flag takes precedence over activation");
    observed.retire1d8=false;observed.actor_slot=2;
    check(!normal_acceleration_hit_gate(source,hit,observed,&gate)&&gate==G::Retire,"unrelated slot can't reuse the binding");
    std::printf("normal Acceleration Ninja stance timing and detached actor shapes: %u checks PASS\n",checks);
}
