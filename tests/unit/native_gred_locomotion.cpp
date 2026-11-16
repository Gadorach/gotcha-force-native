#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_borg_motion_map.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"G-Red locomotion: %s\n",why);std::abort();}}
struct Wall:gf::IBattleMvpTerrain{bool height(float x,float,float,float* y)const override{*y=x>4?500.f:0.f;return true;}};
struct Ledge:gf::IBattleMvpTerrain{bool height(float x,float,float,float* y)const override{*y=x>4?-200.f:0.f;return true;}};
}
int main(){
    using namespace gf;const auto red=native_borg_motion_map(0x615),sasuke=native_borg_motion_map(10);
    check(red.supported&&red.user_classified&&sasuke.supported&&!sasuke.user_classified,"classifications are G-Red-specific");
    check(!native_borg_motion_map(0x70b).supported,"unmapped Borg does not inherit a universal map");
    for(auto choice:red.clips)check(choice.group==0&&choice.choice!=14,"no conditional melee or uncertain crouch clips in G-Red locomotion");
    check(red.clips[unsigned(NativeBorgMotion::Walk)]==BattleMvpMotionChoice{0,1},"classified walk");
    check(red.clips[unsigned(NativeBorgMotion::Takeoff)]==BattleMvpMotionChoice{0,6}&&red.clips[unsigned(NativeBorgMotion::Fall)]==BattleMvpMotionChoice{0,24},"classified air phases");
    check(red.clips[unsigned(NativeBorgMotion::Land)]==BattleMvpMotionChoice{0,9}&&red.clips[unsigned(NativeBorgMotion::AirDash)]==BattleMvpMotionChoice{0,23},"classified landing and air dash");
    for(auto duration:{1u,6u,12u}){
        check(native_borg_phase_time(duration,duration,20)==0,"phase starts at the first source frame");
        if(duration>1)check(native_borg_phase_time(1,duration,20)==20,"phase reaches the last source frame before exiting");
        check(native_borg_phase_time(duration+1,duration,20)==0&&native_borg_phase_time(0,duration,20)==20,"phase counters clamp without underflow or extrapolation");
    }
    for(auto motion:{NativeBorgMotion::Takeoff,NativeBorgMotion::Apex,NativeBorgMotion::Land})
        check(native_borg_motion_blend_ticks(motion,true)==2&&native_borg_motion_blend_ticks(motion,false)==6,"short-phase blends are G-Red-specific, not a Sasuke retune");
    BattleMvpConfig config;config.spawn={BattlePoint{0,0},BattlePoint{0,-700}};
    const std::array<BattlePoint,4> inputs{BattlePoint{0,-1},BattlePoint{0,1},BattlePoint{-1,0},BattlePoint{1,0}};
    const std::array<NativeBorgMotion,4> clips{NativeBorgMotion::DashForward,NativeBorgMotion::DashBackward,NativeBorgMotion::DashLeft,NativeBorgMotion::DashRight};
    NativeBattleMvp world;std::array<GamePadState,2> pads{};
    for(unsigned direction=0;direction<4;++direction){
        check(world.start(config),"directional dash spawn");const auto body=world.actors()[0].facing;pads={};pads[0].l=true;pads[0].stick_x=s32(inputs[direction].x*127);pads[0].stick_y=s32(inputs[direction].z*127);
        check(world.step(pads,false),"dash starts");
        const auto& actor=world.actors()[0];
        check(actor.action==BattleMvpAction::GroundDash&&actor.dash_ticks==config.dash_duration&&!actor.dash_airborne,"bounded ground burst");
        check(actor.facing==body&&std::abs(actor.movement_speed-config.dash_speed)<.001f,"ground strafe preserves body heading");
        check(native_borg_select_motion(actor,red)==clips[direction],"correct classified directional dash");
        for(unsigned tick=0;tick<100;++tick)check(world.step(pads,false),"held dash tick");
        check(world.actors()[0].dash_ticks==0,"held button does not retrigger");
    }
    check(world.start(config),"camera crossing spawn");pads={};pads[0].stick_x=127;pads[0].l=true;
    NativeBorgMovementFrame before,after;check(native_borg_movement_frame({0,-1},&before)&&native_borg_movement_frame({0,1},&after),"opponent-crossing camera bases");
    check(world.step(pads,false,nullptr,nullptr,&before),"initial camera-right dash");const auto facing=world.actors()[0].facing;
    check(world.step(pads,false,nullptr,nullptr,&after)&&world.actors()[0].velocity.x<0&&world.actors()[0].facing==facing,"held right remains camera-right without rotating ground-dash body");
    pads={};for(unsigned tick=0;tick<30;++tick)check(world.step(pads,false),"dash recovery tick");
    check(world.actors()[0].facing==facing&&world.actors()[0].movement_speed==0,"released strafe settles without spinning the idle body");
    check(world.start(config),"jump sequence spawn");pads={};pads[0].b=true;check(world.step(pads,false),"jump starts");
    std::array<bool,unsigned(NativeBorgMotion::Count)> seen{};
    for(unsigned tick=0;tick<100;++tick){seen[unsigned(native_borg_select_motion(world.actors()[0],red))]=true;pads={};check(world.step(pads,false),"jump phase tick");}
    for(auto phase:{NativeBorgMotion::Takeoff,NativeBorgMotion::AirHold,NativeBorgMotion::Apex,NativeBorgMotion::Fall,NativeBorgMotion::Land})check(seen[unsigned(phase)],"each classified jump/landing phase runs");
    check(world.start(config),"air-dash spawn");pads={};pads[0].b=true;check(world.step(pads,false),"take off before air dash");pads={};
    for(unsigned tick=0;tick<18;++tick)check(world.step(pads,false),"rise");
    pads[0].l=true;pads[0].stick_x=127;check(world.step(pads,false),"air dash starts");
    const f32 height=world.actors()[0].position[1];
    check(world.actors()[0].action==BattleMvpAction::AirDash&&world.actors()[0].air_dashes_used==1&&native_borg_select_motion(world.actors()[0],red)==NativeBorgMotion::AirDash,"single forward-pose air dash");
    check(world.actors()[0].facing.x>.99f&&world.actors()[0].vertical_speed==0,"air dash faces travel direction and arrests upward momentum");
    pads={};for(unsigned tick=0;tick<config.dash_duration-1;++tick)check(world.step(pads,false)&&world.actors()[0].position[1]==height,"air burst holds height");
    while(world.actors()[0].dash_cooldown)check(world.step(pads,false),"cooldown tick");
    check(!world.actors()[0].grounded,"still airborne after cooldown");
    pads[0].l=true;check(world.step(pads,false)&&world.actors()[0].dash_ticks==0&&world.actors()[0].air_dashes_used==1,"no second air dash before landing");
    pads={};for(unsigned tick=0;tick<80;++tick)check(world.step(pads,false),"fall/land tick");
    check(world.actors()[0].grounded&&world.actors()[0].air_dashes_used==0,"landing replenishes provisional air-dash allowance");
    check(world.start(config),"analog dash spawn");pads={};pads[0].trigger_l_pressed=true;
    check(world.step(pads,false)&&world.actors()[0].dash_ticks>0,"digital L and analog trigger use the shared input seam");
    Wall wall;check(world.start(config,&wall),"wall spawn");pads={};pads[0].l=true;pads[0].stick_x=127;
    check(world.step(pads,false,&wall)&&world.actors()[0].position[0]==0&&world.actors()[0].dash_ticks==0,"high terrain step blocks and ends dash");
    check(world.start(config),"dash-to-jump spawn");pads={};pads[0].l=true;pads[0].stick_x=127;
    check(world.step(pads,false)&&world.actors()[0].dash_ticks>0,"ground burst before jump cancel");
    pads={};pads[0].b=true;check(world.step(pads,false),"jump interrupts ground dash");
    check(!world.actors()[0].grounded&&world.actors()[0].dash_ticks==0&&world.actors()[0].movement_speed<=config.move_speed,"jump cancel cannot carry ground-dash speed through the flight");
    Ledge ledge;check(world.start(config,&ledge),"ledge spawn");pads={};pads[0].l=true;pads[0].stick_x=127;
    const auto ledge_facing=world.actors()[0].facing;
    check(world.step(pads,false,&ledge),"dash over ledge");
    check(!world.actors()[0].grounded&&world.actors()[0].dash_ticks==0&&world.actors()[0].action==BattleMvpAction::Jump,"leaving a ledge cancels ground burst and its ground pose");
    check(world.actors()[0].facing==ledge_facing&&world.actors()[0].air_dashes_used==0,"ledge interruption neither twists the body nor consumes an air dash");
    pads={};check(world.step(pads,false,&ledge)&&world.actors()[0].movement_speed<=config.move_speed,"ledge fall resumes bounded air-control speed");
    BattleMvpConfig bad=config;bad.dash_duration=0;check(!world.start(bad),"invalid dash tuning refuses");
    BattleMvpActor unclassified;unclassified.grounded=false;unclassified.takeoff_ticks=5;
    check(native_borg_select_motion(unclassified,sasuke)==NativeBorgMotion::LegacyJump,"Sasuke does not borrow G-Red jump phases");
    std::printf("classified G-Red walk/directional dash/jump/landing/air-dash controller: %u checks PASS\n",checks);
}
