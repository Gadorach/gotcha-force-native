#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"native air-dash floor: %s\n",why);std::abort();}}
struct Flat:gf::IBattleMvpTerrain{
    bool metadata=true,blocked=false;
    bool height(float,float,float,float* out)const override{*out=blocked?100:0;return true;}
    bool static_surface(float,float,float,gf::NativeBorgStaticSurface* out)const override{
        if(!metadata)return false;
        *out={0,{0,1,0},7};return true;
    }
};
}
int main(){
    using namespace gf;Flat terrain;NativeBattleMvp world;NativeBorgMotionObservation observation;
    NativeBorgMotionOffer offer;NativeBorgMotionReceipt receipt;std::shared_ptr<const NativeBorgMotionProducer> producer;u64 serial=0;
    auto step=[&](NativeBorgMotionKind kind,f32 y,f32 speed){
        offer={observation,observation.position,observation.facing,native_borg_motion_air_dash(kind)?BattleMvpAction::AirDash:BattleMvpAction::Jump,++serial,producer};
        offer.kind=kind;offer.proposed_position[1]=y;offer.vertical_speed=speed;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"ordered native candidate");
    };
    auto setup=[&](NativeBorgMotionPolicy policy){
        check(world.start({},&terrain),"fresh native floor");serial=0;
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"same-world motion producer");
        step(NativeBorgMotionKind::JumpTakeoff,0,0);step(NativeBorgMotionKind::JumpAir,10,10);
        step(NativeBorgMotionKind::JumpAir,20,-10);step(NativeBorgMotionKind::JumpAir,10,-10);
        step(NativeBorgMotionKind::AirDashWindupEntry,10,-5);step(NativeBorgMotionKind::AirDashWindup,5,-5.07f);
    };
    const f32 acceleration=bits_to_f32(0xbd8f5c29u);
    auto contact=[&](NativeBorgMotionKind kind){
        const bool entry=kind==NativeBorgMotionKind::AirDashActiveEntryFloor,windup=kind==NativeBorgMotionKind::AirDashWindupFloor;
        NativeBorgAirDashFloor e;e.integrated_position=observation.position;e.integrated_position[1]=f32(observation.position[1]+(entry?0:observation.vertical_speed));
        e.acceleration=acceleration;e.vertical_speed=std::max(-30.f,f32((entry?0:observation.vertical_speed)+acceleration));
        check(terrain.static_surface(0,0,0,&e.surface),"independent selected floor");
        offer={observation,{observation.position[0],0,observation.position[2]},observation.facing,BattleMvpAction::AirDash,serial+1,producer};
        offer.kind=kind;offer.air_dash_floor=e;offer.vertical_speed=windup?0:e.vertical_speed;
    };
    auto accept=[&](){
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt)&&world.observe_motion(&observation),
            "matching native null-callback floor receipt");serial=offer.serial;
    };
    for(auto policy:{NativeBorgMotionPolicy::AirDash,NativeBorgMotionPolicy::Relaunch,NativeBorgMotionPolicy::StaticPush,
                     NativeBorgMotionPolicy::FallHistoryPush,NativeBorgMotionPolicy::RecoveryPush,NativeBorgMotionPolicy::WindupPush,
                     NativeBorgMotionPolicy::ActivePush,NativeBorgMotionPolicy::RelaunchPush,NativeBorgMotionPolicy::RelaunchEntryPush,
                     NativeBorgMotionPolicy::RelaunchFloor}){
        setup(policy);contact(NativeBorgMotionKind::AirDashWindupFloor);const auto tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,"every older producer refuses air-dash floor authority");
    }
    setup(NativeBorgMotionPolicy::AirDashFloor);contact(NativeBorgMotionKind::AirDashWindupFloor);const auto valid=offer;const auto tick=world.tick();
    auto refuses=[&](NativeBorgMotionOffer invalid,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&invalid)&&world.tick()==tick&&world.actors()[0].position==observation.position,why);
    };
    auto invalid=valid;invalid.air_dash_floor.reset();refuses(invalid,"tag needs separate air-dash floor evidence");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashWindup;refuses(invalid,"ordinary wind-up cannot borrow floor payload");
    invalid.air_dash_floor.reset();refuses(invalid,"ordinary wind-up retains strict above-floor integration");
    invalid=valid;invalid.kind=NativeBorgMotionKind::JumpTouchdown;invalid.action=BattleMvpAction::Jump;refuses(invalid,"snap is not a landing callback");
    invalid=valid;invalid.relaunch_floor=NativeBorgRelaunchFloor{};refuses(invalid,"relaunch payload cannot accompany an air-dash snap");
    invalid=valid;invalid.vertical_speed=f32(valid.air_dash_floor->vertical_speed*.8f);refuses(invalid,"wind-up cannot use relaunch's .8 tail");
    invalid=valid;invalid.air_dash_floor->acceleration=.8f;refuses(invalid,"relaunch acceleration cannot replace dash data70");
    invalid=valid;invalid.air_dash_floor->integrated_position[1]+=.001f;refuses(invalid,"pre-snap displacement is exact");
    invalid=valid;invalid.air_dash_floor->surface.triangle++;refuses(invalid,"selected surface metadata is independently checked");
    invalid=valid;invalid.proposed_position[1]+=.001f;refuses(invalid,"selected floor height is exact");
    invalid=valid;invalid.air_dash_floor->integrated_position[0]+=1;refuses(invalid,"snap cannot replace planar integration");
    invalid=valid;invalid.action=BattleMvpAction::Jump;refuses(invalid,"air-dash action ownership remains explicit");
    terrain.metadata=false;refuses(valid,"height-only provider cannot authorize floor snap");terrain.metadata=true;
    terrain.blocked=true;refuses(valid,"independent native clipping cannot be acknowledged by a source snap");terrain.blocked=false;
    offer=valid;accept();
    check(!world.actors()[0].grounded&&world.actors()[0].position[1]==0&&world.actors()[0].vertical_speed==0,
        "wind-up zero-speed snap remains airborne, not grounded landing/refill");
    invalid=offer;invalid.air_dash_floor->vertical_speed+=.001f;
    check(!world.motion_receipt(invalid,&receipt),"edited pre-response speed cannot borrow exact receipt");
    for(unsigned i=0;i<3;++i){
        contact(NativeBorgMotionKind::AirDashWindupFloor);accept();
        check(!world.actors()[0].grounded&&world.actors()[0].vertical_speed==0,"repeated zero-displacement wind-up snaps remain airborne");
    }
    contact(NativeBorgMotionKind::AirDashActiveFloor);const auto pre_entry=world.tick();
    check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==pre_entry,"active continuation cannot skip a distinct active entry");
    contact(NativeBorgMotionKind::AirDashActiveEntryFloor);accept();
    check(!world.actors()[0].grounded&&world.actors()[0].vertical_speed==acceleration,"active entry's null callback preserves -.07, not wind-up zero or .8");
    for(unsigned i=0;i<3;++i){
        const auto before=observation.vertical_speed;contact(NativeBorgMotionKind::AirDashActiveFloor);
        auto wrong=offer;wrong.vertical_speed=0;const auto before_tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&wrong)&&world.tick()==before_tick,"active null callback cannot borrow65E64 or wind-up zero");
        accept();
        check(!world.actors()[0].grounded&&world.actors()[0].vertical_speed==f32(before+acceleration),
            "active floor speed continues negative acceleration without damping or grounded state");
    }
    step(NativeBorgMotionKind::JumpTouchdown,0,0);
    check(world.actors()[0].grounded,"separate touchdown phase alone grants grounded state");
    std::printf("native air-dash floor/phase/caller/receipt: %u checks PASS; native proxies, no original whole-frame authority\n",checks);
}
