#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;

const std::string mover=R"(GFBORG-GAMEPLAY 1
variant=normal
health=180
movement=grounded-v1|5.00|13|15
rig-prefix=visual/body.prefix
pose-profile=visual/body.pose
idle=0:0
walk=0:1
air=0:7
dash=0:2
move=kick|b|10|grounded|14|1:1
melee=kick|4|6|88|20|52|44|12
)";
const std::string jump_points="air-jump-points=3\n";
const std::string policy="air-dash-point=seed-if-zero\nair-point-reset=next-grounded-update\n";

void check(bool value,const char* what){
    if(!value){std::fprintf(stderr,"native air-point policy: %s\n",what);std::abort();}
}
std::shared_ptr<const BorgGameplayProfile> load(std::string_view rules){
    BorgGameplayProfile p;
    std::string error;
    if(!borg_read_gameplay_profile(rules,&p,&error)){std::fputs(error.c_str(),stderr);std::abort();}
    return std::make_shared<const BorgGameplayProfile>(std::move(p));
}
struct Ledge final:IBattleMvpTerrain {
    bool height(f32 x,f32,f32,f32* out)const override{*out=x<20?100.f:0.f;return true;}
};

void step(NativeBattleMvp& world,GamePadState pad={},const IBattleMvpTerrain* terrain=nullptr){
    std::array<GamePadState,kBattleMvpMaxActors> pads{};
    pads[0]=pad;
    const std::array<BorgBattleInputs,kBattleMvpMaxActors> actions{};
    check(world.step_roster(pads,actions,false,terrain),"native movement visit");
}
BattleMvpConfig config(std::string_view rows){
    BattleMvpConfig c;
    c.packages={load(mover+std::string(rows)),load(mover)};
    c.spawn={BattlePoint{0,0},BattlePoint{0,800}};
    return c;
}

void refuse(){
    BorgGameplayProfile p;
    for(const auto& rules:std::array{
          mover+policy,mover+"air-action-budget=3\n"+policy,mover+jump_points+policy+policy,
          mover+jump_points+"air-dash-point=spend\n",mover+jump_points+"air-point-reset=immediate\n",
          mover+jump_points+policy+"air-dash=none\n",
        })check(!borg_read_gameplay_profile(rules,&p,nullptr),"unowned, mixed or duplicate policy accepted");
}

void jumps_and_landing(bool counted){
    NativeBattleMvp world;
    check(world.start(config(counted?jump_points+policy:"air-action-budget=3\n")),"jump fixture starts");
    GamePadState jump{};jump.b=true;
    for(unsigned used=1;used<=3;++used){
        step(world,jump);
        check(world.actors()[0].package_air_points_used==used&&!world.actors()[0].grounded,
              "accepted ground launch and airborne jump count once");
        step(world);
    }
    step(world,jump);
    check(world.actors()[0].package_air_points_used==3&&
          !(world.actors()[0].package_movement_cues&u8(BorgMovementCue::AirJump)),
          "fresh jump at cap is refused");
    GamePadState dash{};dash.l=true;dash.dpad_right=true;
    step(world,dash);
    const auto dashes=world.actors()[0].air_dashes_used;
    check(dashes==unsigned(counted)&&world.actors()[0].package_air_points_used==3,
          "separate dash at cap leaves the jump count unchanged");
    for(unsigned i=0;i<200&&!world.actors()[0].grounded;++i)step(world);
    const auto at_landing=world.actors()[0].package_air_points_used;
    check(world.actors()[0].grounded&&world.actors()[0].landing_ticks&&at_landing==(counted?3u:0u),
          "floor contact and authored reset order differ only when requested");
    step(world);
    check(world.actors()[0].package_air_points_used==0,"next grounded update resets used points");
    step(world,jump);
    check(world.actors()[0].package_air_points_used==1,"fresh takeoff can use the reset count");
    std::printf("native-air-jump: %u 1 2 3 3 %u %u 0 1\n",unsigned(counted),dashes,at_landing);
}

void ledge_dash(bool counted){
    NativeBattleMvp world;
    Ledge terrain;
    check(world.start(config(jump_points+(counted?policy:"")),&terrain),"raised-floor fixture starts");
    GamePadState right{};right.dpad_right=true;
    for(unsigned i=0;i<24&&world.actors()[0].grounded;++i)step(world,right,&terrain);
    check(!world.actors()[0].grounded&&world.actors()[0].package_air_points_used==0,
          "walk-off leaves an unspent airborne count");
    step(world,right,&terrain);
    check(!world.actors()[0].grounded&&world.actors()[0].package_air_points_used==0,
          "airborne visit without dash keeps zero");
    right.l=true;step(world,right,&terrain);
    const auto used=world.actors()[0].package_air_points_used;
    check(world.actors()[0].air_dashes_used==1&&used==unsigned(counted),"only opted-in air dash seeds zero");
    step(world,right,&terrain);
    check(world.actors()[0].package_air_points_used==used,"holding the dash does not seed again");
    check(world.actors()[0].dash_ticks>1,"relaunch starts from an active native dash");
    right.b=true;step(world,right,&terrain);
    check(world.actors()[0].package_air_points_used==used+1&&!world.actors()[0].dash_ticks&&
          (world.actors()[0].package_movement_cues&u8(BorgMovementCue::AirJump)),
          "accepted relaunch spends one point and cuts the native dash");
    std::printf("native-air-ledge: %u 0 %u\n",unsigned(counted),used);
}

void ground_dash_does_not_seed(){
    NativeBattleMvp world;
    check(world.start(config(jump_points+policy)),"ground dash fixture starts");
    GamePadState pad{};pad.l=true;pad.dpad_right=true;
    step(world,pad);
    const auto& actor=world.actors()[0];
    check(actor.grounded&&actor.dash_ticks&&!actor.dash_airborne&&!actor.package_air_points_used,
          "seed applies only to accepted airborne dashes");
}
} // namespace

int main(){
    refuse();
    jumps_and_landing(false);
    jumps_and_landing(true);
    ledge_dash(false);
    ledge_dash(true);
    ground_dash_does_not_seed();
}
