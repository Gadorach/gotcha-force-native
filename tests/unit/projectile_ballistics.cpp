#include "engine/gameplay/native_battle_mvp.hpp"
#include <cstdio>
#include <cstdlib>

namespace {
using namespace gf;

void check(bool ok,const char* message){
    if(!ok){std::fprintf(stderr,"projectile ballistics: %s\n",message);std::abort();}
}
bool near(f32 a,f32 b,f32 epsilon=.001f){return std::abs(a-b)<=epsilon;}
bool near(const BattleMvpVec& a,const BattleMvpVec& b){
    return near(a[0],b[0])&&near(a[1],b[1])&&near(a[2],b[2]);
}
std::shared_ptr<const BorgGameplayProfile> parse(std::string_view text){
    BorgGameplayProfile profile;std::string error;
    if(!borg_read_gameplay_profile(text,&profile,&error)){std::fputs(error.c_str(),stderr);std::abort();}
    return std::make_shared<const BorgGameplayProfile>(std::move(profile));
}

constexpr BattleMvpVec launch{165,200,300},point{465,200,700};
constexpr std::string_view standing=R"(GFBORG-GAMEPLAY 1
variant=normal
health=180
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
)";
const std::string mobile=std::string(standing)+R"(movement=grounded-v1|8|13|14
walk=0:1
air=0:2
dash=0:3
)";
const std::string shooter=mobile+R"(stock=bombs|1|300
projectile=bomb|orb|1|40|0|0.3|0|8|60|12|ignore-floor
origin=bomb|11|0|0|0|fixed
move=toss|b|10|any|6|0:4
release=toss|1|bombs|1|bomb
)";
const std::string arc="ballistic-aim=bomb|0.8\n";
const std::string refit="ballistic-gravity-refit=bomb|4|0|7.5|0.15\n";

struct Muzzle final:IBorgBattleOrigins {
    bool socket(unsigned,const BattleMvpActor&,u32,u8,u16,const BorgBattleSocket&,BattleMvpVec* out)const override{
        *out=launch;return true;
    }
};
struct Terrace final:IBattleMvpTerrain {
    bool height(f32,f32 z,f32,f32* out)const override{*out=z>=650?140.f:0.f;return true;}
};

void numeric_cases(){
    const auto aimed=projectile_lower_arc(launch,point,40,.3f,.8f);
    check(aimed&&near(*aimed,{23.97356f,1.87707f,31.96474f}),"level diagonal lower arc and speed");
    const auto t=500./std::hypot((*aimed)[0],(*aimed)[2]);
    check(std::abs((*aimed)[1]*t-.15*t*t)<.001,"uncapped lower arc reaches the sampled height");
    const auto high=projectile_lower_arc(launch,{465,9000,700},40,.3f,.8f);
    check(high&&near(std::atan2((*high)[1],std::hypot((*high)[0],(*high)[2])),.8f),"unreachable point uses a bounded tangent arc");
    const auto low=projectile_lower_arc(launch,{-135,-30,700},40,.3f,.8f);
    check(low&&(*low)[0]<0&&(*low)[1]<0&&near(std::hypot((*low)[0],(*low)[1],(*low)[2]),40),
          "downward arc preserves total speed");
    check(!projectile_lower_arc(launch,{165,220,300},40,.3f,.8f)&&
          !projectile_lower_arc(launch,point,40,0,.8f),"zero horizontal separation and zero gravity refuse aiming");
    const auto clamp=projectile_gravity_for_height({0,0,0},{0,1,40},{0,65,400},0,7.5f);
    check(clamp&&*clamp==0&&!projectile_gravity_for_height({0,0,0},{0,1,40},{0,0,-40},0,7.5f),
          "refit clamps gravity and cannot turn a shot back toward an overshot point");
}

