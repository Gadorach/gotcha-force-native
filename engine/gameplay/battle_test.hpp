#pragma once
// Native DIAGNOSTIC battle environment, not recovered Gotcha Force combat.
// Every tuning value is an owned test fixture. No assets, OS, saves or audio.
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include "engine/input/input_map.hpp"
#include "engine/gameplay/battle_swept_circle.hpp"
namespace gf {
struct BattleTestConfig {
    unsigned tick_rate=60,shot_interval=12,projectile_life=120,target_health=3,ally_mask=8;
    f32 arena_half=15,actor_radius=0.6f,target_radius=0.8f,projectile_radius=0.15f,move_per_tick=0.12f,shot_per_tick=0.9f;
    f32 camera_distance=6,camera_height=3.5f,camera_fov=55; // diagnostic offsets, not recovered camera tuning
    BattlePoint spawn{0,6};std::array<BattlePoint,4> targets={BattlePoint{0,-6},BattlePoint{-4,-2},BattlePoint{5,-4},BattlePoint{-5,4}};
    friend bool operator==(const BattleTestConfig&,const BattleTestConfig&)=default;
};
inline bool battle_test_config_valid(const BattleTestConfig& c){
    auto positive=[](f32 v){return std::isfinite(v)&&v>0;};
    if(c.tick_rate<10||c.tick_rate>240||!c.shot_interval||c.shot_interval>10000||!c.projectile_life||c.projectile_life>10000||
       !c.target_health||c.target_health>10000||!positive(c.arena_half)||c.arena_half<4||c.arena_half>10000||
       !positive(c.actor_radius)||c.actor_radius>c.arena_half/4||!positive(c.target_radius)||c.target_radius>c.arena_half/4||
       !positive(c.projectile_radius)||c.projectile_radius>c.actor_radius||!positive(c.move_per_tick)||c.move_per_tick>c.actor_radius||
       !positive(c.shot_per_tick)||c.shot_per_tick>c.arena_half*16||!positive(c.camera_distance)||c.camera_distance<c.actor_radius*4||
       c.camera_distance>c.arena_half*4||!positive(c.camera_height)||c.camera_height>c.camera_distance||
       !std::isfinite(c.camera_fov)||c.camera_fov<45||c.camera_fov>90||c.ally_mask>=(1u<<c.targets.size()))return false;
    auto inside=[&](BattlePoint p,f32 r){return std::isfinite(p.x)&&std::isfinite(p.z)&&std::abs(p.x)<=c.arena_half-r&&std::abs(p.z)<=c.arena_half-r;};
    if(!inside(c.spawn,c.actor_radius))return false;
    for(unsigned i=0;i<c.targets.size();++i){const auto p=c.targets[i];if(!inside(p,c.target_radius))return false;
        const f64 dx=f64(p.x)-c.spawn.x,dz=f64(p.z)-c.spawn.z,r=f64(c.actor_radius)+c.target_radius;
        if(dx*dx+dz*dz<=r*r)return false;
        for(unsigned j=0;j<i;++j){const f64 x=f64(p.x)-c.targets[j].x,z=f64(p.z)-c.targets[j].z;
            if(x*x+z*z<=4*f64(c.target_radius)*c.target_radius)return false;}}
    return true;
}
class BattleTest {
public:
    static constexpr unsigned kMaxProjectiles=32;
    static constexpr unsigned kNoTarget=4;
    struct Target{BattlePoint position;unsigned health=0;bool ally=false;friend bool operator==(const Target&,const Target&)=default;};
    struct Projectile{bool active=false;BattlePoint position,velocity;unsigned life=0;friend bool operator==(const Projectile&,const Projectile&)=default;};
    friend bool operator==(const BattleTest&,const BattleTest&)=default;
    bool configure(const BattleTestConfig& config){
        if(!battle_test_config_valid(config))return false;
        config_=config;configured_=true;tick_=shots_=hits_=destroyed_=resets_=enemy_down_=ally_down_=0;prev_={};paused_=false;reset_round();return true;
    }
    bool step(const GamePadState& pad,const BattlePoint* pointer_aim=nullptr){
        if(!configured_||tick_==std::numeric_limits<u32>::max())return false;
        if(pointer_aim&&(!std::isfinite(pointer_aim->x)||!std::isfinite(pointer_aim->z)||std::abs(pointer_aim->x)>config_.arena_half||std::abs(pointer_aim->z)>config_.arena_half))return false;
        const bool pause=pad.start&&!prev_.start,reset=pad.x&&!prev_.x,lock=pad.z&&!prev_.z,cycle=pad.y&&!prev_.y,team=pad.r&&!prev_.r;prev_=pad;
        if(pause){paused_=!paused_;fire_armed_=false;return true;}
        if(reset){reset_round();fire_armed_=!pad.a;++resets_;return true;}
        if(paused_)return true;
        ++tick_;if(!pad.a)fire_armed_=true;
        if(lock){locked_=!locked_;selected_=locked_?nearest():kNoTarget;}
        if(team){target_allies_=!target_allies_;locked_=true;selected_=nearest();}
        if(locked_&&(selected_>=targets_.size()||!targets_[selected_].health))selected_=nearest();
        if(cycle){if(!locked_){locked_=true;selected_=nearest();}
            else for(unsigned next=1;next<=targets_.size();++next){const auto i=(selected_+next)%targets_.size();if(targets_[i].health&&targets_[i].ally==target_allies_){selected_=unsigned(i);break;}}}
        f32 x=axis(pad.stick_x),z=axis(pad.stick_y);
        if(pad.dpad_left)x-=1;
        if(pad.dpad_right)x+=1;
        if(pad.dpad_up)z-=1;
        if(pad.dpad_down)z+=1;
        normalize(x,z,false);BattlePoint delta{x*config_.move_per_tick,z*config_.move_per_tick};
        const f32 bound=config_.arena_half-config_.actor_radius;
        delta.x=std::clamp(player_.x+delta.x,-bound,bound)-player_.x;delta.z=std::clamp(player_.z+delta.z,-bound,bound)-player_.z;
        f32 portion=1;
        for(const auto& target:targets_)if(target.health){f32 t=0;const f32 radius=config_.actor_radius+config_.target_radius;
            const f64 rx=f64(player_.x)-target.position.x,rz=f64(player_.z)-target.position.z;
            if(rx*delta.x+rz*delta.z>=0)continue; // allow separating/tangential motion from contact
            if(battle_swept_hit(player_,delta,target.position,radius,&t))portion=std::min(portion,std::max(0.0f,t-1e-5f));}
        player_.x+=delta.x*portion;player_.z+=delta.z*portion;
        f32 aim_x=axis(pad.cstick_x),aim_z=axis(pad.cstick_y);
        if(selected_target()<targets_.size()){aim_x=targets_[selected_].position.x-player_.x;aim_z=targets_[selected_].position.z-player_.z;}
        else if(pointer_aim){aim_x=pointer_aim->x-player_.x;aim_z=pointer_aim->z-player_.z;}
        else if(aim_x==0&&aim_z==0){aim_x=x;aim_z=z;}
        if(aim_x!=0||aim_z!=0){normalize(aim_x,aim_z,true);facing_={aim_x,aim_z};}
        if(cooldown_)--cooldown_;
        if(pad.a&&fire_armed_&&!cooldown_){for(auto& shot:projectiles_)if(!shot.active){
            shot={true,player_,{facing_.x*config_.shot_per_tick,facing_.z*config_.shot_per_tick},config_.projectile_life};
            cooldown_=config_.shot_interval;++shots_;break;}}
        for(auto& shot:projectiles_)if(shot.active){unsigned hit=unsigned(targets_.size());f32 first=2;
            for(unsigned i=0;i<targets_.size();++i)if(targets_[i].health){f32 t=0;
                if(battle_swept_hit(shot.position,shot.velocity,targets_[i].position,config_.target_radius+config_.projectile_radius,&t)&&t<first){first=t;hit=i;}}
            if(hit<targets_.size()){shot.active=false;++hits_;last_hit_={shot.position.x+shot.velocity.x*first,shot.position.z+shot.velocity.z*first};
                if(--targets_[hit].health==0){++destroyed_;if(targets_[hit].ally)++ally_down_;else ++enemy_down_;}}
            else{shot.position.x+=shot.velocity.x;shot.position.z+=shot.velocity.z;
                if(--shot.life==0||std::abs(shot.position.x)>config_.arena_half||std::abs(shot.position.z)>config_.arena_half)shot.active=false;}}
        update_camera();return true;
    }
    const BattleTestConfig& config()const{return config_;}const auto& targets()const{return targets_;}const auto& projectiles()const{return projectiles_;}
    BattlePoint player()const{return player_;}BattlePoint facing()const{return facing_;}BattlePoint last_hit()const{return last_hit_;}
    bool configured()const{return configured_;}bool paused()const{return paused_;}bool locked()const{return selected_target()!=kNoTarget;}
    unsigned selected_target()const{return locked_&&selected_<targets_.size()&&targets_[selected_].health?selected_:kNoTarget;}
    bool targeting_allies()const{return target_allies_;}
    BattlePoint camera_eye()const{return camera_eye_;}BattlePoint camera_interest()const{return camera_interest_;}
    u32 enemy_down()const{return enemy_down_;}u32 ally_down()const{return ally_down_;}
    u32 tick()const{return tick_;}u32 shots()const{return shots_;}u32 hits()const{return hits_;}u32 destroyed()const{return destroyed_;}u32 resets()const{return resets_;}
private:
    static f32 axis(s32 v){return f32(std::clamp(v,-128,127))/(v<0?128.0f:127.0f);}
    static void normalize(f32& x,f32& z,bool unit){const f32 len=std::sqrt(x*x+z*z);if(len>0&&(unit||len>1)){x/=len;z/=len;}}
    unsigned nearest()const{unsigned best=unsigned(targets_.size());f64 distance=std::numeric_limits<f64>::max();
        for(unsigned i=0;i<targets_.size();++i)if(targets_[i].health&&targets_[i].ally==target_allies_){const f64 x=f64(targets_[i].position.x)-player_.x,z=f64(targets_[i].position.z)-player_.z,d=x*x+z*z;
            if(d<distance){best=i;distance=d;}}return best;}
    void update_camera(){
        if(locked()){camera_interest_=targets_[selected_].position;f32 x=camera_interest_.x-player_.x,z=camera_interest_.z-player_.z;normalize(x,z,true);
            camera_eye_={player_.x-x*config_.camera_distance,player_.z-z*config_.camera_distance};}
        else{ // Geometric native tether: carry previous eye, constrain distance.
            // Character rotation at rest does not rotate this camera. No guessed
            // original spring/smoothing constants are introduced.
            f32 x=camera_eye_.x-player_.x,z=camera_eye_.z-player_.z;
            if(x==0&&z==0){x=-facing_.x;z=-facing_.z;}normalize(x,z,true);
            camera_eye_={player_.x+x*config_.camera_distance,player_.z+z*config_.camera_distance};
            camera_interest_={player_.x-x*config_.camera_distance*2,player_.z-z*config_.camera_distance*2};
        }
    }
    void reset_round(){player_=config_.spawn;facing_={0,-1};last_hit_={};cooldown_=0;locked_=target_allies_=false;selected_=kNoTarget;fire_armed_=true;projectiles_={};
        for(unsigned i=0;i<targets_.size();++i)targets_[i]={config_.targets[i],config_.target_health,(config_.ally_mask&(1u<<i))!=0};
        camera_eye_={player_.x,player_.z+config_.camera_distance};update_camera();}
    BattleTestConfig config_;std::array<Target,4> targets_{};std::array<Projectile,kMaxProjectiles> projectiles_{};
    BattlePoint player_,facing_,last_hit_,camera_eye_,camera_interest_;GamePadState prev_{};u32 tick_=0,shots_=0,hits_=0,destroyed_=0,resets_=0,enemy_down_=0,ally_down_=0;unsigned cooldown_=0,selected_=kNoTarget;
    bool configured_=false,paused_=false,locked_=false,fire_armed_=true,target_allies_=false;
};
} // namespace gf
