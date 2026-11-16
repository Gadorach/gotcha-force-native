#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"native wind-up push: %s\n",why);std::abort();}}
struct Plane:gf::IBattleMvpTerrain{
    bool metadata=true,lower=false;
    bool height(float x,float,float,float* out)const override{*out=x>50?0:lower&&x>.01f?-100:-3*x;return true;}
    bool static_surface(float x,float,float,gf::NativeBorgStaticSurface* out)const override{
        if(!metadata)return false;
        const float length=std::sqrt(10.f);*out={-3*x,{3/length,1/length,0},7};return true;
    }
};
struct ReferenceFloors:gf::IBattleMvpTerrain{
    bool height(float,float,float reference,float* out)const override{*out=reference>=55?100:0;return true;}
};
}
int main(){
    using namespace gf;NativeBattleMvp world;NativeBorgMotionObservation observation;NativeBorgMotionOffer offer;NativeBorgMotionReceipt receipt;
    ReferenceFloors floors;BattleMvpConfig config;
    check(world.start(config,&floors)&&world.actors()[0].position[1]==0,"default spawn-floor reference remains0");
    config.spawn_query_y[0]=100;
    check(world.start(config,&floors)&&world.actors()[0].position[1]==100&&world.actors()[0].grounded&&world.actors()[0].vertical_speed==0,
        "declared native reference queries a real selected floor, never an airborne position override");
    check(world.observe_motion(&observation),"reference-floor observation");
    for(f32 bad:{std::numeric_limits<f32>::quiet_NaN(),20001.f,-20001.f}){
        auto invalid=config;invalid.spawn_query_y[0]=bad;
        check(!world.start(invalid,&floors)&&world.motion_observation_current(observation),"invalid reference refuses without replacing the current world");
    }
    Plane terrain;std::shared_ptr<const NativeBorgMotionProducer> producer;u64 serial=0;
    auto step=[&](NativeBorgMotionKind kind,BattleMvpAction action,f32 y,f32 speed,f32 entry=0){
        offer={observation,observation.position,observation.facing,action,++serial,producer};
        offer.kind=kind;offer.proposed_position[1]=y;offer.vertical_speed=speed;offer.entry_vertical_delta=entry;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"accepted native wind-up history");
    };
    auto setup=[&](NativeBorgMotionPolicy policy,bool windup=true){
        BattleMvpConfig c;c.spawn={BattlePoint{0,0},BattlePoint{400,400}};
        check(world.start(c,&terrain),"fresh plane world");serial=0;
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"fresh same-world producer");
        step(NativeBorgMotionKind::JumpTakeoff,BattleMvpAction::Jump,0,0);
        step(NativeBorgMotionKind::JumpAir,BattleMvpAction::Jump,10,10);
        step(NativeBorgMotionKind::JumpAir,BattleMvpAction::Jump,20,windup?-10.f:-25.f);
        if(!windup)return;
        step(NativeBorgMotionKind::AirDashWindupEntry,BattleMvpAction::AirDash,20,-5);
        for(unsigned n=0;n<3;++n)step(NativeBorgMotionKind::AirDashWindup,BattleMvpAction::AirDash,
            f32(observation.position[1]+observation.vertical_speed),f32(observation.vertical_speed+bits_to_f32(0xbd8f5c29u)));
    };
    auto push=[&](){
        NativeBorgStaticPush e;e.integrated_position=observation.position;e.integrated_position[1]=f32(observation.position[1]+observation.vertical_speed);
        e.vertical_speed=std::max(-30.f,f32(observation.vertical_speed+bits_to_f32(0xbd8f5c29u)));e.damping=bits_to_f32(0x3f666666u);
        check(terrain.static_surface(e.integrated_position[0],e.integrated_position[2],observation.position[1],&e.surface)&&
            source_surface_angle(e.surface.normal,&e.angle),"independent pre-push plane");
        offer={observation,{},observation.facing,BattleMvpAction::AirDash,serial+1,producer};
        offer.kind=NativeBorgMotionKind::AirDashWindupPush;offer.static_push=e;offer.vertical_speed=f32(e.vertical_speed*e.damping);
        check(source_surface_push_position(e.integrated_position,e.surface.normal,e.surface.height,e.angle,&offer.proposed_position),"shared reviewed projection");
    };
    for(auto policy:{NativeBorgMotionPolicy::AirDash,NativeBorgMotionPolicy::Relaunch,NativeBorgMotionPolicy::StaticPush,
                     NativeBorgMotionPolicy::FallHistoryPush,NativeBorgMotionPolicy::RecoveryPush}){
        setup(policy);push();const auto tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,"every older producer refuses continued wind-up push kind");
    }
    setup(NativeBorgMotionPolicy::WindupPush,false);push();const auto no_entry=world.tick();
    check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==no_entry,"wind-up push cannot skip accepted no-motion entry");
    setup(NativeBorgMotionPolicy::WindupPush);push();const auto valid=offer;const auto tick=world.tick();
    auto refuses=[&](const NativeBorgMotionOffer& invalid,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&invalid)&&world.tick()==tick&&world.actors()[0].position==observation.position,why);
    };
    auto invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashWindupEntry;refuses(invalid,"entry cannot borrow continued-frame projection evidence");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashWindup;refuses(invalid,"ordinary wind-up cannot borrow a push packet");
    invalid.static_push.reset();refuses(invalid,"ordinary wind-up retains old-velocity integration checks");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashRecoveryPush;refuses(invalid,"recovery kind cannot borrow wind-up acceleration/history");
    invalid=valid;invalid.static_push->vertical_speed=std::max(-30.f,f32(observation.vertical_speed-1));
    invalid.vertical_speed=f32(invalid.static_push->vertical_speed*.9f);refuses(invalid,"fall/recovery acceleration -1 cannot replace source wind-up -.07");
    invalid=valid;invalid.static_push->integrated_position[1]+=.001f;refuses(invalid,"pre-push wind-up integration is exact");
    invalid=valid;invalid.static_push->surface.triangle++;refuses(invalid,"spoofed selected plane refuses");
    invalid=valid;invalid.proposed_position[0]+=.001f;refuses(invalid,"edited scale/add result refuses");
    invalid=valid;invalid.entry_vertical_delta=15;refuses(invalid,"wind-up push cannot borrow additive relaunch");
    terrain.metadata=false;refuses(valid,"height-only provider cannot certify wind-up projection");terrain.metadata=true;
    check(world.step({},false,&terrain,nullptr,nullptr,&valid)&&world.motion_receipt(valid,&receipt)&&
        !world.actors()[0].grounded&&world.actors()[0].action==BattleMvpAction::AirDash,"continued wind-up push retains airborne action/receipt");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashRecoveryPush;
    check(!world.motion_receipt(invalid,&receipt),"recovery cannot borrow accepted wind-up receipt");
    check(world.observe_motion(&observation),"after push observation");serial=valid.serial;push();
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&!world.actors()[0].grounded,"another wind-up projection preserves its distinct acceleration");
    terrain.lower=true;
    for(unsigned follow=0;follow<3;++follow){
        setup(NativeBorgMotionPolicy::WindupPush);push();
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"wind-up push before continued/active/relaunch handoff");
        serial=offer.serial;
        if(follow==0)step(NativeBorgMotionKind::AirDashWindup,BattleMvpAction::AirDash,
            f32(observation.position[1]+observation.vertical_speed),f32(observation.vertical_speed+bits_to_f32(0xbd8f5c29u)));
        else if(follow==1)step(NativeBorgMotionKind::AirDashActiveEntry,BattleMvpAction::AirDash,observation.position[1],-.07f);
        else step(NativeBorgMotionKind::RelaunchEntry,BattleMvpAction::Jump,
            f32(observation.position[1]+f32(observation.vertical_speed+15)),f32(observation.vertical_speed+15+.8f),15);
    }
    std::printf("native continued wind-up push/phase/acceleration/reference: %u checks PASS; native proxies, no original whole-frame equivalence\n",checks);
}