void parser_cases(){
    const auto rules=parse(shooter+arc+refit);
    const auto& scoped=*rules->projectiles[0].ballistic_aim;
    check(scoped.pitch_cap==.8f&&scoped.refit&&scoped.refit->interval==4&&scoped.refit->step==.15f,
          "profile reads both opt-ins");
    BorgGameplayProfile refused;
    const auto reject=[&](const std::string& text){return !borg_read_gameplay_profile(text,&refused,nullptr);};
    check(reject(shooter+refit)&&reject(shooter+arc+arc)&&reject(shooter+arc+refit+refit)&&
          reject(shooter+"ballistic-aim=missing|0.8\n")&&reject(shooter+"ballistic-aim=bomb|1.6\n")&&
          reject(shooter+arc+"ballistic-gravity-refit=bomb|4|1|7.5|0.15\n"),
          "missing, repeated or out-of-domain aim operands refuse");
    check(reject(shooter+arc+"guidance=bomb|tracking|0.1|20\n")&&
          reject(shooter+arc+"launch-heading=bomb|body\n")&&
          reject(shooter+arc+"projectile-speed-ramp=bomb|1|60\n"),
          "competing launch and trajectory rules refuse");
    auto programmatic=*rules;
    programmatic.projectiles[0].socket->aim_height=true;
    check(!borg_gameplay_profile_valid(programmatic),"ballistic aiming forbids a second socket height policy");
}

void live_gate_cases(){
    const auto thrower=parse(shooter+arc+refit);
    const auto weak=parse(std::string(standing)+"movement=stationary-v1\n");
    const auto ally=parse(mobile+"move=finish|x|10|any|6|0:0\nmelee=finish|1|1|80|40|60|32|180\n");
    Terrace ground;Muzzle muzzle;
    for(bool replacement:{false,true}){
        NativeBattleMvp world;BattleMvpConfig config;
        config.actor_height=120;config.actor_radius=22;
        config.declared_actors=replacement?4:3;config.teams_declared=true;config.teams={0,1,0,1};
        config.packages={thrower,weak,ally,replacement?weak:nullptr};
        config.spawn={BattlePoint{0,0},BattlePoint{465,700},BattlePoint{565,700},BattlePoint{-900,800}};
        check(world.start(config,&ground)&&world.target_of(0)==1,"release fixture starts with one nearer sampled target");
        std::array<GamePadState,kBattleMvpMaxActors> pads{};
        std::array<BorgBattleInputs,kBattleMvpMaxActors> inputs{};
        for(unsigned tick=0;tick<6&&!world.shots_spawned();++tick){
            inputs[0].pressed=tick?0:BorgBattleInputs::b;
            inputs[2].pressed=tick?0:BorgBattleInputs::x;
            check(world.step_roster(pads,inputs,false,&ground,{},nullptr,nullptr,&muzzle),"launch/KO ordering step");
        }
        const auto& a=world.actors()[0];const auto& shot=world.projectiles()[0];
        check(world.shots_spawned()==1&&shot.active&&!world.actors()[1].health&&
              !world.package_action(0).stocks[0].current,"target dies between paid emission and the live launch visit");
        const BattleMvpVec expected=replacement?BattleMvpVec{23.97356f,1.87707f,31.96474f}:
            BattleMvpVec{40*a.facing.x,0,40*a.facing.z};
        check(near(shot.velocity,{expected[0],expected[1]-.3f,expected[2]})&&
              near(shot.position,{launch[0]+expected[0],launch[1]+expected[1]-.15f,launch[2]+expected[2]})&&
              !shot.ballistic.launch_pending&&!shot.ballistic.launch_point&&shot.ballistic.tracked_slot==255,
              "live gate can close, or stay open without replacing the captured point or tracker");
    }
}

