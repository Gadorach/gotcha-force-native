#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borg_mobility_hud.hpp"

namespace {
using namespace gf;

void check(bool result,const char* message){
    if(!result){std::fprintf(stderr,"mobility HUD: %s\n",message);std::exit(1);}
}

std::shared_ptr<const BorgGameplayProfile> profile(std::string_view body){
    BorgGameplayProfile parsed;
    std::string error;
    const auto valid=borg_read_gameplay_profile("GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=200\n"
        "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"+std::string(body),&parsed,&error);
    check(valid,error.c_str());
    return std::make_shared<const BorgGameplayProfile>(std::move(parsed));
}
}

int main(){
    BorgGameplayProfile p;
    BattleMvpActor actor;
    check(!borg_mobility_hud(p,actor),"ordinary movement does not invent a resource");
    p.flight_ticks=32;actor.package_flight_used=6;actor.package_air_points_used=2;
    const auto lift=borg_mobility_hud(p,actor);
    check(lift&&*lift==BorgMobilityHud{BorgMobilityKind::Lift,26,32}&&borg_mobility_text(*lift)=="LIFT 26/32",
          "powered lift uses its own spent-tick counter");
    p.flight_ticks=0;p.air_jump_points=3;
    actor.air_dashes_used=1;
    const auto jumps=borg_mobility_hud(p,actor);
    check(jumps&&*jumps==BorgMobilityHud{BorgMobilityKind::Jumps,1,3},
          "jump points do not subtract an independent dash");
    p.air_jump_points=0;p.air_action_budget=4;
    const auto shared=borg_mobility_hud(p,actor);
    check(shared&&*shared==BorgMobilityHud{BorgMobilityKind::AirActions,2,4}&&borg_mobility_text(*shared)=="AIR ACTIONS 2/4",
          "shared actions have their own label and are not counted as jumps");
    p.air_action_budget=0;p.cruise=BorgCruiseBoost{};p.cruise->capacity=240;
    actor.package_boost_meter.value736=98;
    const auto boost=borg_mobility_hud(p,actor);
    check(boost&&*boost==BorgMobilityHud{BorgMobilityKind::Boost,98,240},"cruise uses the live boost meter");
    actor.package_boost_meter.value736=-1;
    check(!borg_mobility_hud(p,actor),"invalid boost state is not clamped into a counter");
    actor.package_boost_meter.value736=240;actor.source_ground_motion=true;
    check(!borg_mobility_hud(p,actor),"borrowed motion owns a separate display");
    actor.source_ground_motion=false;actor.health=0;
    check(!borg_mobility_hud(p,actor),"KO hides the readout");

    BattleMvpConfig config;
    config.packages={profile("movement=grounded-v1|8|13|14\nwalk=0:1\nair=0:2\ndash=0:3\nair-action-budget=3\n"
                             "projectile=shot|bullet|1|55|0|0|0|8|20|1|ignore-floor\n"
                             "move=fire|b|10|any|6|0:4\nrelease=fire|2|none|0|shot\n"),
                     profile("movement=stationary-v1\n")};
    config.spawn={BattlePoint{-300,0},BattlePoint{300,0}};
    NativeBattleMvp world;
    const auto current=[&](){return borg_mobility_hud(*config.packages[0],world.actors()[0]);};
    const auto count=[&](u16 remaining){
        return current()==BorgMobilityHud{BorgMobilityKind::AirActions,remaining,3};
    };
    check(world.start(config)&&count(3),"spawn has full shared budget");
    std::array<GamePadState,2> pads{};pads[0].b=true;
    check(world.step(pads,false)&&!world.actors()[0].grounded&&count(2),"takeoff consumes one shared point");
    pads={};pads[0].l=true;
    check(world.step(pads,false)&&world.actors()[0].dash_airborne&&count(1),"air dash consumes the same displayed pool");
    for(unsigned n=0;n<180&&!world.actors()[0].grounded;++n)check(world.step({},false),"advance to landing");
    check(world.actors()[0].grounded&&count(3),"real landing restores the shared display");
}
