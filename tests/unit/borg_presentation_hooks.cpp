#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borg_movement_audio.hpp"
#include "engine/graphics/scene_draw.hpp"
namespace {
using namespace gf;
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"%s\n",why);std::abort();}}
std::shared_ptr<const BorgGameplayProfile> parse(std::string_view text){
    BorgGameplayProfile p;std::string error;
    check(borg_read_gameplay_profile(text,&p,&error),error.c_str());
    return std::make_shared<const BorgGameplayProfile>(std::move(p));
}
struct Origin final:IBorgBattleOrigins {
    bool accept=true;
    mutable unsigned calls=0;
    bool socket(unsigned owner,const BattleMvpActor& actor,u32 tick,u8 move,u16 frame,const BorgBattleSocket& spec,
                BattleMvpVec* out)const override{
        ++calls;
        check(!owner&&tick==2&&!move&&frame==1&&spec.bone==7&&spec.local==BattleMvpVec{-50,0,20},
              "projectile uses its declared socket on the actual release tick");
        *out={actor.position[0]+40,65,actor.position[2]+10};return accept;
    }
};
}
int main(){
    using enum BorgBattleEventKind;
    constexpr std::string_view setup=R"(GFBORG-GAMEPLAY 1
variant=normal
health=180
movement=grounded-v1|7|13|14
rig-prefix=body.prefix
pose-profile=body.pose
idle=0:0
walk=0:1
air=0:7
dash=0:2
air-jump-points=3
stock=shots|4|150
projectile=star|shuriken|1|42|0|0|0|8|100|9|ignore-floor
origin=star|7|-50|0|20|target-height
look=star|64|122|218|255
move=throw|b|20|any|8|3:0
pose-start=throw|7
release=throw|1|shots|1|star
movement-effect=air|air-jump|puff|0|0|0|0|62|16
movement-sound=landing|audio/land.pcm
)";
    const auto p=parse(setup);
    check(p->moves[0].pose_start==7&&p->movement_fx.size()==1&&p->movement_sound[2]=="audio/land.pcm","loaded independent assets");
    BorgGameplayProfile invalid;
    for(const auto row:{"look=star|181|0|1|2","origin=star|1|0|0|0|fixed","movement-effect=invalid|rise|jet|0|0|0|0|50|1",
                        "movement-sound=takeoff|../outside.pcm"})
        check(!borg_read_gameplay_profile(std::string(setup)+row,&invalid,nullptr),"oversized or ambiguous binding refuses");
    BattleMvpConfig config;config.packages={p,p};config.spawn={BattlePoint{},BattlePoint{0,400}};
    NativeBattleMvp world;Origin origin;
    const auto step=[&](u8 attack=0,bool jump=false){
        GamePadState pad;pad.b=jump;
        check(world.step({pad,{}},false,nullptr,nullptr,nullptr,nullptr,{BorgBattleInputs{attack,0,0},{}},&origin),"world tick");
    };
    check(world.start(config),"bind");
    step(BorgBattleInputs::b);step();
    const auto& star=world.projectiles()[0];
    check(origin.calls==1&&star.previous==BattleMvpVec{40,65,10}&&star.weapon==BattleMvpWeapon::Shuriken&&
          star.package_look.size==64&&star.package_radius==8&&star.velocity[1]<0,"origin, image and collision are independent");
    origin.accept=false;check(world.start(config),"socket refusal world");step(BorgBattleInputs::b);step();
    check(!world.shots_spawned()&&std::any_of(world.package_events().begin(),world.package_events().end(),
        [](auto e){return e.kind==SocketRefused;}),"missing accepted socket never falls back");
    Camera camera;camera.eye[2]=10;camera.viewport_width=640;camera.viewport_height=480;
    std::vector<Vertex> triangles;const f32 center[]{0,0,0};
    check(ScreenDraw::shuriken(camera,center,2,0,{122,218,255,255},triangles)&&triangles.size()==24,
        "four-blade geometry rather than a bullet quad");
    BorgMovementSoundBank bank;std::string pcm{"\xd0\x07\xd0\x07",4};
    check(borg_movement_sound_decode(pcm,&bank[2])&&bank[2]==std::vector<s16>{2000,2000}&&
          !borg_movement_sound_decode("xx",&bank[1]),"bounded little-endian stereo decode");
    bank[1]=bank[2];bank[3]=bank[2];
    const std::array<const BorgMovementSoundBank*,2> banks{&bank,nullptr};
    BorgMovementAudio music;std::vector<s16> out;
    check(world.start(config),"movement cues fixture");
    step(0,true);
    check(world.actors()[0].package_movement_cues==9&&music.visit(world,banks,&out)&&out[10]==1000,"takeoff and rising loop");
    step();step(0,true);
    check(world.actors()[0].package_movement_cues==10&&music.visit(world,banks,&out)&&out[0]==2000&&out[10]==1000,
        "accepted airborne jump emits one pulse alongside the current loop");
    check(!music.visit(world,banks,&out),"same tick cannot repeat audio");
    step();step(0,true);step();step(0,true);
    check(world.actors()[0].package_air_points_used==3&&world.actors()[0].package_movement_cues==8,"cap prevents another air-jump cue");
    for(unsigned i=0;i<100&&!(world.actors()[0].package_movement_cues&4u);++i)step();
    check(world.actors()[0].package_movement_cues==4&&music.visit(world,banks,&out)&&out[0]==1000&&out[10]==0,
        "landing plays a one-shot, not the ascent loop");
    check(world.start(config),"replace round");step();
    check(music.visit(world,banks,&out)&&out.empty(),"no old movement voices after restart");
    std::puts("Borg presentation: socket release, shuriken geometry, movement cues and PCM lifetime PASS");
}
