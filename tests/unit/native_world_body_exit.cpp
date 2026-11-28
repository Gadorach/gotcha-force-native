// Authored Tau-shaped melee shows native round termination, not a retail handoff.
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/portable_battle_duel.hpp"

namespace {
using namespace gf;

void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"native body exit: %s\n",why);std::abort();}
}

std::shared_ptr<const BorgGameplayProfile> profile(unsigned health){
    const std::string rows="GFBORG-GAMEPLAY 1\nvariant=normal\nhealth="+std::to_string(health)+R"(
movement=grounded-v1|5|13|15
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
    auto p=std::make_shared<BorgGameplayProfile>();
    check(borg_read_gameplay_profile(rows,p.get(),nullptr),"authored profile");
    return p;
}

void observe(bool lethal){
    BattleMvpConfig c;
    c.packages={profile(60),profile(lethal?1:60)};
    c.spawn={BattlePoint{0,0},BattlePoint{0,100}};
    NativeBattleMvp world;
    check(world.start(c),"start with declared authored recipient health");
    unsigned hp_tick=0,amount=0,returned=0;
    u32 object=0;
    for(unsigned n=1;n<=24;++n){
        const std::array<GamePadState,kBattleMvpMaxActors> pads{};
        std::array<BorgBattleInputs,kBattleMvpMaxActors> input{};
        if(n==8)input[0].pressed=BorgBattleInputs::b;
        if(n>=8)input[0].held=BorgBattleInputs::b;
        check(world.step_roster(pads,input,false),"native dispatcher returns");
        if(lethal&&n>17)continue;
        for(const auto& e:world.package_events()){
            check(e.actor==0,"contact remains owned by attacker");
            if(e.kind==BorgBattleEventKind::MeleeArmed){check(!object&&e.object,"one armed object");object=e.object;}
            if(e.kind==BorgBattleEventKind::Hit){
                check(!hp_tick&&e.object==object&&e.recipient==1,"one accepted hit");
                hp_tick=e.tick;amount=e.value;
            }
            if(e.kind==BorgBattleEventKind::MeleeRetired){
                check(!returned&&hp_tick&&e.object==object,"armed window retires after the accepted hit");
                returned=e.tick;
            }
        }
    }
    const auto& a=world.actors()[0];
    const auto& b=world.actors()[1];
    check(hp_tick==17&&amount==(lethal?1u:12u)&&a.damage_credited==amount&&b.damage_taken==amount&&
        b.health==(lethal?0u:48u)&&returned==(lethal?17u:23u),"native amount and exact native object retirement");
    check(world.actor_count()==2&&(b.action==BattleMvpAction::Defeated)==lethal&&
        world.result()==(lethal?BattleMvpResult::GRedWins:BattleMvpResult::Fighting)&&world.tick()==(lethal?17u:24u),
        "team KO terminates the authored duel without replacing its seat");
    const auto result=portable_duel_end(world,0);
    check(result==(lethal?PortableDuelEnd::Win:PortableDuelEnd::Fighting)&&
          portable_duel_campaign_outcome(result)==(lethal?std::optional{PortableBattleOutcome::Win}:std::nullopt),
          "the portable result bridge reports a completed win only for the lethal arm");
    std::printf("native-body-exit: %u %u %u %u %u %u %u\n",
        unsigned(lethal),hp_tick,amount,b.health,a.damage_credited,returned,world.tick());
}
}

int main(){
    observe(false);
    observe(true);
}
