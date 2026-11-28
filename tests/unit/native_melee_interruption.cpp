// Scripted native Tau-shaped Battle Test control; no original game files.
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;

constexpr std::string_view normal=R"(GFBORG-GAMEPLAY 1
variant=normal
health=60
movement=grounded-v1|5.00|13|15
rig-prefix=visual/body.prefix
pose-profile=visual/body.pose
body-record=0
idle=0:0
walk=0:1
air=0:24
dash=0:2
move=opening-slash|b|10|grounded|24|1:1
range=opening-slash|150
melee=opening-slash|9|14|88|38|52|44|12
)";

void check(bool ok,const char* message){
    if(!ok){std::fprintf(stderr,"native melee interruption: %s\n",message);std::abort();}
}

struct Hit {unsigned tick=0;u32 object=0;};

void observe(bool interrupted){
    BorgGameplayProfile parsed;
    check(borg_read_gameplay_profile(normal,&parsed,nullptr),"authored Tau opening");
    auto profile=std::make_shared<const BorgGameplayProfile>(std::move(parsed));
    BattleMvpConfig config;
    config.packages={profile,profile};
    config.spawn={BattlePoint{0,0},BattlePoint{0,100}};
    NativeBattleMvp world;
    check(world.start(config),"battle opens");
    std::vector<unsigned> starts,arms,cancels,retirements;
    std::vector<Hit> inflicted,received;
    for(unsigned visit=1;visit<=48;++visit){
        std::array<GamePadState,kBattleMvpMaxActors> pads{};
        std::array<BorgBattleInputs,kBattleMvpMaxActors> inputs{};
        if(interrupted&&visit==1)inputs[1].pressed=inputs[1].held=BorgBattleInputs::b;
        if(visit==8||(interrupted&&visit==34))inputs[0].pressed=BorgBattleInputs::b;
        if(8<=visit&&visit<33)inputs[0].held=BorgBattleInputs::b;
        if(interrupted&&visit==33)inputs[0].released=BorgBattleInputs::b;
        check(world.step_roster(pads,inputs,false),"scripted melee visit");
        for(const auto& e:world.package_events()){
            check(e.kind!=BorgBattleEventKind::StockSpent&&e.kind!=BorgBattleEventKind::Emitted,
                  "an ordinary melee needs no projectile carrier or debit");
            if(e.kind==BorgBattleEventKind::Hit){
                check(e.actor!=e.recipient&&e.value==12&&e.object,"exact authored melee damage with distinct attacking body");
                (e.actor==0?inflicted:received).push_back({e.tick,e.object});
            }
            if(e.actor!=0)continue;
            switch(e.kind){
            case BorgBattleEventKind::Started: starts.push_back(e.tick);break;
            case BorgBattleEventKind::MeleeArmed: arms.push_back(e.tick);break;
            case BorgBattleEventKind::Interrupted: cancels.push_back(e.tick);break;
            case BorgBattleEventKind::MeleeRetired: retirements.push_back(e.tick);break;
            default:break;
            }
        }
    }
    check(starts==(interrupted?std::vector<unsigned>{8,34}:std::vector<unsigned>{8}),"scripted input start and deliberate restart");
    check(arms==std::vector<unsigned>{interrupted?43u:17u},"pending melee did not arm after interruption");
    check(cancels==(interrupted?std::vector<unsigned>{11}:std::vector<unsigned>{}),"positive damage interrupts on the next native update");
    check(inflicted.size()==1&&inflicted[0].tick==arms[0],"uninterrupted or fresh melee wrote HP once");
    check(received.size()==unsigned(interrupted)&&(!interrupted||received[0].tick==10),"real incoming contact before the arming update");
    if(interrupted)
        check(received[0].object!=inflicted[0].object,"fresh melee has its own identity after cancellation");
    check(retirements==(interrupted?std::vector<unsigned>{}:std::vector<unsigned>{23}),"no retirement for a window that did not arm");
    check(world.actors()[0].health==(interrupted?48u:60u)&&world.actors()[1].health==48&&
          world.actors()[0].damage_credited==12&&world.actors()[1].damage_credited==(interrupted?12u:0u),
          "interruption did not mint a hit or alter credit");
    std::printf("native-melee: %u %u %u %u %u %u %u %u %u\n",unsigned(interrupted),starts[0],
                received.empty()?0:received[0].tick,cancels.empty()?0:cancels[0],interrupted?starts[1]:0,
                arms[0],inflicted[0].tick,world.actors()[0].health,world.actors()[1].health);
}
} // namespace

int main(){
    observe(false);
    observe(true);
}
