#include <cstdio>
#include <cstdlib>
#include "engine/assets/borg_spawn_profile.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg spawn profile: %s\n",why);std::abort();}
}
std::string replace(std::string text,std::string_view old,std::string_view next){
    const auto at=text.find(old);check(at!=std::string::npos,"replace test field");
    text.replace(at,old.size(),next);
    return text;
}
}
int main(){
    using namespace gf;
    const std::string profile=R"(GFBORG-SPAWN 1
spawn=top-entry|base|challenge|normal
anchor=arena-top-clearance
height=unresolved
horizontal-anchor=mode-selected
horizontal=none
initial-state=falling
initial-vy=unresolved
requirement=required
on-obstruction=refuse
spawn=static-entry|tower|*|*
anchor=absolute-y
height=-15.5
horizontal-anchor=stage-center
horizontal=none
initial-state=fixed-airborne
initial-vy=0
requirement=required
on-obstruction=refuse
spawn=flight-entry|wings|story|normal
anchor=floor-offset
height=120
horizontal-anchor=absolute-xz
horizontal=-240|320.5
initial-state=flying
initial-vy=5
requirement=preferred
on-obstruction=mode-selected
spawn=ordinary-entry|base|duel|*
anchor=mode-selected
height=none
horizontal-anchor=unresolved
horizontal=unresolved
initial-state=grounded
initial-vy=0
requirement=preferred
on-obstruction=mode-selected
)";
    BorgSpawnProfile parsed;
    check(borg_read_spawn_profile(profile,&parsed)&&parsed.rules.size()==4,"distinct scopes can record both airborne and grounded roles");
    const auto& top=parsed.rules[0];
    check(top.anchor==BorgSpawnAnchor::ArenaTopClearance&&top.height_kind==BorgSpawnValue::Unresolved&&
        top.initial_state==BorgSpawnState::Falling&&!top.initial_vy&&top.on_obstruction==BorgSpawnObstruction::Refuse,
        "near-top anchor does not invent a clearance or vertical speed");
    check(parsed.rules[1].height==-15.5f&&parsed.rules[1].initial_state==BorgSpawnState::FixedAirborne&&
        parsed.rules[1].initial_vy==0&&parsed.rules[2].height==120.f&&parsed.rules[2].initial_state==BorgSpawnState::Flying&&
        parsed.rules[2].on_obstruction==BorgSpawnObstruction::ModeSelected,
        "absolute height, floor offset, static airborne and flying are separate data");
    check(parsed.rules[1].horizontal_anchor==BorgSpawnHorizontalAnchor::StageCenter&&
        parsed.rules[1].horizontal_kind==BorgSpawnValue::None&&
        parsed.rules[2].horizontal_anchor==BorgSpawnHorizontalAnchor::AbsoluteXZ&&
        parsed.rules[2].xz==std::array<f32,2>{-240,320.5f},"stage center and world coordinates are distinct horizontal anchors");
    check(parsed.rules[3].height_kind==BorgSpawnValue::None&&parsed.rules[3].anchor==BorgSpawnAnchor::ModeSelected,
        "deliberate absence of an altitude is not unresolved");
    for(const auto& invalid:{
        replace(profile,"top-entry|base|challenge|normal","top-entry|base|*|normal"),
        replace(profile,"spawn=flight-entry","spawn=top-entry"),
        replace(profile,"spawn=flight-entry|wings","spawn=flight-entry|../wings"),
        replace(profile,"height=120","height=-1"),
        replace(profile,"height=-15.5","height=100001"),
        replace(profile,"height=unresolved","height=NaN"),
        replace(profile,"height=none","height=30"),
        replace(profile,"horizontal-anchor=stage-center\nhorizontal=none","horizontal-anchor=stage-center\nhorizontal=0|0"),
        replace(profile,"horizontal=-240|320.5","horizontal=-240"),
        replace(profile,"horizontal=-240|320.5","horizontal=NaN|4"),
        replace(profile,"horizontal=-240|320.5","horizontal=none"),
        replace(profile,"anchor=floor-offset","anchor=unresolved"),
        replace(profile,"initial-vy=unresolved","initial-vy=inf"),
        replace(profile,"requirement=required\non-obstruction=refuse","requirement=required\non-obstruction=mode-selected"),
        replace(profile,"initial-state=falling\n",""),
        profile+"z=120\n"
    }){
        check(!borg_read_spawn_profile(invalid,&parsed)&&parsed.rules.size()==4&&parsed.rules[0].id=="top-entry",
            "overlapping scopes, invalid values and contradictory placement refuse atomically");
    }
    std::puts("Borg spawn profile: scoped anchors, unresolved data, initial state and refused overlaps PASS");
}
