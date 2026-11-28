#include <cstdio>
#include <cstdlib>
#include <string>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;
constexpr auto prefix=R"(GFBORG-GAMEPLAY 1
variant=normal
health=200
movement=grounded-v1|6|13|12
rig-prefix=body.prefix
pose-profile=body.pose
)";
constexpr auto plane="body-motion=static-cruise\n";

void check(bool ok,const char* what){
    if(!ok){std::fprintf(stderr,"native Blue Striker boost: %s\n",what);std::abort();}
}
std::shared_ptr<const BorgGameplayProfile> profile(std::string_view rows){
    auto p=std::make_shared<BorgGameplayProfile>();
    std::string error;
    if(!borg_read_gameplay_profile(std::string(prefix)+std::string(rows),p.get(),&error)){
        std::fprintf(stderr,"%s\n",error.c_str());std::abort();
    }
    return p;
}
BattleMvpConfig configure(std::string_view row){
    BattleMvpConfig c;
    c.arena_half=6000;
    c.spawn={BattlePoint{0,0},BattlePoint{1200,400}};
    c.packages={profile(std::string(plane)+std::string(row)+
                        "move=kick|b|10|airborne|14|none\nmelee=kick|4|6|88|20|52|44|12\n"),
                profile("idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\nmove=kick|b|10|grounded|14|1:1\nmelee=kick|4|6|88|20|52|44|12\n")};
    return c;
}
void tick(NativeBattleMvp& world,bool down,unsigned visit){
    std::array<GamePadState,kBattleMvpMaxActors> pad{};
    std::array<BorgBattleInputs,kBattleMvpMaxActors> actions{};
    pad[0].b=down;
    pad[0].dpad_left=true;
    pad[1].dpad_down=visit<20;
    check(world.step_roster(pad,actions,false),"both native actors advance");
    check(!world.actors()[0].grounded&&world.actors()[0].position[1]==240&&world.actors()[1].grounded,
          "resource check stays airborne above the authored native floor");
}

void matched(unsigned route){
    NativeBattleMvp world;
    check(world.start(configure("cruise-boost=240|10000|50|50|16|31\n")),"qualified scalar profile starts");
    bool prior_down=false;
    for(unsigned visit=1;visit<=(route==0?49u:route==1?54u:250u);++visit){
        const bool down=route==0?visit==1:route==1?visit<37:visit<=201||visit>=218;
        const bool fresh=down&&!prior_down;
        const auto old=world.actors()[0];
        tick(world,down,visit);
        const auto& now=world.actors()[0];
        const auto before=old.package_boost_meter;
        const auto after=now.package_boost_meter;
        const bool ready=old.package_boosting&&old.package_boost_age==31;
        // The authored age is an elapsed handler-visit counter, not the
        // original remaining clock. Project only the qualified unit-rate range.
        const unsigned clock=now.package_boosting?31u-now.package_boost_age:0;
        std::printf("blue-boost: %u %u %u %u %u %u %u %u %u %u %u %u %u\n",route,visit,
            unsigned(down),unsigned(fresh),old.package_boosting?28u:27u,unsigned(ready),
            unsigned(before.value736),unsigned(before.delay73a),now.package_boosting?28u:27u,
            unsigned(after.value736),unsigned(after.delay73a),
            unsigned(now.package_boosting&&now.package_boost_age==31),clock);
        const unsigned exit=route==0?32:route==1?37:201;
        if(visit==31)check(now.package_boosting&&now.package_boost_age==31&&after.value736==8450,
                           "entry plus 30 continues, no exit merely from opening the clock");
        if(visit==exit)check(!now.package_boosting&&after.value736==before.value736&&after.delay73a==16,
                             "exit after readiness, no exit debit or refill");
        if(visit>exit&&visit<=exit+16)
            check(!now.package_boosting&&after.value736==before.value736&&after.delay73a==int(exit+16-visit),
                  "only decrements delay, including the final delay visit");
        if(route<2&&visit==exit+17)check(!now.package_boosting&&after.value736==before.value736+50,
                                       "first refill follows the delay-only visits");
        if(route==2&&visit==200)check(now.package_boosting&&!after.value736&&after.delay73a==16,"held debit reaches zero before exit");
        if(route==2&&visit==218)check(now.package_boosting&&!after.value736&&after.delay73a==16&&clock==30,
                                     "fresh edge can spend first refill, leaving no meter for the pre-latch interval");
        if(route==2&&visit>218&&visit<=248)check(now.package_boosting&&!after.value736&&after.delay73a==16,
                                               "empty pre-latch continuations keep boost and pause replenishment");
        if(route==2&&visit==249)check(!now.package_boosting&&!after.value736&&after.delay73a==16,"held empty exits on the following visit");
        prior_down=down;
    }
    check(world.actors()[1].position[2]>400,"authored opposing actor moved independently of the boost comparison");
}

void early_release_control(std::string_view row,unsigned exit,unsigned first_refill){
    NativeBattleMvp world;
    const auto c=configure(row);
    check(world.start(c),"authored control starts");
    bool saw_exit=false,saw_refill=false;
    for(unsigned visit=1;visit<=first_refill;++visit){
        const auto before=world.actors()[0];
        tick(world,visit==1,visit);
        const auto& now=world.actors()[0];
        if(before.package_boosting&&!now.package_boosting){
            check(!saw_exit&&visit==exit,"release timing control differs only by the declared minimum");
            saw_exit=true;
        }
        if(now.package_boost_meter.value736>before.package_boost_meter.value736){
            check(!saw_refill&&visit==first_refill,"delay and exit order in the control");
            saw_refill=true;
        }
    }
    check(saw_exit&&saw_refill,"control observed both exit and refill");
    std::printf("blue-control: %u %u %u %u\n",c.packages[0]->cruise->capacity,
                c.packages[0]->cruise->minimum,exit,first_refill);
}
}

int main(){
    for(unsigned route=0;route<3;++route)matched(route);
    early_release_control("cruise-boost=240|240|12|12|8|4\n",5,14);
    early_release_control("cruise-boost=240|10000|50|50|16|30\n",31,48);
}
