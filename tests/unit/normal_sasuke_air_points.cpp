#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_sasuke_air_points.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Sasuke air points: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    constexpr auto rules=NormalSasukeAirPoints::rules;
    static_assert(NormalSasukeAirPoints::model==0x000a&&NormalSasukeAirPoints::source_variant==0&&rules.limit==3);
    CharacterMovementInputState input;
    NormalSasukeRelaunchFields fields;
    fields.prior_handlers_declined=true;
    for(u8 selector:{6,16,17})for(u8 used=0;used<=3;++used)
        for(u8 latch:{0,1,128})for(bool lock:{false,true})for(bool pulse:{false,true}){
            fields.selector=selector;fields.air_ready588=latch;fields.flags5e0=0x40u|(lock?2u:0u);
            input.commands5d4=pulse?2u:0u;
            input.previous_held=0x100u; // held without the pulse is never sufficient
            u8 next=255;
            check(normal_sasuke_air_relaunch_request(input,fields,used,&next)&&
               next==(!lock&&pulse&&used<3&&(selector==16||latch)?8:selector),"count relaunch leaf and latch asymmetry");
        }
    u8 value=99;
    fields.prior_handlers_declined=false;
    check(!normal_sasuke_air_relaunch_request(input,fields,1,&value)&&value==99,"prior handlers cannot be bypassed");
    fields.prior_handlers_declined=true;fields.selector=5;
    check(!normal_sasuke_air_relaunch_request(input,fields,1,&value)&&value==99,"unreviewed selector refuses");
    fields.selector=6;
    check(!normal_sasuke_air_relaunch_request(input,fields,4,&value)&&value==99,"out-of-scope used count refuses");
    input.commands5d4=0x200;
    check(!normal_sasuke_air_relaunch_request(input,fields,1,&value)&&value==99,"unsupported command cannot be passed as movement-only");
    input.commands5d4=2;fields.flags5e0=0x40;fields.air_ready588=1;
    const u8 before_entry=1;
    check(normal_sasuke_air_relaunch_request(input,fields,before_entry,&value)&&value==8&&before_entry==1,
        "request leaves the counter unspent");
    check(character_air_point_entry(rules,before_entry,AirPointEntry::Relaunch,&value)&&value==2,"entry computes the next used count");
    for(const auto entry:{AirPointEntry::Ascent,AirPointEntry::Relaunch,AirPointEntry::DashWindup}){
        for(u8 used=0;used<=3;++used){
            value=99;
            const bool dash=entry==AirPointEntry::DashWindup;
            const bool ok=character_air_point_entry(rules,used,entry,&value);
            check(ok==(dash||used<3)&&value==(dash?(used?used:1):used<3?used+1:99),
                "only committed launch entries add a point; dash seeds zero only");
        }
    }
    value=99;
    check(!character_air_point_entry(rules,3,AirPointEntry(255),&value)&&value==99&&
        !character_air_point_entry({128,true},1,AirPointEntry::Ascent,&value)&&value==99,"invalid entry or signed count domain");
    check(character_air_point_entry({3,false},0,AirPointEntry::DashWindup,&value)&&value==0,
        "zero seed is explicit policy, not a universal dash rule");

    AirPointGaugeReadout gauge;
    for(u8 count=0;count<=3;++count)for(bool hidden:{false,true}){
        const bool ready=count&&!hidden;
        check(character_air_point_gauge(rules,count,hidden,&gauge)&&gauge.draw_ready==ready&&
            gauge.remaining==(ready?3-count:0),"only nonzero unsuppressed counts yield a readout, including zero remaining");
    }
    gauge={true,2};
    check(!character_air_point_gauge(rules,4,false,&gauge)&&gauge==AirPointGaugeReadout{true,2},"bad count cannot publish a gauge");
    // Synthetic ordinary landing order. Snap/landing entry alone do not call
    // the reset store; shared resource recovery samples its accepted cache.
    CharacterAirResourceState s{350,2,2},next;
    check(character_air_resource_recover(s,AirMeterRecoveryGate::PreviousGroundedCache,0,1,10000,0,&next)&&
        next.air_actions747==2&&next.delay73a==2,"new flags do not override the earlier airborne cache");
    check(character_air_resource_recover(next,AirMeterRecoveryGate::PreviousGroundedCache,0,0,10000,0,&next)&&
        next.air_actions747==0&&next.delay73a==1,"grounded resource update resets the count even while meter delay ticks");
    check(character_air_point_gauge(rules,next.air_actions747,false,&gauge)&&!gauge.draw_ready,"no full-gauge draw after reset");
    std::printf("normal Sasuke air points: %u checks PASS\n",checks);
}
