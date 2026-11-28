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

void observe(unsigned ally,unsigned low_hp=0,bool report=true){
    auto profile=std::make_shared<BorgGameplayProfile>();
    check(borg_read_gameplay_profile(rules,profile.get(),nullptr),"parse authored profile");
    BattleMvpConfig c;
    c.declared_actors=3;c.actor_radius=30;
    c.packages={profile,profile,profile};
    c.teams_declared=true;c.teams={0,u8(ally!=1),u8(ally!=2)};
    c.spawn={BattlePoint{0,0},BattlePoint{-32,126},BattlePoint{32,126}};
    const unsigned enemy=3-ally;
    const bool enemy_ko=low_hp==enemy;
    const unsigned dealt=enemy_ko?1:12,retirement=enemy_ko?17:23;
    if(low_hp){
        auto low=std::make_shared<BorgGameplayProfile>(*profile);
        low->health=1;c.packages[low_hp]=low;c.team_force={90,90};
    }
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
                check(n==17&&!hits&&e.object==object&&e.recipient==enemy&&e.value==dealt,"only the opponent takes damage");
                ++hits;
            }
            if(e.kind==BorgBattleEventKind::MeleeRetired){check(n==retirement&&e.object==object,"same window retires");++retired;}
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
        if(enemy_ko&&n==17)break;
    }
    check(hits==1&&retired==1&&world.actors()[ally].health==(low_hp==ally?1u:60u)&&
          world.actors()[enemy].health==(enemy_ko?0u:48u)&&world.actors()[0].damage_credited==dealt&&
          world.result()==(enemy_ko?BattleMvpResult::GRedWins:BattleMvpResult::Fighting),
          "independent team and geometric gates");
    if(low_hp){
        check(world.team_force()[0]==90&&world.team_force()[1]==90-dealt,"native HP debit is not the original KO cost");
        if(report)std::printf("native-allied-ko: %u %u %u %u %u %u %u\n",low_hp,world.actors()[ally].health,
                              world.actors()[enemy].health,dealt,world.team_force()[0],world.team_force()[1],unsigned(enemy_ko));
    }else if(report)std::printf("native-team-pair: %u %u 17 60 48 12 23\n",ally,enemy);
}
}

int main(int argc,char** argv){
    if(argc==2&&std::string_view(argv[1])=="--allied-ko"){observe(2,1);observe(2,2);return 0;}
    check(argc==1,"unknown output mode");
    observe(1);observe(2);
    observe(2,1,false);observe(2,2,false);
}
