#include <cstdlib>
#include "engine/gameplay/borg_control_selector.hpp"
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
    check(world.start({},&terrain)&&world.actor_count()==2&&world.actors()[1].model==0x000a);
}
