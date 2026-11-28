// The current authored flying-boss profile is not a fixed-airborne controller.
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;

void check(bool pass,const char* what){
    if(!pass){std::fprintf(stderr,"native Emperor movement contrast: %s\n",what);std::abort();}
}

std::shared_ptr<const BorgGameplayProfile> read(std::string_view rows){
    auto out=std::make_shared<BorgGameplayProfile>();
    std::string text="GFBORG-GAMEPLAY 1\nvariant=normal\nrig-prefix=body.prefix\npose-profile=body.pose\n";
    text+=rows;
    std::string error;
    if(!borg_read_gameplay_profile(text,out.get(),&error)){std::fprintf(stderr,"%s\n",error.c_str());std::abort();}
    return out;
}

struct Flat final:IBattleMvpTerrain {
    bool height(f32,f32,f32,f32* out)const override{*out=120.f;return true;}
};

void step(NativeBattleMvp& world,const Flat& terrain,GamePadState first={}){
    std::array<GamePadState,kBattleMvpMaxActors> pads{};
    const std::array<BorgBattleInputs,kBattleMvpMaxActors> actions{};
    pads[0]=first;pads[1].dpad_down=true;
    check(world.step_roster(pads,actions,false,&terrain),"native actor visit");
}
}

int main(){
    const auto authored=read(R"(health=2000
movement=grounded-v1|4.17|13|15
body-record=0
body-motion=static-cruise
cruise-boost=50|10000|1|2000|0|30
field=wing-hit|3|sphere|240|240|240|30|0|40|190|90|220|120
field-follow=wing-hit|source
field-core-damage=wing-hit|enemies|240|10|6
move=wing-hit|b|10|airborne|28|none
)");
    const auto opposed=read(R"(health=200
movement=grounded-v1|4|13|15
idle=0:0
walk=0:1
air=0:7
dash=0:2
move=kick|b|10|grounded|14|1:1
melee=kick|4|6|88|20|52|44|12
)");
    const auto target=read("health=2000\nmovement=stationary-v1\nidle=0:0\n");
    const Flat terrain;
    BattleMvpConfig config;
    config.spawn={BattlePoint{-100,100},BattlePoint{450,400}};
    config.packages={authored,opposed};
    NativeBattleMvp world;
    check(world.start(config,&terrain),"unchanged authored boss profile starts");
    const auto origin=world.actors()[0];
    check(origin.position==BattleMvpVec{-100,170,100}&&!origin.grounded,"authored offset follows floor, not a world-space pin");
    step(world,terrain);
    const auto neutral=world.actors()[0];
    check(neutral.position[1]==170&&!neutral.grounded&&neutral.position!=origin.position&&
          neutral.package_boost_meter.value736==10000,"neutral authored cruise advances");
    GamePadState input{};input.dpad_left=true;input.b=true;
    step(world,terrain,input);
    const auto& steered=world.actors()[0];
    check(steered.facing!=neutral.facing&&steered.package_boosting&&steered.package_boost_meter.value736==9999&&
          steered.position[1]==170&&world.actors()[1].position[2]>400,"authored steering and boost operate with a moving peer");
    std::puts("emperor-native: cruise 170 1 1 1 9999");

    config.packages[0]=target;
    check(world.start(config,&terrain),"ordinary stationary target starts");
    const auto initial=world.actors()[0].position;
    step(world,terrain,input);
    const auto& stationary=world.actors()[0];
    check(stationary.grounded&&stationary.position==initial&&initial[1]==120&&!stationary.package_boosting&&
          target->moves.empty(),"stationary-v1 is grounded and cannot carry the authored move");
    std::puts("emperor-native: stationary 120 0 0 0 0");
}
