#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_roster_series.hpp"

namespace {
using namespace gf;

void check(bool result,const char* message){
    if(!result){std::fprintf(stderr,"roster series: %s\n",message);std::exit(1);}
}

std::shared_ptr<const BorgGameplayProfile> fighter(std::string text){
    BorgGameplayProfile next;
    std::string error;
    const bool ok=borg_read_gameplay_profile("GFBORG-GAMEPLAY 1\nvariant=normal\n"
        "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"+text,&next,&error);
    check(ok,error.c_str());
    return std::make_shared<const BorgGameplayProfile>(std::move(next));
}

std::shared_ptr<const BorgGameplayProfile> shooter(unsigned health,unsigned count){
    return fighter("movement=grounded-v1|6|11|14\nwalk=0:1\nair=0:2\ndash=0:3\nhealth="+std::to_string(health)+
        "\nprojectile=bolt|bullet|"+std::to_string(count)+"|60|0|0|0|8|20|19|ignore-floor\n"
        "move=fire|b|10|any|6|0:4\nrelease=fire|2|none|0|bolt\n");
}

BattleMvpConfig roster(){
    BattleMvpConfig config;
    config.declared_actors=3;config.teams_declared=true;config.local_aim_cycle=true;
    return config;
}

void play(NativeBattleMvp& world,bool both=false){
    for(unsigned tick=0;tick<20&&world.result()==BattleMvpResult::Fighting;++tick){
        std::array<BorgBattleInputs,3> inputs{};
        if(!tick){
            inputs[0].pressed=inputs[0].held=BorgBattleInputs::b;
            if(both)inputs[1]=inputs[0];
        }
        check(world.step_roster(std::array<GamePadState,3>{},inputs,false),"advance ordinary battle tick");
    }
}
}

int main(){
    auto config=roster();
    const auto idle=fighter("movement=stationary-v1\nhealth=1\n");
    config.teams={0,0,1};config.packages={shooter(200,1),idle,idle};
    config.spawn={BattlePoint{0,-200},BattlePoint{-300,0},BattlePoint{0,200}};
    NativeBattleMvp world,foreign;
    BattleRosterSeries series(2);
    check(world.start(config)&&series.open_round(world)&&!series.open_round(world)&&!series.observe(world),
          "accept a fresh roster once; do not score an unfinished round");
    check(foreign.start(config)&&!series.open_round(foreign),"another world cannot replace the accepted round");
    play(foreign);
    check(foreign.result()==BattleMvpResult::GRedWins&&!series.observe(foreign),"a real but foreign win cannot score");
    play(world);
    check(world.result()==BattleMvpResult::GRedWins&&series.observe(world)&&!series.observe(world)&&
          series.wins()[0]==1&&series.winner()==255,"record one actual team win, once");
    const u32 ko_tick=world.tick();
    check(world.start(config)&&!series.owns_round(world)&&!series.observe(world)&&series.open_round(world)&&series.wins()[0]==1,
          "next round keeps the score and cannot reuse the old scope");
    check(world.start(config)&&series.open_round(world)&&series.wins()[0]==1&&!series.observe(world),
          "restarting a live round awards nothing and keeps the earlier win");
    play(world);
    check(series.observe(world)&&series.winner()==0&&series.wins()[0]==2&&!series.observe(world),
          "second distinct round clinches the series");
    check(world.start(config)&&series.open_round(world)&&series.winner()==255&&series.wins()[0]==0&&series.wins()[1]==0,
          "rematch resets both teams' score");

    auto draw=roster();
    draw.teams={0,1,0};draw.packages={shooter(19,1),shooter(19,2),idle};
    draw.spawn={BattlePoint{0,-160},BattlePoint{0,130},BattlePoint{0,-64}};
    check(world.start(draw),"start simultaneous KO fixture");
    check(series.open_round(world),"bind its new round");
    play(world,true);
    check(world.result()==BattleMvpResult::Draw&&series.observe(world)&&!series.observe(world)&&
          series.wins()[0]==0&&series.wins()[1]==0&&series.winner()==255,"draw completes the round without a point");

    const auto step=[&](bool fire=false){
        std::array<BorgBattleInputs,3> inputs{};
        if(fire)inputs[0].pressed=inputs[0].held=BorgBattleInputs::b;
        check(world.step_roster(std::array<GamePadState,3>{},inputs,false),"advance timed fixture");
    };
    auto vitality=config;
    vitality.packages[2]=fighter("movement=stationary-v1\nhealth=500\n");
    BattleRosterSeries tied(0,1);
    check(world.start(vitality)&&tied.open_round(world)&&tied.remaining_ticks(world)==60,"time-only round starts at a second");
    for(unsigned n=0;n<60;++n)step();
    check(tied.observe(world)&&tied.timed_out()&&tied.outcome()==BattleMvpResult::Draw&&
          tied.winner()==255&&tied.wins()[0]==0&&tied.wins()[1]==0,
          "equal fractions tie even with unequal roster sizes and raw HP");
    check(world.start(vitality)&&tied.open_round(world)&&tied.remaining_ticks(world)==60&&!tied.ended(),
          "a new round clears the old timeout and restores its clock");

    BattleRosterSeries scored(2,1);
    check(world.start(vitality)&&scored.open_round(world),"combine clock with series");
    step(true);
    while(world.tick()<30)step();
    check(world.set_paused(true),"pause the timed round");
    for(unsigned n=0;n<20;++n)step();
    check(world.tick()==30&&scored.remaining_ticks(world)==30&&!scored.observe(world)&&world.set_paused(false),
          "paused steps do not consume simulation time");
    while(world.tick()<59)step();
    check(!scored.observe(world)&&scored.remaining_ticks(world)==1,"no time decision before the last tick");
    step();
    check(world.result()==BattleMvpResult::Fighting&&world.actors()[2].health==481&&
          scored.observe(world)&&scored.timed_out()&&scored.outcome()==BattleMvpResult::GRedWins&&
          scored.wins()[0]==1&&scored.remaining_ticks(world)==0&&!scored.observe(world),
          "higher remaining fraction wins exactly once, without altering combat HP or result");

    BattleRosterSeries buzzer(2,1);
    check(world.start(config)&&buzzer.open_round(world)&&ko_tick<60,"schedule a real KO at the time limit");
    while(world.tick()<60)step(world.tick()==60-ko_tick);
    check(world.result()==BattleMvpResult::GRedWins&&buzzer.observe(world)&&!buzzer.timed_out()&&buzzer.wins()[0]==1,
          "a KO on the last tick wins before timeout adjudication");
}
