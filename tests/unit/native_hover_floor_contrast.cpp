#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_type2_glide.hpp"
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;

void check(bool pass,const char* what){
    if(!pass){std::fprintf(stderr,"native floor contrast: %s\n",what);std::abort();}
}

void resource_edges(){
    const CharacterAirResourceState at_contact{4000,0,1};
    CharacterAirResourceState nurse_landing,nurse_after,wing_landing,wing_after,inhibited;
    check(character_air_resource_recover(at_contact,0,0x40,1,10000,2000,&nurse_landing)&&
        character_air_resource_recover(nurse_landing,0,0x200,0,10000,2000,&nurse_after),
        "type-zero recovery consumes the previous grounded cache, not the snap");
    const auto wing=NormalType2GlideSource{0x0a00,0};
    check(normal_type2_glide_refill(wing,at_contact,0x40,&wing_landing)&&
        normal_type2_glide_refill(wing,wing_landing,0x200,&wing_after)&&
        normal_type2_glide_refill(wing,at_contact,0xC0,&inhibited),
        "Wing's flag-gated meter can replenish while airborne");
    check(nurse_landing.value736==4000&&nurse_landing.air_actions747==1&&
        nurse_after.value736==6000&&!nurse_after.air_actions747&&
        wing_landing.value736==6000&&wing_landing.air_actions747==1&&
        wing_after.value736==8000&&wing_after.air_actions747==1&&inhibited.value736==4000,
        "detached recovery matches the two source landing forks, and respects inhibition");
    std::printf("hover-native-resource: %d %u %d %u %d %u %d %u %d\n",
        nurse_landing.value736,unsigned(nurse_landing.air_actions747),nurse_after.value736,unsigned(nurse_after.air_actions747),
        wing_landing.value736,unsigned(wing_landing.air_actions747),wing_after.value736,unsigned(wing_after.air_actions747),
        inhibited.value736);
}

std::shared_ptr<const BorgGameplayProfile> profile(bool visual){
    const std::string rows=R"(GFBORG-GAMEPLAY 1
variant=normal
health=150
movement=grounded-v1|5|13|15
rig-prefix=body.prefix
pose-profile=body.pose
idle=0:0
walk=0:1
air=0:7
dash=0:2
air-action-budget=3
move=kick|b|10|grounded|14|1:1
melee=kick|4|6|88|20|52|44|12
)";
    std::string error;
    auto out=std::make_shared<BorgGameplayProfile>();
    check(borg_read_gameplay_profile(rows+(visual?"pose-ground-offset=-24\n":""),out.get(),&error),
          "authored profile accepts optional presentation offset");
    return out;
}

struct Flat final:IBattleMvpTerrain {
    bool height(f32,f32,f32,f32* out)const override{*out=10;return true;}
};

void authored_control(){
    BattleMvpConfig c;
    c.spawn={BattlePoint{0,0},BattlePoint{450,400}};
    c.packages={profile(false),profile(false)};
    const Flat terrain;
    NativeBattleMvp baseline,shifted;
    check(baseline.start(c,&terrain),"authored native baseline starts");
    c.packages[0]=profile(true);
    check(shifted.start(c,&terrain),"visual-only alternate starts at same floor");
    bool landed=false;
    for(unsigned visit=0;visit<180&&!landed;++visit){
        std::array<GamePadState,kBattleMvpMaxActors> pads{};
        const std::array<BorgBattleInputs,kBattleMvpMaxActors> actions{};
        pads[0].b=!visit;
        pads[1].dpad_down=visit<20;
        check(baseline.step_roster(pads,actions,false,&terrain)&&shifted.step_roster(pads,actions,false,&terrain),
              "advance authored peers without changing collision data from pose");
        const auto& a=baseline.actors()[0];
        const auto& b=shifted.actors()[0];
        check(a.position==b.position&&a.grounded==b.grounded&&a.package_air_points_used==b.package_air_points_used&&
              a.vertical_speed==b.vertical_speed,"presentation offset does not move the logical floor or resource gate");
        if(!visit)check(!a.grounded&&a.package_air_points_used==1,"normal authored takeoff");
        if(a.grounded){
            check(a.position[1]==10&&!a.package_air_points_used&&baseline.actors()[1].position[2]>400,
                  "authored combined budget resets at touchdown, unlike the Nurse source count");
            landed=true;
        }
    }
    check(landed,"the authored floor control completed");
    std::puts("hover-native-authored: 10 10 0");
}
} // namespace

int main(){
    resource_edges();
    authored_control();
}
