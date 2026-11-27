#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;

void check(bool result,const char* message){
    if(!result){std::fprintf(stderr,"melee partitions: %s\n",message);std::exit(1);}
}

struct FlatStage : IBattleMvpTerrain {
    std::array<StageBodyPartition,1> boxes{{{"wall",-10,-3,0,3}}};
    std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
    bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
};

std::shared_ptr<const BorgGameplayProfile> profile(std::string_view body){
    BorgGameplayProfile parsed;
    std::string error;
    const bool valid=borg_read_gameplay_profile(
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|6|11|14\nhealth=200\n"
        "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nwalk=0:1\nair=0:2\ndash=0:3\n"+
        std::string(body),&parsed,&error);
    check(valid,error.c_str());
    return std::make_shared<const BorgGameplayProfile>(std::move(parsed));
}

struct Outcome {
    unsigned armed=0,guards=0,hits=0,near_hp=0,far_hp=0,force=0;
    u8 recipient=255;
};

Outcome fight(bool blocks_melee,bool roster,bool guard,u8 button){
    const auto attacker=profile(
        "move=slash|b|10|grounded|14|0:4\n"
        "melee=slash|4|7|190|40|55|42|20\n"
        "move=bolt|x|20|grounded|14|0:4\n"
        "projectile=shot|bullet|1|50|0|0|0|8|20|17|ignore-floor\nrelease=bolt|3|none|0|shot\n");
    const auto defender=profile("move=guard|x|10|grounded|14|0:4\nguard=guard|1|10|1|1|melee\n");
    FlatStage terrain;
    terrain.boxes[0].blocks_melee=blocks_melee;
    BattleMvpConfig config;
    config.packages={attacker,defender};
    config.spawn={BattlePoint{-5,-52},BattlePoint{-5,52},BattlePoint{-300,-200},BattlePoint{63,104}};
    config.team_force={500,500};
    if(roster){config.declared_actors=4;config.packages[2]=config.packages[3]=defender;}
    NativeBattleMvp world;
    check(world.start(config,&terrain),"non-overlapping bodies admit on either side");
    Outcome seen;
    for(unsigned tick=0;tick<20;++tick){
        std::array<GamePadState,4> pads{};
        std::array<BorgBattleInputs,4> inputs{};
        if(!tick){
            inputs[0].pressed=inputs[0].held=button;
            if(guard)inputs[1].pressed=inputs[1].held=BorgBattleInputs::x;
        }
        check(world.step_roster(pads,inputs,false,&terrain),"advance package action");
        for(const auto& event:world.package_events()){
            seen.armed+=event.kind==BorgBattleEventKind::MeleeArmed;
            seen.guards+=event.kind==BorgBattleEventKind::GuardBlocked;
            if(event.kind==BorgBattleEventKind::Hit)seen.recipient=event.recipient;
        }
    }
    seen.hits=world.hits();seen.force=world.team_force()[1];
    seen.near_hp=world.actors()[1].health;seen.far_hp=world.actors()[3].health;
    return seen;
}
}

int main(){
    const auto blocked=fight(true,false,true,BorgBattleInputs::b);
    check(blocked.armed==1&&!blocked.hits&&!blocked.guards&&blocked.near_hp==200&&
          blocked.force==500&&blocked.recipient==255,"occluded strike does not hit, spend guard, drain force, or claim a recipient");
    const auto plain=fight(false,false,false,BorgBattleInputs::b);
    check(plain.armed==1&&plain.hits==1&&plain.near_hp==180&&plain.recipient==1,
          "body-only partition does not imply melee occlusion");
    const auto unblocked_guard=fight(false,true,true,BorgBattleInputs::b);
    check(unblocked_guard.armed==1&&!unblocked_guard.hits&&unblocked_guard.guards==1,
          "unflagged fixture spends the first eligible recipient's guard exactly once");
    const auto around_edge=fight(true,true,true,BorgBattleInputs::b);
    check(around_edge.armed==1&&!around_edge.guards&&around_edge.hits==1&&around_edge.near_hp==200&&
          around_edge.far_hp==180&&around_edge.recipient==3,"blocked nearer body does not steal a visible contact around the end");
    const auto shot=fight(true,false,false,BorgBattleInputs::x);
    check(shot.hits==1&&shot.near_hp==183&&shot.recipient==1,"melee flag does not change projectile collision");
}
