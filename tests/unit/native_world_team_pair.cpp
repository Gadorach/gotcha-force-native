// Authored three-actor contrast; no retail shapes or damage operands.
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;

void check(bool pass,const char* what){
    if(!pass){std::fprintf(stderr,"native team pair: %s\n",what);std::abort();}
}

constexpr std::string_view rules=R"(GFBORG-GAMEPLAY 1
variant=normal
health=60
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

void observe(unsigned ally){
    auto profile=std::make_shared<BorgGameplayProfile>();
    check(borg_read_gameplay_profile(rules,profile.get(),nullptr),"parse authored profile");
    BattleMvpConfig c;
    c.declared_actors=3;c.actor_radius=30;
    c.packages={profile,profile,profile};
    c.teams_declared=true;c.teams={0,u8(ally!=1),u8(ally!=2)};
    c.spawn={BattlePoint{0,0},BattlePoint{-32,126},BattlePoint{32,126}};
    const unsigned enemy=3-ally;
    NativeBattleMvp world;
    check(world.start(c)&&world.actor_count()==3,"start authored three-actor roster");
    unsigned hits=0,retired=0;
    u32 object=0;
    for(unsigned n=1;n<=24;++n){
        std::array<GamePadState,kBattleMvpMaxActors> pads{};
        std::array<BorgBattleInputs,kBattleMvpMaxActors> moves{};
        if(n>=11)pads[1].dpad_up=pads[2].dpad_up=true;
        if(n==8)moves[0].pressed=BorgBattleInputs::b;
        if(n>=8)moves[0].held=BorgBattleInputs::b;
        check(world.step_roster(pads,moves,false),"run authored roster step");
        for(const auto& e:world.package_events()){
            check(e.actor==0,"only the attacker owns events");
            if(e.kind==BorgBattleEventKind::MeleeArmed){check(n==17&&!object&&e.object,"one armed window");object=e.object;}
            if(e.kind==BorgBattleEventKind::Hit){
                check(n==17&&!hits&&e.object==object&&e.recipient==enemy&&e.value==12,"only the opponent takes damage");
                ++hits;
            }
            if(e.kind==BorgBattleEventKind::MeleeRetired){check(n==23&&e.object==object,"same window retires");++retired;}
        }
        if(n==17){
            const auto& a=world.actors()[0];
            const auto& proxy=*profile->moves[0].melee;
            for(unsigned i:{1u,2u}){
                const auto p=world.actors()[i].position;
                f32 fraction=0;
                check(battle_swept_hit({a.position[0],a.position[2]},{a.facing.x*proxy.reach,a.facing.z*proxy.reach},
                                       {p[0],p[2]},c.actor_radius+proxy.radius,&fraction),"both authored proxies overlap");
            }
        }
    }
    check(hits==1&&retired==1&&world.actors()[ally].health==60&&world.actors()[enemy].health==48&&
          world.actors()[0].damage_credited==12,"independent team and geometric gates");
    std::printf("native-team-pair: %u %u 17 60 48 12 23\n",ally,enemy);
}
}

int main(){
    observe(1);observe(2);
}
