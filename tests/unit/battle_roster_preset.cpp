#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_roster_preset.hpp"

namespace {
using namespace gf;
void check(bool valid,const char* message){
    if(!valid){std::fprintf(stderr,"roster preset: %s\n",message);std::exit(1);}
}
}

int main(){
    const std::string text="GFROSTER 1\r\n# Atelier \xc3\xa9\r\nstage=example:workshop\r\n"
        "fighter=0|example:flight-stick|normal|west\r\nfighter=1|example:jump-stick|clear|east\r\n"
        "fighter=1|example:jump-stick|normal|east-rear";
    BattleRosterPreset parsed;
    std::string error;
    check(battle_roster_read_preset(text,&parsed,&error),"valid three-fighter CRLF preset");
    check(parsed.stage=="example:workshop"&&parsed.fighters.size()==3&&
          parsed.fighters[1].variant=="clear"&&parsed.fighters[2]==BattleRosterPresetFighter{1,"example:jump-stick","normal","east-rear"},
          "ordered slots retain explicit variants and markers; admissibility is separate");
    const auto previous=parsed;
    const auto bad=[&](std::string invalid){
        check(!battle_roster_read_preset(invalid,&parsed,&error)&&parsed==previous&&!error.empty(),
              "invalid preset must not overwrite a previously validated value");
    };
    bad("GFROSTER 2\n");
    bad(text.substr(0,text.find("\r\nfighter=1|example:jump-stick|normal")));
    bad(text.substr(0,text.find("stage="))+text.substr(text.find("fighter=")));
    bad(text+"\nstage=example:other");
    bad(text+"\nfighter=0|example:flight-stick|normal|west");
    bad(text+"\nfighter=0|example:flight-stick||west-outer");
    bad(text+"\nfighter=0|../model.hsd|normal|west-outer");
    bad(text+"\nfighter=2|example:flight-stick|normal|west-outer");
    bad(text+"\nfighter=0|example:flight-stick|normal|west-outer|extra");
    bad(text+"\nsave=/tmp/save");
    for(const auto* value:{"0","6","02","two",""})bad(text+"\nrounds-to-win="+value);
    bad(text+"\nrounds-to-win=2\nrounds-to-win=3");
    for(const auto* value:{"0","601","01","1.5","+1","-1"," 1",""})bad(text+"\nround-seconds="+value);
    bad(text+"\nround-seconds=1\nround-seconds=2");
    for(const auto* value:{"0","3","01","one",""})bad(text+"\nlocals="+value);
    bad(text+"\nlocals=1\nlocals=2");
    bad(text+"\n# invalid \xc0\xaf");
    bad(text+std::string(kBattleRosterPresetBytes,'\n'));
    auto swapped=text;
    swapped[swapped.find("fighter=")+8]='1';
    bad(swapped);
    auto coop=text;
    coop[coop.find("fighter=1")+8]='0';
    auto no_opponents=coop;
    no_opponents[no_opponents.find("fighter=1")+8]='0';
    bad(no_opponents);
    bad(coop+"\nfighter=0|example:flight-stick|normal|west-outer\nfighter=0|example:flight-stick|normal|west-rear");
    const auto third_enemy=text+"\nfighter=1|example:jump-stick|normal|east-outer";
    bad(third_enemy+"\nfighter=1|example:jump-stick|normal|east-far");
    const auto full=third_enemy+"\nfighter=0|example:flight-stick|normal|west-rear"
                                "\nfighter=0|example:flight-stick|normal|west-outer";
    bad(full+"\nfighter=0|example:flight-stick|normal|west-far");
    check(battle_roster_read_preset(full,&parsed,&error)&&parsed.fighters.size()==6&&error.empty(),
          "exact capacity of three fighters per team accepted");
    check(battle_roster_read_preset(coop,&parsed,&error)&&!parsed.rounds_to_win&&parsed.fighters[0].team==0&&parsed.fighters[1].team==0&&
          parsed.fighters[2].team==1,"co-op preserves the first two local slots and requires an opposing CPU");
    check(battle_roster_read_preset(coop+"\nrounds-to-win=2",&parsed,&error)&&parsed.rounds_to_win==2,
          "preset can opt in to a bounded series");
    check(battle_roster_read_preset(coop+"\nround-seconds=1",&parsed,&error)&&parsed.round_seconds==1&&!parsed.rounds_to_win,
          "single rounds can set a timer without scoring a series");
    check(battle_roster_read_preset(coop+"\nround-seconds=600\nrounds-to-win=2",&parsed,&error)&&
          parsed.round_seconds==600&&parsed.rounds_to_win==2,"timer and series are independent options");
    check(parsed.locals==2&&battle_roster_read_preset(coop+"\nlocals=1",&parsed,&error)&&parsed.locals==1&&
          parsed.fighters[1].team==0&&parsed.fighters[2].team==1,
          "one-local preset preserves the ordered ally and opponent slots; omitted locals still means two");
}