void refit_cases(){
    Terrace ground;Muzzle muzzle;
    const auto idle=parse(std::string(standing)+"movement=stationary-v1\n");
    const auto moving=parse(mobile+"move=swipe|x|10|any|6|0:0\nmelee=swipe|1|1|90|30|60|32|1\n");
    for(bool enabled:{false,true}){
        NativeBattleMvp world;BattleMvpConfig config;
        config.actor_height=120;config.actor_radius=22;config.declared_actors=3;
        config.teams_declared=true;config.teams={0,1,1};config.local_aim_cycle=true;
        config.packages={parse(shooter+arc+(enabled?refit:"")),moving,idle};
        config.spawn={BattlePoint{0,0},BattlePoint{465,700},BattlePoint{-600,1000}};
        check(world.start(config,&ground)&&world.aim_target_of(0)==1,"refit fixture has an original and new lock target");
        unsigned sampled=0;
        std::array<GamePadState,kBattleMvpMaxActors> pads{};
        std::array<BorgBattleInputs,kBattleMvpMaxActors> inputs{};
        for(unsigned tick=0;tick<12&&sampled<6;++tick){
            inputs[0].pressed=tick?0:BorgBattleInputs::b;
            pads[1].b=sampled==1; // Raise the retained target after release.
            BattleMvpLocalRosterInput local;
            if(sampled==1)local.cycle_aim=1;
            check(world.step_roster(pads,inputs,false,&ground,{},nullptr,nullptr,&muzzle,{},local),"ballistic flight step");
            const auto& shot=world.projectiles()[0];
            if(!shot.active)continue;
            ++sampled;
            const f32 expected_gravity=enabled&&sampled>=4?.15f:.3f;
            check(near(shot.package_gravity,expected_gravity)&&near(shot.velocity[0],23.97356f)&&
                  near(shot.velocity[2],31.96474f)&&shot.ballistic.tracked_slot==(enabled?1:255),
                  "fourth active visit refits the original target's height and never steers yaw");
            if(sampled>1)check(world.aim_target_of(0)==2&&world.actors()[1].position[1]>140,
                                "the currently locked actor is different from the jumping tracker");
        }
        check(sampled==6&&world.shots_spawned()==1&&!world.hits(),"flight refit does not spend stock or manufacture a contact");
    }
}

void deflection_case(){
    auto text=shooter+arc+refit;
    constexpr std::string_view socket="origin=bomb|11|0|0|0|fixed\n";
    text.erase(text.find(socket),socket.size());
    const auto field=parse(mobile+"move=well|x|80|any|8|0:4\n"
        "field=well|1|box|170|170|170|5|500|100|95|50|160|80\nfield-shot-pull=well|enemies|10\n");
    NativeBattleMvp world;BattleMvpConfig config;
    config.packages={field,parse(text)};
    config.spawn={BattlePoint{0,0},BattlePoint{0,600}};
    check(world.start(config),"pull fixture starts");
    unsigned after=0;
    bool pulled=false;
    for(unsigned tick=0;tick<16&&after<5;++tick){
        const auto field_input=BorgBattleInputs{u8(tick?0:BorgBattleInputs::x),0,0};
        const auto throw_input=BorgBattleInputs{u8(tick?0:BorgBattleInputs::b),0,0};
        check(world.step({},false,nullptr,nullptr,nullptr,nullptr,{field_input,throw_input}),"pull fixture step");
        for(const auto& e:world.package_events())if(e.kind==BorgBattleEventKind::FieldShotPulled) pulled=true;
        const auto& shot=world.projectiles()[0];
        if(pulled&&shot.active){
            ++after;
            check(shot.ballistic.tracked_slot==255&&near(shot.package_gravity,.3f),
                  "field deflection cancels the refit instead of competing with the new velocity");
        }
    }
    check(pulled&&after==5&&world.shots_spawned()==1,"observed several active visits after a pull, beyond the refit cadence");
}
}

int main(){
    numeric_cases();parser_cases();live_gate_cases();refit_cases();deflection_case();
    std::puts("Projectile ballistics: bounded launch, live gating and refit PASS");
}
