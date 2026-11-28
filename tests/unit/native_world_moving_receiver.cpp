// Authored control: the current moving receiver is sampled after motion.
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;

void check(bool pass,const char* what){
    if(!pass){std::fprintf(stderr,"native moving receiver: %s\n",what);std::abort();}
}

constexpr std::string_view rules=R"(GFBORG-GAMEPLAY 1
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

void observe(bool away){
    auto profile=std::make_shared<BorgGameplayProfile>();
    check(borg_read_gameplay_profile(rules,profile.get(),nullptr),"authored Tau-shaped profile parses");
    BattleMvpConfig c;
    c.packages={profile,profile};
    c.spawn={BattlePoint{0,0},BattlePoint{0,147}};
    NativeBattleMvp world;
    check(world.start(c),"authored battle opens at common spacing");
    unsigned hits=0,arms=0,starts=0,returns=0;
    u32 object=0;
    for(unsigned visit=1;visit<=24;++visit){
        std::array<GamePadState,kBattleMvpMaxActors> pads{};
        std::array<BorgBattleInputs,kBattleMvpMaxActors> actions{};
        if(visit==8)actions[0].pressed=actions[0].held=BorgBattleInputs::b;
        if(visit>8&&visit<=24)actions[0].held=BorgBattleInputs::b;
        pads[1].dpad_down=away&&visit>=11;
        pads[1].dpad_up=!away&&visit>=11;
        const auto previous=world.actors()[1].position;
        check(world.step_roster(pads,actions,false),"both authored actors take a visit");
        const auto& attacker=world.actors()[0];
        const auto& receiver=world.actors()[1];
        for(const auto& e:world.package_events()){
            check(e.actor==0&&e.kind!=BorgBattleEventKind::StockSpent&&e.kind!=BorgBattleEventKind::Emitted,
                  "only the authored melee may publish events");
            if(e.kind==BorgBattleEventKind::Started){check(visit==8,"shared start");++starts;}
            if(e.kind==BorgBattleEventKind::MeleeArmed){check(visit==17&&e.object,"shared arming");object=e.object;++arms;}
            if(e.kind==BorgBattleEventKind::Hit){
                check(!away&&visit==17&&e.object==object&&e.recipient==1&&e.value==12,"authored accepted contact and HP amount");
                ++hits;
            }
            if(e.kind==BorgBattleEventKind::MeleeRetired){check(visit==23&&e.object==object,"shared retirement");++returns;}
        }
        if(visit==17){
            const auto& body=*profile->moves[0].melee;
            const BattlePoint from{attacker.position[0],attacker.position[2]};
            const BattlePoint reach{attacker.facing.x*body.reach,attacker.facing.z*body.reach};
            const f32 radius=c.actor_radius+body.radius;
            const auto overlaps=[&](BattleMvpVec p){
                f32 fraction=0;
                return battle_swept_hit(from,reach,{p[0],p[2]},radius,&fraction);
            };
            check(previous!=receiver.position&&overlaps(previous)&&overlaps(receiver.position)==!away&&
                  receiver.health==(away?60:48)&&hits==unsigned(!away),"current proxy changes outcome; stale proxy would hit on both arms");
            std::printf("native-receiver: %u %u %.2f %.2f %u %u %u %u\n",unsigned(away),visit,
                        previous[2],receiver.position[2],unsigned(overlaps(previous)),unsigned(overlaps(receiver.position)),
                        hits,receiver.health);
        }
    }
    check(starts==1&&arms==1&&returns==1&&hits==unsigned(!away),"one matched window with or without damage");
}
}

int main(){
    observe(false);
    observe(true);
}
