#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"Borg movement: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;NativeBorgMovementFrame frame;
    check(native_borg_movement_frame({1,0},&frame)&&native_borg_movement_frame_valid(frame),"camera-facing movement basis");
    auto forward=native_borg_move_direction(0,-1,frame),right=native_borg_move_direction(1,0,frame);
    check(forward.x==1&&forward.z==0&&right.x==0&&right.z==1,"forward/right follow camera, not fixed world axes");
    const auto diagonal=native_borg_move_direction(1,-1,frame);check(std::abs(std::hypot(diagonal.x,diagonal.z)-1)<1e-5f,"diagonal input does not increase speed");
    auto v=native_borg_velocity({0,0},{5,0},.9f);check(std::abs(v.x-.9f)<1e-6f&&v.z==0,"bounded start acceleration");
    v=native_borg_velocity({5,0},{0,0},1.4f);check(std::abs(v.x-3.6f)<1e-5f,"bounded braking");
    v=native_borg_velocity({5,0},{-5,0},.25f);check(v.x>4.7f,"air steering does not reverse velocity instantly");
    auto facing=native_borg_turn({0,1},{1,0},.1f);check(std::abs(std::atan2(facing.x,facing.z)-.1f)<1e-5f,"turn rate is bounded");
    const f32 a=3.12413936f,b=-3.12413936f;
    facing=native_borg_turn({std::sin(a),std::cos(a)},{std::sin(b),std::cos(b)},.02f);
    check(std::abs(std::remainder(std::atan2(facing.x,facing.z)-a,6.28318530718f))<.021f,"turning crosses angle wrap by short path");
    NativeBattleMvp duel;std::array<GamePadState,2> pads{};
    check(duel.start(),"controller actor spawn");const auto spawn=duel.actors()[0].position;pads[0].stick_y=-128;
    check(duel.step(pads,false,nullptr,nullptr,&frame)&&duel.actors()[0].position[0]>spawn[0]&&duel.actors()[0].position[2]==spawn[2],"native actor uses supplied camera basis");
    check(duel.start(),"reset acceleration test");pads={};pads[0].stick_x=127;
    check(duel.step(pads,false)&&duel.actors()[0].movement_speed>0&&duel.actors()[0].movement_speed<duel.config().move_speed,"first movement tick accelerates");
    for(unsigned i=0;i<20;++i)check(duel.step(pads,false),"run acceleration tick");
    check(std::abs(duel.actors()[0].movement_speed-duel.config().move_speed)<.001f,"sustained input reaches run speed");
    const auto distance=duel.actors()[0].travel_distance;pads={};
    check(duel.step(pads,false)&&duel.actors()[0].movement_speed>0&&duel.actors()[0].movement_speed<duel.config().move_speed,"release brakes rather than teleports to rest");
    check(duel.actors()[0].travel_distance>distance,"animation distance tracks actual braking motion");
    for(unsigned i=0;i<8;++i)check(duel.step(pads,false),"braking tick");
    check(duel.actors()[0].movement_speed==0&&duel.actors()[0].action==BattleMvpAction::Idle,"braking settles at a stable idle");
    const auto body=duel.actors()[0].facing;pads[0].a=true;
    check(duel.step(pads,false)&&duel.actors()[0].facing==body&&std::abs(duel.actors()[0].upper_body_yaw)>.2f,"weapon aim turns upper body without forcing body heading");
    bool aimed=false;for(const auto& shot:duel.projectiles())if(shot.active&&shot.owner==0)aimed|=shot.velocity[2]<0;
    check(aimed,"projectile direction follows target aim rather than locomotion facing");
    pads={};for(unsigned i=0;i<40;++i)check(duel.step(pads,false),"attack aim release tick");
    check(duel.actors()[0].facing==body&&std::abs(duel.actors()[0].upper_body_yaw)<1e-5f,"upper body releases target aim while idle body retains its heading");
    pads[0].b=true;check(duel.step(pads,false)&&!duel.actors()[0].grounded,"jump controller starts");
    bool landed=false;
    for(unsigned i=0;i<100;++i){pads={};check(duel.step(pads,false),"jump/landing tick");if(duel.actors()[0].landing_ticks){landed=true;check(duel.actors()[0].grounded&&duel.actors()[0].action==BattleMvpAction::Land,"landing has its own transition");}}
    check(landed&&duel.actors()[0].landing_ticks==0&&duel.actors()[0].action==BattleMvpAction::Idle,"landing transitions back to idle");
    auto bad=frame;bad.right=bad.forward;const auto before=duel.tick();
    check(!duel.step({},false,nullptr,nullptr,&bad)&&duel.tick()==before,"invalid camera basis does not mutate controller");
    check(!native_borg_movement_frame({0,0},&frame)&&!native_borg_movement_frame({std::numeric_limits<f32>::quiet_NaN(),0},&frame),"degenerate/nonfinite basis refuses");
    std::printf("native Borg camera-relative movement/acceleration/braking/turning/jump/landing: %u checks PASS\n",checks);
}
