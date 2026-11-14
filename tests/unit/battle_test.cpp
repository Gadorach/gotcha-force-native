#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/battle_test_config.hpp"
#include "engine/gameplay/battle_test_view.hpp"
#include "engine/core/headless_backends.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"battle test: %s\n",why);std::abort();}}
bool close(float a,float b){return std::abs(a-b)<0.0001f;}
struct Recorder:gf::IRenderBackend {
    std::vector<gf::DrawCall> calls;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{}void begin_frame(const gf::FrameDesc&)override{calls.clear();}void end_frame()override{}void present()override{}
    bool upload_vertices(const gf::Vertex*,std::size_t)override{return true;}
    bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}
    bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void submit(const gf::DrawCall& call)override{calls.push_back(call);}
};
}
int main(){
    gf::BattleTestConfig cfg;gf::BattleTest world;gf::GamePadState pad;float t=-1;
    check(!world.step(pad)&&gf::battle_test_config_valid(cfg)&&world.configure(cfg),"explicit valid diagnostic configuration required");
    auto bad=cfg;bad.move_per_tick=NAN;check(!world.configure(bad)&&world.configured()&&world.config()==cfg,"invalid configure leaves existing session intact");
    bad=cfg;bad.target_health=0;check(!gf::battle_test_config_valid(bad),"zero health refuses");bad=cfg;bad.targets[0]=cfg.spawn;check(!gf::battle_test_config_valid(bad),"overlapping spawn refuses");
    bad=cfg;bad.targets[1]=bad.targets[0];check(!gf::battle_test_config_valid(bad),"overlapping targets refuse");
    check(gf::battle_swept_hit({0,0},{100,0},{50,0},1,&t)&&close(t,0.49f),"sweep catches high-speed crossing between endpoints");
    check(gf::battle_swept_hit({0,0},{100,0},{50,1},1,&t)&&close(t,0.5f),"tangent sweep");
    check(gf::battle_swept_hit({0,0},{0,0},{0,0},1,&t)&&t==0,"initial overlap counts at start");
    check(!gf::battle_swept_hit({0,0},{0,0},{2,0},1,&t)&&!gf::battle_swept_hit({NAN,0},{1,0},{0,0},1,&t)&&!gf::battle_swept_hit({0,0},{1,0},{0,0},1,nullptr),"stationary miss/nonfinite/missing output refuses");
    pad.stick_x=127;world.step(pad);check(close(world.player().x,cfg.move_per_tick),"full positive movement");
    world.configure(cfg);pad.stick_x=-128;world.step(pad);check(close(world.player().x,-cfg.move_per_tick),"full negative movement has same distance");
    world.configure(cfg);pad.stick_x=pad.stick_y=127;world.step(pad);auto p=world.player();
    check(close(std::hypot(p.x-cfg.spawn.x,p.z-cfg.spawn.z),cfg.move_per_tick),"diagonal movement normalized, not faster");
    world.configure(cfg);pad={};pad.dpad_right=true;for(unsigned i=0;i<1000;++i)world.step(pad);
    check(world.player().x<=cfg.arena_half-cfg.actor_radius&&close(world.player().x,cfg.arena_half-cfg.actor_radius),"arena bounds include actor extent");
    world.configure(cfg);pad={};pad.dpad_up=true;for(unsigned i=0;i<200;++i)world.step(pad);
    check(world.player().z>=cfg.targets[0].z+cfg.actor_radius+cfg.target_radius-0.0001f,"actor sweep stops before solid target, no tunnelling");
    pad.dpad_up=false;pad.dpad_down=true;const auto contact=world.player();world.step(pad);check(world.player().z>contact.z,"separating motion from contact is allowed");
    world.configure(cfg);pad={};pad.z=true;pad.a=true;for(unsigned i=0;i<400;++i)world.step(pad);
    check(!world.locked()&&world.selected_target()==gf::BattleTest::kNoTarget&&world.hits()==9&&world.destroyed()==3,"lock target selection/projectiles/health resolve targets, then retain no dead lock");
    check(world.shots()>0&&world.shots()<=34,"held fire obeys explicit diagnostic cooldown");
    pad={};pad.x=true;pad.a=true;world.step(pad);const auto before=world.shots();world.step(pad);
    check(world.resets()==1&&world.player()==cfg.spawn&&world.targets()[0].health==cfg.target_health&&world.shots()==before,"reset edge restores targets and consumes held fire");
    pad={};world.step(pad);pad.a=true;world.step(pad);check(world.shots()==before+1,"fresh fire after reset release works");
    world.configure(cfg);pad={};pad.start=true;world.step(pad);pad.start=false;pad.a=true;for(unsigned i=0;i<20;++i)world.step(pad);
    check(world.paused()&&world.tick()==0&&world.shots()==0,"pause freezes simulation while consuming controls");
    pad.start=true;world.step(pad);pad.start=false;world.step(pad);check(!world.paused()&&world.shots()==0,"held fire cannot leak through pause/resume");
    pad.a=false;world.step(pad);pad.a=true;world.step(pad);check(world.shots()==1,"fresh post-pause fire");
    auto fast=cfg;fast.shot_per_tick=30;fast.target_health=1;world.configure(fast);pad={};pad.a=true;world.step(pad);
    check(world.hits()==1&&world.destroyed()==1&&world.targets()[0].health==0,"fast bullet hits target before leaving arena in same step");
    auto slow=cfg;slow.shot_interval=1;slow.shot_per_tick=0.0001f;world.configure(slow);for(unsigned i=0;i<100;++i)world.step(pad);
    unsigned active=0;for(const auto& shot:world.projectiles())active+=shot.active;
    check(world.shots()==gf::BattleTest::kMaxProjectiles&&active==gf::BattleTest::kMaxProjectiles,"projectile pool bounded without unbounded allocation");
    Recorder backend;gf::RenderEngine render(backend);std::string error;check(render.initialize({640,480,false,true,"owned battle test"},nullptr,&error),"recording rendering seam");
    gf::BattleTest a,b;a.configure(cfg);b.configure(cfg);gf::BattleTestView view;
    for(unsigned i=0;i<350;++i){pad={};pad.z=i==1;pad.a=i%18==3;pad.stick_x=i<20?75:0;check(a.step(pad)&&b.step(pad),"deterministic fixed diagnostic input");
        if(i%3==0){render.begin_frame({i,0,0,0});render.reset_arena();check(view.draw(render,b,640,480)==2,"proxy arena and explicit diagnostic HUD draw");render.end_frame();}
        check(a==b,"render cadence cannot change complete simulation state");}
    render.state().texture=77;render.state().colour_write=false;render.state().alpha_write=false;const auto saved=render.state();render.reset_arena();view.draw(render,a,640,480);
    check(render.state().texture==saved.texture&&render.state().colour_write==saved.colour_write&&render.state().alpha_write==saved.alpha_write,"battle view restores caller render state");
    check(backend.calls.back().blend&&backend.calls.back().blend_src==5&&backend.calls.back().blend_dst==6&&backend.calls.back().colour_write&&!backend.calls.back().depth_write,"diagnostic HUD uses render-seam alpha factors, not raw GX codes");
    gf::BattleTest follow;follow.configure(cfg);pad={};pad.z=true;follow.step(pad);
    const auto selected=follow.selected_target();check(selected==1&&follow.locked(),"exactly one nearest live target selected on lock edge");
    const auto original_camera=view.camera_for(follow,640,480);auto camera=original_camera;auto player=follow.player();const auto target=follow.targets()[selected].position;
    check(close(camera.target[0],target.x)&&close(camera.target[2],target.z)&&close(std::hypot(camera.eye[0]-player.x,camera.eye[2]-player.z),cfg.camera_distance)&&
        (player.x-camera.eye[0])*(target.x-player.x)+(player.z-camera.eye[2])*(target.z-player.z)>0,"third-person camera sits behind player along selected-enemy viewing direction");
    const float focal[]={target.x,0.8f,target.z};float sx=0,sy=0;
    check(camera.project(focal,&sx,&sy,nullptr)&&close(sx,320)&&close(sy,240),"one selected enemy defines the viewport focus");
    pad={};pad.stick_x=127;for(unsigned i=0;i<60;++i)follow.step(pad);camera=view.camera_for(follow,640,480);player=follow.player();
    check(follow.selected_target()==selected&&camera.eye[0]!=original_camera.eye[0]&&close(std::hypot(camera.eye[0]-player.x,camera.eye[2]-player.z),cfg.camera_distance),"camera follows moving player while target identity remains sticky, not nearest-enemy hopping");
    pad={};pad.y=true;follow.step(pad);const auto next=follow.selected_target();follow.step(pad);
    check(next==2&&follow.selected_target()==next,"target cycle selects one next enemy once; held button doesn't cycle repeatedly");
    pad={};pad.z=true;follow.step(pad);check(!follow.locked()&&follow.selected_target()==gf::BattleTest::kNoTarget,"unlock clears the sole target");
    gf::BattleTest tether;tether.configure(cfg);const auto resting_eye=tether.camera_eye();pad={};pad.cstick_x=127;tether.step(pad);
    check(tether.facing().x==1&&tether.camera_eye()==resting_eye,"free tether does not snap behind character rotation at rest");
    pad={};pad.stick_x=127;tether.step(pad);auto tether_eye=tether.camera_eye();auto tether_player=tether.player();
    check(std::abs(tether_eye.x)<0.01f&&tether_eye.z>resting_eye.z-0.01f&&close(std::hypot(tether_eye.x-tether_player.x,tether_eye.z-tether_player.z),cfg.camera_distance),"free camera carries old eye along movement path and preserves tether distance, not instant facing");
    for(unsigned i=0;i<80;++i){pad={};pad.stick_x=i<40?127:0;pad.stick_y=i>=40?-128:0;tether.step(pad);tether_eye=tether.camera_eye();tether_player=tether.player();
        check(close(std::hypot(tether_eye.x-tether_player.x,tether_eye.z-tether_player.z),cfg.camera_distance),"fixed tether distance through a turn");}
    check(tether.camera_eye().x!=resting_eye.x&&tether.camera_eye().z!=resting_eye.z,"tether follows combined path rather than fixed overview");
    gf::BattleTest teams;teams.configure(cfg);pad={};pad.z=true;teams.step(pad);pad={};pad.r=true;teams.step(pad);
    check(teams.targeting_allies()&&teams.selected_target()==3&&teams.targets()[teams.selected_target()].ally,"team selector replaces enemy with one ally target");
    teams.step(pad);check(teams.targeting_allies()&&teams.selected_target()==3,"held team selector doesn't toggle repeatedly");
    pad={};pad.a=true;for(unsigned i=0;i<400;++i)teams.step(pad);
    check(teams.ally_down()==1&&teams.enemy_down()==0&&teams.hits()==3&&teams.targets()[0].health==cfg.target_health,"diagnostic projectile damage can hit selected ally; no blanket friendly-fire immunity");
    check(!teams.locked(),"destroyed ally is not retained as a live target");
    auto only_allies=cfg;only_allies.ally_mask=15;gf::BattleTest interwave;interwave.configure(only_allies);pad={};pad.z=true;interwave.step(pad);
    check(!interwave.locked()&&interwave.selected_target()==gf::BattleTest::kNoTarget,"no available enemy enters free-camera state even with live allies present");
    pad={};pad.r=true;interwave.step(pad);check(interwave.locked()&&interwave.targeting_allies(),"ally targeting remains possible when enemy set is empty");
    gf::BattleTest mouse_world;mouse_world.configure(cfg);gf::BattlePoint aim;
    check(view.pointer_aim(mouse_world,640,480,320,240,&aim)&&std::abs(aim.x)<0.001f&&!view.pointer_aim(mouse_world,640,480,-1,20,&aim),"mouse ray uses follow camera and refuses outside viewport");
    const gf::BattlePoint bad_aim{NAN,0};const auto copy=a;check(!a.step({},&bad_aim)&&a==copy,"invalid pointer aim has no simulation effects");
    std::ifstream file("content/test-battle/battle.ini");std::string text((std::istreambuf_iterator<char>(file)),{});check(!text.empty(),"owned loose INI fixture exists");
    auto load=[&](const std::string& s){return gf::battle_test_config_load({reinterpret_cast<const gf::u8*>(s.data()),s.size()},&cfg,&error);};
    check(load(text)&&cfg==gf::BattleTestConfig{},"loose diagnostic INI matches owned fixture, no archive offsets");
    for(const auto& suffix:{"unknown=1\n","tick_rate=30\n","shot_interval=NaN\n","[other]\n"})check(!load(text+suffix),"unknown/duplicate/bad config refuses");
    auto nan=text;const auto at=nan.find("move_per_tick=0.12");nan.replace(at,18,"move_per_tick=nan");check(!load(nan),"nonfinite scalar refuses");
    auto missing=text;missing.erase(missing.find("proxy_art=1"),12);check(!load(missing),"missing diagnostic/proxy acknowledgement refuses");
    auto legacy=text;legacy.replace(legacy.find("format=2"),8,"format=1");check(!load(legacy),"older diagnostic format refuses rather than silently invent new camera/team fields");
    render.shutdown();std::printf("{\"battle_test_checks\":%u,\"diagnostic_rules_only\":true,\"original_combat_ported\":false,\"all_passed\":true}\n",checks);
}
