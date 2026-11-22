#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;
constexpr std::string_view wingmate=R"(GFBORG-GAMEPLAY 1
variant=normal
health=180
movement=grounded-v1|7|13|14
rig-prefix=visual/body.prefix
pose-profile=visual/body.pose
idle=0:0
walk=0:1
air=0:7
dash=0:2
stock=beam-stock|1|200
projectile=beam|beam|1|60|0|0|0|8|100|10|ignore-floor
projectile=wing|beam|1|45|0|0|0|8|100|7|ignore-floor
move=primary|b|10|any|4|0:1
release=primary|2|beam-stock|1|beam
move=deploy|x|20|any|6|0:2
emitter=left|primary|wing|-35|82|28|30
emitter=right|primary|wing|35|82|28|30
deploy-emitter=deploy|2|left
deploy-emitter=deploy|2|right
)";
constexpr std::string_view dummy=R"(GFBORG-GAMEPLAY 1
variant=normal
health=500
movement=stationary-v1
rig-prefix=visual/body.prefix
pose-profile=visual/body.pose
idle=0:0
)";
void check(bool ok,const char* message){
    if(!ok){std::fprintf(stderr,"Borg emitter: %s\n",message);std::abort();}
}
std::shared_ptr<const BorgGameplayProfile> parse(std::string_view text){
    BorgGameplayProfile out;std::string error;
    if(!borg_read_gameplay_profile(text,&out,&error)){std::fprintf(stderr,"%s\n",error.c_str());std::abort();}
    return std::make_shared<const BorgGameplayProfile>(std::move(out));
}
bool near(f32 a,f32 b){return std::abs(a-b)<.001f;}
}

int main(){
    using enum BorgBattleEventKind;
    auto profile=parse(wingmate);
    BorgGameplayProfile refused;
    for(auto line:{"emitter=left|primary|wing|35|82|28|30","emitter=extra|deploy|wing|35|82|28|30",
                   "deploy-emitter=deploy|3|left","deploy-emitter=deploy|6|right","emitter=extra|primary|wing|121|82|28|30",
                   "return-stock=wing|expiry"})
        check(!borg_read_gameplay_profile(std::string(wingmate)+line,&refused,nullptr),
            "bad references, bounds or stock ownership refuse");
    const auto target=parse(dummy);
    BattleMvpConfig config;config.packages={profile,target};config.spawn={BattlePoint{},BattlePoint{0,1000}};
    NativeBattleMvp world;
    const auto step=[&](u8 pressed=0,bool right=false){
        GamePadState pad;pad.dpad_right=right;
        check(world.step({pad,{}},false,nullptr,nullptr,nullptr,nullptr,{BorgBattleInputs{pressed,0,0},{}}),"world update");
    };
    const auto count=[&](BorgBattleEventKind kind){
        const auto events=world.package_events();
        return std::count_if(events.begin(),events.end(),[&](auto e){return e.kind==kind;});
    };
    const auto advance=[&](unsigned ticks){while(ticks--)step();};
    check(world.start(config),"start supported package pair");
    const auto scope=world.presentation_scope();
    step(BorgBattleInputs::x);step();
    check(!world.emitters()[0][0].id&&!world.shots_spawned(),"deployment waits for the marker");
    step();
    const auto left=world.emitters()[0][0],right=world.emitters()[0][1];
    check(count(EmitterDeployed)==2&&left.id&&right.id&&left.id!=right.id&&world.actor_count()==2&&
          left.position==BattleMvpVec{-35,82,-28}&&right.position==BattleMvpVec{35,82,-28}&&!world.shots_spawned(),
          "hover offsets and separate emitter identities, without creating actors or projectiles");
    step(0,true);step(0,true);
    const auto& body=world.actors()[0];const auto& follow=world.emitters()[0][0];
    check(follow.id==left.id&&follow.remaining==28&&follow.position!=left.position&&
          near(follow.position[0],body.position[0]-35*body.facing.z-28*body.facing.x)&&
          near(follow.position[1],body.position[1]+82)&&
          near(follow.position[2],body.position[2]+35*body.facing.x-28*body.facing.z),
          "emitter follows the accepted root and body heading, not a camera or aim transform");
    advance(2);
    step(BorgBattleInputs::b);step();step();
    const auto events=world.package_events();
    check(events.size()==5&&events[0].kind==Marker&&events[1].kind==StockSpent&&events[2].kind==Emitted&&
        events[3].kind==EmitterFired&&events[4].kind==EmitterFired&&
        world.shots_spawned()==3&&world.package_action(0).stocks[0].current==0,
        "accepted owner volley precedes two child allocations with one debit");
    for(unsigned i=0;i<2;++i){
        const auto& shot=world.projectiles()[i+1];
        check(shot.owner==0&&shot.package_emitter==world.emitters()[0][i].id&&shot.package_return_stock==255&&
              shot.previous==world.emitters()[0][i].position,"child uses emitter's position, but owner owns credit and no paid channel");
    }
    advance(2);
    step(BorgBattleInputs::b);step();step();
    check(count(StockRefused)==1&&!count(EmitterFired)&&world.shots_spawned()==3,
          "empty stock creates neither main volley nor followers");
    advance(2);
    step(BorgBattleInputs::x);step();step();
    check(count(EmitterDeployRefused)==2&&world.emitters()[0][0].id==left.id&&world.emitters()[0][0].remaining==13,
          "redeployment does not replace and refresh a still-live emitter");
    advance(13);
    check(count(EmitterRetired)==2&&!world.emitters()[0][0].id&&!world.emitters()[0][1].id,
          "expiry retires both independent emitters");
    step(BorgBattleInputs::x);advance(2);
    check(world.emitters()[0][0].id&&world.start(config)&&!world.owns_motion_scope(scope)&&!world.emitters()[0][0].id,
          "restart discards live hover state and allocator identity");

    auto saturated=*profile;saturated.stocks[0].capacity=20;
    for(auto& shot:saturated.projectiles){shot.count=8;shot.speed=1;shot.lifetime=600;}
    for(auto& emitter:saturated.emitters)emitter.lifetime=600;
    config.packages[0]=std::make_shared<const BorgGameplayProfile>(saturated);
    check(world.start(config),"pool capacity fixture");
    step(BorgBattleInputs::x);advance(6);
    for(unsigned i=0;i<5;++i){step(BorgBattleInputs::b);advance(4);}
    check(world.shots_spawned()==120,"main and companion volleys share one bounded pool");
    step(BorgBattleInputs::b);advance(2);
    check(count(Emitted)==1&&count(EmitterFireRefused)==2&&world.shots_spawned()==128&&
        world.package_action(0).stocks[0].current==14,"paid main shot can succeed while both followers refuse independently");
    advance(2);step(BorgBattleInputs::b);advance(2);
    check(count(EmitRefused)==1&&!count(EmitterFireRefused)&&!count(EmitterFired)&&world.shots_spawned()==128&&
        world.package_action(0).stocks[0].current==13,"failed main allocation cannot signal either emitter");
    std::puts("Borg emitters: owner anchors, synchronized allocations, no extra debit, expiry and restart PASS");
}
