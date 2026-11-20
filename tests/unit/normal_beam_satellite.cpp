#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/borgs/normal_beam_satellite.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Beam Satellite: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using S=NormalBeamSatelliteSource;
    constexpr S source{0x0e01,0};
    static_assert(source.reviewed()&&S::movement_type==9&&S::resource_policy==1);
    static_assert(S::meter.matches_operands(10000,0,2000,{0,0,0,50}));
    BeamSatellitePlacementRows rows;
    rows.stage1c=4;rows.declared=true;
    rows.xyz[0]={{{-1.f,80.f,100.f},{50.f,-60.f,2.f}}};
    rows.xyz[1]={{{25.f,20.f,90.f},{-40.f,10.f,3.f}}};
    BeamSatellitePlacementFields fields{4,0,0,0,255,true};
    BeamSatellitePlacementPlan plan;
    for(u8 team:{0,1})for(u8 cursor:{0,1,2,3,4,5,254,255}){
        fields.team88=team;fields.cursor_fe=cursor;
        check(normal_beam_satellite_table_placement(source,rows,fields,&plan)&&
            plan.pre_projection==rows.xyz[team][cursor&1u]&&plan.row_index==16+team*2+(cursor&1u)&&
            plan.placed43b==1&&plan.side43c==(cursor&1u)&&plan.next_cursor_fe==(cursor&3u)+1&&
            plan.write_placement_fields&&plan.write_team_cursor,"fresh row is complete XYZ and cursor advances only as a proposal");
        check(fields.placed43b==0&&fields.cursor_fe==cursor&&fields.side43c==255,"proposal cannot mutate the caller's observations");
    }
    fields.placed43b=1;fields.side43c=0;
    check(normal_beam_satellite_table_placement(source,rows,fields,&plan)&&plan.pre_projection==rows.xyz[1][0]&&
        plan.next_cursor_fe==255&&!plan.write_placement_fields&&!plan.write_team_cursor,"repeat uses retained side, not the cursor");
    rows.stage1c=2;rows.xyz[1][0][1]=123.f;
    fields.stage1c=2;
    check(normal_beam_satellite_table_placement(source,rows,fields,&plan)&&plan.row_index==10&&plan.pre_projection[1]==123.f,
        "retained side still follows the currently declared stage");
    const auto retained=plan;
    fields.stage1c=3;
    check(!normal_beam_satellite_table_placement(source,rows,fields,&plan)&&plan==retained,"stale stage row refuses");
    fields.stage1c=2;fields.side43c=2;
    check(!normal_beam_satellite_table_placement(source,rows,fields,&plan)&&plan==retained,"bad retained side refuses");
    fields.side43c=0;fields.table_path_declared=false;
    check(!normal_beam_satellite_table_placement(source,rows,fields,&plan)&&plan==retained,"cannot assume table path or substitute per-slot fallback");
    fields.table_path_declared=true;
    for(const auto other:{S{0x0e01,1},S{0x0e04,0}})
        check(!normal_beam_satellite_table_placement(other,rows,fields,&plan)&&plan==retained,"different variant or Fortress cannot inherit placement");
    rows.xyz[1][0][1]=std::numeric_limits<f32>::quiet_NaN();
    check(!normal_beam_satellite_table_placement(source,rows,fields,&plan)&&plan==retained,"nonfinite selected row leaves the previous result untouched");

    CharacterMovementInputState input;
    u8 selector=255;
    check(normal_beam_satellite_steering_request(source,S::idle,0x40,true,input,&selector)&&selector==S::idle,
        "idle without steering makes no movement request");
    input.previous_held=0x10000;
    check(normal_beam_satellite_steering_request(source,S::idle,0x42,true,input,&selector)&&selector==S::idle,"lock blocks idle turn");
    check(normal_beam_satellite_steering_request(source,S::idle,0x40,true,input,&selector)&&selector==S::turn,
        "logical air state does not block steering");
    check(normal_beam_satellite_turn_handoff(source,selector,false,&selector)&&selector==S::turn,"uncompleted turn stays in turn");
    check(normal_beam_satellite_turn_handoff(source,selector,true,&selector)&&selector==S::travel,"completed turn requests travel");
    check(normal_beam_satellite_steering_request(source,selector,0x40,true,input,&selector)&&selector==S::travel,"continued steering retains travel");
    input.previous_held=0;
    check(normal_beam_satellite_steering_request(source,selector,0x40,true,input,&selector)&&selector==S::brake,"neutral requests braking");
    BeamSatelliteBrakeHandoff handoff;
    check(normal_beam_satellite_brake_handoff(source,selector,1.f,0x50,&handoff)&&handoff.selector==S::brake&&
        !handoff.ordinary_reset_requested&&handoff.flags5e0==0x50,"equality is not below brake threshold");
    check(normal_beam_satellite_brake_handoff(source,selector,0.75f,0x12340c2,&handoff)&&handoff.selector==S::idle&&
        handoff.ordinary_reset_requested&&handoff.flags5e0==0x230042,"reset returns to idle and reasserts airborne");
    check(!normal_beam_satellite_steering_request(source,27,0x40,true,input,&selector)&&selector==S::brake,
        "representable boost selector is not an admitted ordinary route");
    check(!normal_beam_satellite_steering_request(source,S::idle,0x40,false,input,&selector)&&selector==S::brake,
        "earlier owners remain required");

    u32 flags=0;
    check(normal_beam_satellite_airborne_flags(source,0x80,&flags)&&flags==0xc0,"class flag write is unrelated to root or floor");
    CharacterAirResourceState r{3000,1,3};
    check(normal_beam_satellite_refill(source,r,1,&r)&&r.value736==3000&&r.delay73a==1&&r.air_actions747==3,
        "logical airborne state holds meter and delay");
    check(normal_beam_satellite_refill(source,r,0,&r)&&r.value736==3000&&r.delay73a==0&&r.air_actions747==0,
        "explicit grounded cache decrements delay without fuel");
    r.value736=9000;
    check(normal_beam_satellite_refill(source,r,0,&r)&&r.value736==10000,"grounded cache refills and caps on next pass");
    check(!normal_beam_satellite_refill(source,r,2,&r)&&r.value736==10000,"unknown cache cannot publish a resource update");
    std::printf("normal Beam Satellite pre-projection placement and steering: %u checks PASS\n",checks);
}
