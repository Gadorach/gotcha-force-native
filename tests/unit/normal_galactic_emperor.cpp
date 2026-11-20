#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/borgs/normal_beam_satellite.hpp"
#include "engine/gameplay/borgs/normal_galactic_emperor.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Galactic Emperor: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using S=NormalGalacticEmperorSource;
    using H=GalacticEmperorPinSite;
    using L=GalacticEmperorMovementLane;
    constexpr S emperor{0x0e04,0};
    constexpr NormalBeamSatelliteSource beam{0x0e01,0};
    static_assert(emperor.reviewed()&&S::movement_type==8&&S::resource_policy==1);
    static_assert(S::meter.matches_operands(10000,0,2000,{0,0,0,50}));
    GalacticEmperorPinPlan pin;
    for(auto site:{H::ClassInit,H::PreClass,H::PostClass}){
        check(normal_galactic_emperor_class_pin(emperor,site,0,0x820,&pin)&&pin.flags5e0==0x860&&pin.write_root_heading&&
            pin.root==std::array<f32,3>{0,2000,0}&&pin.heading72==8192,"ordinary class hook proposes absolute root and heading");
        for(u8 role:{1,2,128})
            check(normal_galactic_emperor_class_pin(emperor,site,role,0x80,&pin)&&pin.flags5e0==0xc0&&!pin.write_root_heading,
                "nonzero role still asserts airborne but cannot use ordinary class pin");
    }
    const auto no_pin=pin;
    check(!normal_galactic_emperor_class_pin(emperor,H(255),0,0,&pin)&&pin==no_pin,"invalid hook site cannot publish a pin");
    check(!normal_galactic_emperor_class_pin({0x0e01,0},H::ClassInit,0,0,&pin)&&
        !normal_galactic_emperor_class_pin({0x0e04,1},H::ClassInit,0,0,&pin)&&pin==no_pin,
        "Beam and alternate Emperor do not inherit the exact class pin");
    check(normal_galactic_emperor_class_pin(emperor,H::ClassInit,0,0,&pin)&&pin.write_root_heading,"stage class prefix separately");

    StageTeamXyzRows rows;
    rows.stage1c=3;rows.declared=true;
    rows.xyz[0]={{{1,90,2},{3,170,4}}};
    rows.xyz[1]={{{5,-12,6},{7,60,8}}};
    const StageTeamXyzPlacementFields fields{3,1,7,0,0,true};
    StageTeamXyzPlacementPlan plan,beam_plan;
    check(normal_galactic_emperor_table_placement(emperor,rows,fields,&plan)&&normal_beam_satellite_table_placement(beam,rows,fields,&beam_plan)&&
        plan==beam_plan&&plan.row_index==15&&plan.pre_projection==std::array<f32,3>{7,60,8}&&plan.next_cursor_fe==4&&
        plan.pre_projection[1]!=pin.root[1],"share only table arithmetic, not class height or projection");
    auto repeated=fields;repeated.placed43b=1;repeated.side43c=0;
    check(normal_galactic_emperor_table_placement(emperor,rows,repeated,&plan)&&
        plan.pre_projection==std::array<f32,3>{5,-12,6}&&!plan.write_team_cursor&&!plan.write_placement_fields&&
        plan.next_cursor_fe==fields.cursor_fe,"retained side does not consume another cursor");
    const auto saved=plan;
    repeated.stage1c=4;
    check(!normal_galactic_emperor_table_placement(emperor,rows,repeated,&plan)&&plan==saved,"stale stage is not a spawn override");
    check(!normal_beam_satellite_table_placement({0x0e04,0},rows,fields,&plan)&&plan==saved&&
        !normal_galactic_emperor_table_placement({0x0e01,0},rows,fields,&plan)&&plan==saved,
        "wrapper authorities stay separate despite shared arithmetic");

    f32 height=27.f;
    check(!normal_galactic_emperor_floor_fallback_y(emperor,false,120.f,&height)&&height==27.f,
        "table path cannot borrow the zero-authored-Y fallback");
    check(normal_galactic_emperor_floor_fallback_y(emperor,true,120.f,&height)&&height==1120.f&&height!=pin.root[1],
        "declared fallback adds type-8 offset to queried floor only");
    check(!normal_galactic_emperor_floor_fallback_y(emperor,true,std::numeric_limits<f32>::infinity(),&height)&&height==1120.f,
        "invalid query cannot become a height");

    for(auto lane:{L::Upper,L::Lower})for(u8 sel=0;sel<32;++sel){
        u8 state=255;
        const bool ok=!sel||(sel>=9&&sel<=11);
        const bool next=normal_galactic_emperor_selector_state(emperor,lane,sel,&state);
        check(next==ok&&state==(!ok?255:!sel?52:lane==L::Upper?0:sel+1),"independent exact lane table, including refusals");
    }
    u8 state=255;
    check(!normal_galactic_emperor_selector_state(emperor,L(255),0,&state)&&state==255,"malformed lane does not fall through");
    CharacterAirResourceState r{2000,1,3};
    check(normal_galactic_emperor_refill(emperor,r,1,&r)&&r.value736==2000&&r.delay73a==1&&r.air_actions747==3,
        "normal airborne stance does not generate recovery");
    check(normal_galactic_emperor_refill(emperor,r,0,&r)&&r.value736==2000&&r.delay73a==0&&r.air_actions747==0,
        "separately supplied grounded cache ticks delay and clears count");
    check(normal_galactic_emperor_refill(emperor,r,0,&r)&&r.value736==4000&&r.delay73a==0,
        "shared logical-ground refill adds fuel only on the next pass");
    std::printf("normal Galactic Emperor class pin, pre-projection placement and lane coverage: %u checks PASS\n",checks);
}
