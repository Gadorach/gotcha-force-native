#include <cstdlib>
#include "engine/gameplay/borg_control_selector.hpp"
#include "engine/gameplay/borg_lab_picker.hpp"
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
void check(bool value){if(!value)std::abort();}
struct Terrain final:gf::IBattleMvpTerrain {
    mutable unsigned origin_queries=0;
    bool height(gf::f32 x,gf::f32 z,gf::f32,gf::f32* out)const override{
        if(!x&&!z){++origin_queries;return false;}
        *out=10;return true;
    }
};
}

int main(){
    {
        std::vector<gf::BorgLabPicker::Entry> rows;
        for(unsigned i=0;i<20;++i)rows.push_back({"example:same","same name","look-"+std::to_string(i)});
        gf::BorgLabPicker lab(std::move(rows));
        gf::PointerState pointer;pointer.within_window=true;pointer.left_pressed=true;pointer.x=30;pointer.y=28;
        check(!lab.step(pointer,500)&&lab.open());
        pointer.wheel=-1;pointer.left_pressed=false;
        check(!lab.step(pointer,500)&&lab.open());
        pointer.left_pressed=true;pointer.wheel=0;pointer.y=28+2*gf::BorgLabPicker::row_height;
        check(lab.step(pointer,500)==9&&!lab.open());
        lab.activate(9);pointer.y=28;
        check(!lab.step(pointer,500)&&lab.open());
        pointer.y=28+2*gf::BorgLabPicker::row_height;
        check(lab.step(pointer,500)==9&&!lab.open());
        pointer.y=28;check(!lab.step(pointer,500)&&lab.open());
        pointer.x=499;pointer.y=52;
        check(!lab.step(pointer,500)&&!lab.open());
    }
    gf::BorgControlSelector selector;
    gf::PointerState p;p.x=32;p.y=30;p.within_window=true;
    check(!selector.select(p)&&!selector.open());
    p.left_pressed=true;
    check(!selector.select(p)&&selector.open());
    p.y=94;
    check(selector.select(p)==1&&!selector.open());
    p.y=30;check(!selector.select(p)&&selector.open());
    p.y=500;check(!selector.select(p)&&!selector.open());
    p.y=30;p.within_window=false;check(!selector.select(p)&&!selector.open());

    Terrain terrain;gf::NativeBattleMvp world;gf::BattleMvpConfig config;
    config.solo_model=gf::BorgControlSelector::entries[1].model;
    config.spawn[1]={0,0}; // neither an actor nor a terrain probe
    check(world.start(config,&terrain)&&world.actor_count()==1);
    const auto old_scope=world.presentation_scope();
    check(world.actors()[0].model==0x000a&&!world.actors()[1].model&&!world.actors()[1].health);
    gf::GamePadState input;input.a=input.x=input.b=true;input.dpad_up=true;
    check(!world.step({input,{}},true,&terrain)&&!world.tick());
    check(!world.step({gf::GamePadState{},input},false,&terrain)&&!world.tick());
    check(world.step({input,{}},false,&terrain)&&world.tick()==1);
    check(!world.actors()[0].grounded&&world.actors()[0].aim_facing==world.actors()[0].facing);
    check(!world.shots_spawned()&&!world.hits()&&!terrain.origin_queries);
    check(world.result()==gf::BattleMvpResult::Fighting);

    config.solo_model=gf::BorgControlSelector::entries[0].model;
    check(world.start(config,&terrain)&&!world.owns_motion_scope(old_scope));
    gf::NativeBorgMotionObservation motion;
    check(world.observe_motion(&motion)&&motion.model==0x0615&&!world.tick());
    check(!world.step({},false,&terrain)&&!world.tick()); // no MVP fallback
    config.solo_model=0x0401;
    check(!world.start(config,&terrain)&&world.actors()[0].model==0x0615);
    config.solo_model=0;config.provisional_walk=true;
    check(world.start(config,&terrain)&&world.actor_count()==1&&!world.actors()[0].model);
    const auto start=world.actors()[0].position;
    input.l=input.trigger_l_pressed=true;
    check(!world.step({input,{}},true,&terrain)&&!world.tick());
    check(!world.step({gf::GamePadState{},input},false,&terrain)&&!world.tick());
    for(unsigned i=0;i<24;++i)check(world.step({input,{}},false,&terrain));
    check(world.actors()[0].position!=start&&world.actors()[0].grounded&&!world.actors()[0].dash_ticks&&
          !world.actors()[1].health&&!world.shots_spawned()&&!world.hits()&&world.actor_count()==1);
    config.solo_model=0x000a;
    check(!world.start(config,&terrain)); // the exact solo controller path is not interchangeable
    config.solo_model=0;config.provisional_jump=true;config.provisional_walk=false;
    check(!world.start(config,&terrain)); // jump capability requires one-actor lab admission
    config.provisional_walk=true;
    check(world.start(config,&terrain)&&world.actor_count()==1);
    unsigned jumps=0,landings=0;
    for(unsigned frame=0;frame<80;++frame){
        const auto grounded=world.actors()[0].grounded;
        input.b=frame==2||frame==8;
        check(world.step({input,{}},false,&terrain)&&!world.actors()[0].dash_ticks&&
              !world.shots_spawned()&&!world.hits());
        jumps+=unsigned(grounded&&!world.actors()[0].grounded&&world.actors()[0].takeoff_ticks);
        landings+=unsigned(!grounded&&world.actors()[0].grounded&&world.actors()[0].landing_ticks);
    }
    check(jumps==1&&landings==1&&world.actors()[0].grounded);
    config.provisional_ground_dash=true;config.provisional_walk=false;config.provisional_jump=false;
    check(!world.start(config,&terrain));
    config.provisional_walk=true;config.provisional_jump=true;
    check(world.start(config,&terrain));
    gf::GamePadState dash;dash.a=dash.x=dash.l=dash.dpad_left=true;
    const auto body_heading=world.actors()[0].facing;
    check(world.step({dash,{}},false,&terrain)&&world.actors()[0].dash_ticks&&
          world.actors()[0].dash_motion==gf::BattleMvpDashDirection::Left&&world.actors()[0].facing==body_heading);
    const auto dash_direction=world.actors()[0].dash_direction;
    dash.dpad_left=false;dash.dpad_right=true;
    check(world.step({dash,{}},false,&terrain)&&world.actors()[0].dash_direction==dash_direction);
    check(world.step({},false,&terrain));
    dash.b=true;
    check(world.step({dash,{}},false,&terrain)&&!world.actors()[0].grounded&&!world.actors()[0].dash_ticks);
    check(world.start(config,&terrain));
    gf::GamePadState airborne;airborne.b=true;
    check(world.step({airborne,{}},false,&terrain)&&!world.actors()[0].grounded);
    const auto rise=world.actors()[0].vertical_speed;
    dash.b=false;
    check(world.step({dash,{}},false,&terrain)&&!world.actors()[0].dash_ticks&&!world.actors()[0].dash_cooldown&&
          !world.actors()[0].air_dashes_used&&world.actors()[0].vertical_speed==rise-config.gravity&&
          !world.shots_spawned()&&!world.hits());
    config.provisional_air_dash=true;config.provisional_jump=false;
    check(!world.start(config,&terrain));
    config.provisional_jump=true;config.provisional_ground_dash=false;
    check(world.start(config,&terrain));
    gf::GamePadState ground_l;ground_l.l=true;
    check(world.step({ground_l,{}},false,&terrain)&&world.actors()[0].grounded&&!world.actors()[0].dash_ticks);
    for(unsigned airtime=0;airtime<2;++airtime){
        unsigned windup=0,active=0,recovery=0,ignored=0;
        gf::BattlePoint locked{};
        for(unsigned frame=0;frame<60;++frame){
            gf::GamePadState pad;
            pad.b=frame==0;pad.l=frame==10||frame==32;
            pad.dpad_left=frame==10;pad.dpad_right=frame>10&&frame<30;
            const auto before=world.actors()[0].vertical_speed;
            const bool was_airborne=!world.actors()[0].grounded;
            check(world.step({pad,{}},false,&terrain)&&!world.shots_spawned()&&!world.hits());
            const auto& body=world.actors()[0];
            if(was_airborne&&!body.grounded)check(body.vertical_speed==before-config.gravity);
            if(frame==10){
                locked=body.dash_direction;
                check(body.lab_air_windup_ticks==gf::BattleMvpActor::lab_air_windup&&body.air_dashes_used==1);
            }
            if(body.dash_ticks){
                check(!body.grounded&&body.dash_airborne&&body.dash_direction==locked&&body.facing==locked);
                ++active;
            }
            windup+=body.lab_air_windup_ticks!=0;
            recovery+=body.lab_air_recovery_ticks!=0;
            if(frame==32&&!body.grounded&&!body.dash_ticks&&!body.lab_air_windup_ticks&&body.air_dashes_used==1)++ignored;
        }
        check(windup==gf::BattleMvpActor::lab_air_windup&&active==gf::BattleMvpActor::lab_air_burst&&
              recovery==gf::BattleMvpActor::lab_air_recovery&&ignored==1&&world.actors()[0].grounded&&
              !world.actors()[0].air_dashes_used);
    }
    check(world.start(config,&terrain));
    bool late_windup=false;
    for(unsigned frame=0;frame<60;++frame){
        gf::GamePadState pad;pad.b=frame==0;pad.l=frame==46;
        check(world.step({pad,{}},false,&terrain)&&!world.actors()[0].dash_ticks);
        late_windup|=world.actors()[0].lab_air_windup_ticks!=0;
    }
    check(late_windup&&world.actors()[0].grounded&&!world.actors()[0].lab_air_windup_ticks&&
          !world.actors()[0].lab_air_recovery_ticks&&!world.actors()[0].air_dashes_used);
    config.provisional_glide=true;
    check(!world.start(config,&terrain)); // glide and air dash are separately scoped
    config.provisional_air_dash=false;
    check(world.start(config,&terrain));
    unsigned held_takeoffs=0,held_landings=0,glide_ticks=0;
    for(unsigned frame=0;frame<150;++frame){
        gf::GamePadState pad;pad.b=true;
        const auto before=world.actors()[0];
        check(world.step({pad,{}},false,&terrain));
        const auto& body=world.actors()[0];
        held_takeoffs+=before.grounded&&!body.grounded;
        held_landings+=!before.grounded&&body.grounded;
        if(body.lab_gliding){
            const auto drop=before.position[1]-body.position[1];
            check(!body.grounded&&!body.dash_ticks&&drop>0&&drop<=gf::BattleMvpActor::lab_glide_sink+.001f);
            ++glide_ticks;
        }
    }
    check(held_takeoffs==1&&held_landings==1&&glide_ticks>30&&world.actors()[0].grounded&&
          !world.actors()[0].lab_gliding&&!world.actors()[0].lab_glide_entry_ticks);
    gf::BattleMvpConfig cruise;
    cruise.provisional_cruise=gf::BorgCruiseBoost{120,24,6,4,2,3};
    check(!world.start(cruise,&terrain)); // can only enter through the solo lab
    cruise.provisional_walk=true;cruise.provisional_jump=true;
    check(!world.start(cruise,&terrain));
    cruise.provisional_jump=false;
    check(world.start(cruise,&terrain)&&!world.actors()[0].grounded);
    const auto cruise_y=world.actors()[0].position[1];
    gf::GamePadState thrust;thrust.b=thrust.x=thrust.a=thrust.l=true;
    check(world.step({thrust,{}},false,&terrain)&&world.actors()[0].package_boosting&&
          world.actors()[0].package_boost_meter.value736==18&&world.actors()[0].movement_speed>cruise.move_speed);
    for(unsigned frame=0;frame<15;++frame){
        check(world.step({thrust,{}},false,&terrain));
        const auto& body=world.actors()[0];
        check(!body.grounded&&body.position[1]==cruise_y&&!body.landing_ticks&&!body.dash_ticks&&
              !world.shots_spawned()&&!world.hits());
    }
    check(!world.actors()[0].package_boosting&&world.actors()[0].package_boost_meter.value736==24);
    check(world.step({},false,&terrain)&&world.step({thrust,{}},false,&terrain)&&world.actors()[0].package_boosting);
    gf::BattleMvpConfig pace;
    pace.provisional_ground_pace=750;
    check(!world.start(pace,&terrain)); // cannot leak into ordinary battle tuning
    pace.provisional_walk=pace.provisional_jump=true;
    check(world.start(pace,&terrain));
    gf::GamePadState travel;travel.dpad_up=true;
    for(unsigned tick=0;tick<20;++tick)check(world.step({travel,{}},false,&terrain));
    check(std::abs(world.actors()[0].movement_speed-7.5f)<.001f);
    travel.b=true;
    check(world.step({travel,{}},false,&terrain)&&!world.actors()[0].grounded&&
          std::abs(world.actors()[0].movement_speed-pace.move_speed)<.001f);
    pace.provisional_ground_pace.reset();travel.b=false;
    check(world.start(pace,&terrain));
    for(unsigned tick=0;tick<20;++tick)check(world.step({travel,{}},false,&terrain));
    check(std::abs(world.actors()[0].movement_speed-pace.move_speed)<.001f);
    check(world.start({},&terrain)&&world.actor_count()==2&&world.actors()[1].model==0x000a);
}
