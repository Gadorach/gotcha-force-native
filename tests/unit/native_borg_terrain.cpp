#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_borg_motion_map.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"Borg terrain movement: %s\n",why);std::abort();}}
struct Terrain:gf::IBattleMvpTerrain {
    bool z_wall=false,corner=false,missing_slide=false;
    float wall_height=500;
    mutable unsigned queries=0;
    bool height(float x,float z,float,float* y)const override{
        ++queries;
        if(missing_slide&&x<=4&&z>0)return false;
        *y=(corner?(x>4||z>4):z_wall?z>4:x>4)?wall_height:0.f;
        return true;
    }
};
}
int main(){
    using namespace gf;
    BattleMvpConfig config;config.spawn={BattlePoint{3.8f,0},BattlePoint{-700,-700}};
    NativeBattleMvp world;std::array<GamePadState,2> pads{};
    Terrain terrain;
    check(world.start(config,&terrain),"spawn beside X wall");
    const auto start=world.actors()[0].position;
    pads[0].stick_x=127;pads[0].stick_y=127;terrain.queries=0;
    check(world.step(pads,false,&terrain),"diagonal movement into wall");
    const auto& sliding=world.actors()[0];
    check(sliding.position[0]==start[0]&&sliding.position[2]>start[2],"blocked X preserves free Z movement");
    check(sliding.position[1]==0&&sliding.grounded,"slide retains sampled ground");
    check(sliding.velocity.x==0&&sliding.velocity.z>0&&sliding.travel_distance==sliding.movement_speed,"animation follows actual clipped motion");
    check(terrain.queries<=7,"terrain query count is bounded per tick");
    for(unsigned i=0;i<20;++i)check(world.step(pads,false,&terrain)&&world.actors()[0].position[0]==start[0],"sustained slide never enters wall");
    check(world.actors()[0].position[2]>20,"holding diagonally keeps moving along wall");
    pads={};for(unsigned i=0;i<10;++i)check(world.step(pads,false,&terrain),"brake after slide");
    check(world.actors()[0].movement_speed==0,"slide settles to idle after release");
    check(world.start(config,&terrain),"head-on wall spawn");pads={};pads[0].stick_x=127;
    check(world.step(pads,false,&terrain)&&world.actors()[0].position==start,"head-on obstruction remains blocked");
    check(world.start(config,&terrain),"dash wall spawn");pads={};pads[0].l=true;pads[0].stick_x=127;pads[0].stick_y=127;
    const auto heading=world.actors()[0].facing;
    check(world.step(pads,false,&terrain),"diagonal dash wall contact");
    check(world.actors()[0].position[0]==start[0]&&world.actors()[0].position[2]>start[2],"dash contact retains free-axis displacement");
    check(world.actors()[0].dash_ticks==0&&world.actors()[0].facing==heading,"wall slide interrupts burst without rotating body");
    terrain.z_wall=true;config.spawn[0]={0,3.8f};
    check(world.start(config,&terrain),"spawn beside Z wall");pads={};pads[0].stick_x=127;pads[0].stick_y=127;
    check(world.step(pads,false,&terrain)&&world.actors()[0].position[0]>0&&world.actors()[0].position[2]==3.8f,"blocked Z preserves free X movement");
    terrain.corner=true;config.spawn[0]={3.8f,3.8f};
    check(world.start(config,&terrain),"corner spawn");const auto corner=world.actors()[0].position;
    check(world.step(pads,false,&terrain)&&world.actors()[0].position==corner,"two blocked axes cannot leak through a corner");
    terrain.corner=false;terrain.z_wall=false;config.spawn[0]={3.8f,0};
    terrain.wall_height=30;
    check(world.start(config,&terrain),"walkable step spawn");
    check(world.step(pads,false,&terrain)&&world.actors()[0].position[0]>4&&world.actors()[0].position[2]>0&&world.actors()[0].position[1]==30,"walkable step uses original diagonal rather than a slide");
    terrain.wall_height=500;
    //Place the opponent in the free slide path. The fallback must sweep again,
    //rather than reuse the primary diagonal's actor-contact fraction.
    config.spawn[0]={3.99f,0};config.spawn[1]={3.99f,84.1f};
    check(world.start(config,&terrain),"opponent beside wall");pads[0].l=true;
    check(world.step(pads,false,&terrain),"slide toward opponent");
    const auto& actor=world.actors()[0];const auto& other=world.actors()[1];
    check(std::hypot(actor.position[0]-other.position[0],actor.position[2]-other.position[2])>=2*config.actor_radius,"wall slide cannot bypass actor collision");
    check(actor.position[0]==config.spawn[0].x&&actor.position[2]>0&&actor.position[2]<.101f,"re-swept free-axis slide stops at actor contact");
    config.spawn[0]={3.8f,0};config.spawn[1]={-700,-700};check(world.start(config,&terrain),"missing terrain regression spawn");terrain.missing_slide=true;
    check(!world.step(pads,false,&terrain)&&!world.ready(),"missing fallback terrain refuses instead of inventing a floor");
    terrain.missing_slide=false;terrain.queries=0;
    check(world.start(config,&terrain),"slide determinism spawn");
    NativeBattleMvp repeated=world;
    for(unsigned i=0;i<90;++i){
        pads[0].l=i%31==0;pads[0].b=i==30;
        check(world.step(pads,false,&terrain)&&repeated.step(pads,false,&terrain),"deterministic wall/jump/dash tick");
        const auto& a=world.actors()[0];const auto& b=repeated.actors()[0];
        check(a.position==b.position&&a.facing==b.facing&&a.travel_distance==b.travel_distance&&a.dash_ticks==b.dash_ticks,"identical fixed inputs give identical wall response");
        check(a.position[0]<=4,"jump/dash sequence never crosses the high wall");
    }
    std::printf("bounded native Borg wall slides/corners/actor contacts/refusals: %u checks PASS\n",checks);
}
