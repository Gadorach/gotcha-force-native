#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/borgs/normal_blue_striker_boost.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Blue Striker boost: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using R=NormalBlueStrikerBoost;
    static_assert(R::model==0x0d00&&R::source_variant==0&&R::movement_type==3&&R::resource_policy==1);
    static_assert(R::meter.matches_operands(10000,16,50,{0,0,0,50})&&R::exit_clock==30.f);
    CharacterMovementInputState input;
    BlueStrikerBoostRequestFields fields{R::cruise,0,0x40,true};
    CharacterAirResourceState resource{25,0,4};
    auto request=[&](u8 expected){
        u8 next=0;
        check(normal_blue_striker_boost_request(input,fields,resource,&next)&&next==expected,"selector request");
    };
    input.previous_held=0x100;
    request(R::cruise); // held alone is insufficient
    input.commands5d4=2;
    fields.flags5e0|=2;
    request(R::cruise); // locked
    fields.flags5e0&=~2u;
    resource.value736=0;
    request(R::cruise);
    resource.value736=25;
    input.previous_held=0;
    request(R::boost); // conditioned pulse, no held recheck

    constexpr f32 nan=std::numeric_limits<f32>::quiet_NaN();
    BlueStrikerBoostDebit debit{{999,7,2},0x234};
    check(!normal_blue_striker_boost_debit(resource,0x40,nan,&debit)&&debit.resource.value736==999&&debit.flags5e0==0x234,
        "bad debit cannot publish meter or flags");
    for(u32 flags:{0u,0x40u,0x80u,0xc0u}){
        CharacterAirResourceState recovered;
        check(normal_blue_striker_preselect_refill({9980,0,4},flags,&recovered)&&
            recovered.value736==(flags&0x80u?9980:10000)&&recovered.air_actions747==4,
            "refill is clamped, ignores airborne bit and preserves air-point field");
    }
    check(normal_blue_striker_boost_debit({51,2,4},0x40,0.5f,&debit)&&
        debit.resource.value736==26&&debit.resource.delay73a==16&&debit.resource.air_actions747==4&&debit.flags5e0==0xc0,
        "fractional-rate debit uses the shared resource helper");
    check(normal_blue_striker_boost_debit({0,3,4},0x40,1.f,&debit)&&
        debit.resource.value736==0&&debit.resource.delay73a==3&&debit.flags5e0==0xc0,
        "already empty handler still sets boost but does not reset the delay");

    // Staged scalar sequence. The exact actor, input clock and selector
    // acceptance belong to a future owner; the test supplies them explicitly.
    input.commands5d4=2;
    check(normal_blue_striker_preselect_refill(resource,fields.flags5e0,&resource)&&resource.value736==75,"refill before request");
    request(R::boost);
    check(normal_blue_striker_boost_debit(resource,fields.flags5e0,1.f,&debit)&&debit.resource.value736==25&&
        debit.resource.delay73a==16,"accepted boost handler debits after selection");
    resource=debit.resource;fields.selector=R::boost;fields.flags5e0=debit.flags5e0;
    input.commands5d4=0;input.previous_held=0;
    check(normal_blue_striker_preselect_refill(resource,fields.flags5e0,&resource)&&resource.delay73a==16,"active boost pauses delay");
    request(R::boost); // released with latch closed
    check(normal_blue_striker_boost_debit(resource,fields.flags5e0,1.f,&debit)&&debit.resource.value736==0&&
        debit.resource.delay73a==16,"continuing active call debits again and clamps");
    resource=debit.resource;input.previous_held=0x100;
    request(R::boost); // empty with latch closed
    BlueStrikerExitClockStep clock;
    check(normal_blue_striker_exit_clock(2.f,1.25f,&clock)&&clock.remaining568==0.75f&&!clock.opens_exit_latch,
        "positive clock retains its phase");
    check(normal_blue_striker_exit_clock(0.75f,1.f,&clock)&&clock.remaining568==-0.25f&&clock.opens_exit_latch,
        "crossing zero opens the exit latch without clamping the clock");
    check(!normal_blue_striker_exit_clock(0.f,1.f,&clock)&&clock.remaining568==-0.25f,
        "exhausted clock is outside the supplied phase-one domain");
    fields.exit_latch588=1;
    resource.value736=25;
    request(R::boost); // clock opening alone is not an exit
    input.previous_held=0;
    request(R::cruise); // released and still positive
    resource.value736=0;input.previous_held=0x100;
    check(normal_blue_striker_preselect_refill(resource,fields.flags5e0,&resource)&&resource.delay73a==16,
        "current exit update still observes old boost flag in the earlier pass");
    request(R::cruise); // held but empty
    const auto cruise=normal_blue_striker_cruise_entry(fields.flags5e0|0x2200u);
    check(cruise.flags5e0==0x240&&cruise.cached87==1,"ordinary cruise entry clears boost; it does not become grounded");
    fields.selector=R::cruise;fields.flags5e0=cruise.flags5e0;fields.exit_latch588=0;
    for(int delay=15;delay>=0;--delay){
        check(normal_blue_striker_preselect_refill(resource,fields.flags5e0,&resource)&&
            resource.delay73a==delay&&resource.value736==0,"no fuel on any delay-only pass, even 1 to 0");
    }
    check(normal_blue_striker_preselect_refill(resource,fields.flags5e0,&resource)&&resource.value736==50&&
        resource.delay73a==0,"following inactive pass increments meter");
    request(R::cruise); // returned fuel did not generate a new pulse

    AirMeterGaugeReadout gauge;
    check(character_air_meter_gauge(10000,R::meter.limit,false,&gauge)&&gauge==AirMeterGaugeReadout{false,0},"full hides");
    check(character_air_meter_gauge(0,R::meter.limit,false,&gauge)&&gauge==AirMeterGaugeReadout{true,1},"empty draws");
    check(character_air_meter_gauge(5000,R::meter.limit,false,&gauge)&&gauge==AirMeterGaugeReadout{true,0.5f},"half-meter sample");
    check(character_air_meter_gauge(0,R::meter.limit,true,&gauge)&&gauge==AirMeterGaugeReadout{false,0},"suppressed empty is not drawn");
    gauge={true,0.5f};
    check(!character_air_meter_gauge(10001,R::meter.limit,false,&gauge)&&gauge==AirMeterGaugeReadout{true,0.5f},
        "invalid meter refuses without replacing readout");

    u8 next=255;fields.prior_handlers_declined=false;
    check(!normal_blue_striker_boost_request(input,fields,resource,&next)&&next==255,"earlier owners cannot be bypassed");
    fields.prior_handlers_declined=true;fields.selector=0;
    check(!normal_blue_striker_boost_request(input,fields,resource,&next)&&next==255,"initial selection is outside this component");
    check(!normal_blue_striker_preselect_refill({0,17,4},0,&resource)&&resource.value736==50,"wrong delay domain cannot publish");
    std::printf("normal Blue Striker boost and meter gauge: %u checks PASS\n",checks);
}
