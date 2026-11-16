#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"native relaunch push: %s\n",why);std::abort();}}
struct Plane:gf::IBattleMvpTerrain{
    bool metadata=true,lower=false;
    bool height(float x,float,float,float* out)const override{*out=x>50?0:lower&&x>.01f?-100:-3*x;return true;}
    bool static_surface(float x,float,float,gf::NativeBorgStaticSurface* out)const override{
        if(!metadata)return false;
        const float length=std::sqrt(10.f);*out={-3*x,{3/length,1/length,0},7};return true;
    }
};
}
int main(){
    using namespace gf;Plane terrain;NativeBattleMvp world;NativeBorgMotionObservation observation;NativeBorgMotionOffer offer;NativeBorgMotionReceipt receipt;
    std::shared_ptr<const NativeBorgMotionProducer> producer;u64 serial=0;
    auto step=[&](NativeBorgMotionKind kind,f32 y,f32 speed,f32 entry=0){
        offer={observation,observation.position,observation.facing,BattleMvpAction::Jump,++serial,producer};
        offer.kind=kind;offer.proposed_position[1]=y;offer.vertical_speed=speed;offer.entry_vertical_delta=entry;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"accepted native relaunch phase candidate");
    };
    auto setup=[&](NativeBorgMotionPolicy policy,unsigned stage=0){
        BattleMvpConfig config;config.spawn={BattlePoint{0,0},BattlePoint{400,400}};
        check(world.start(config,&terrain),"fresh native static plane");serial=0;
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"fresh source producer");
        step(NativeBorgMotionKind::JumpTakeoff,0,0);
        step(NativeBorgMotionKind::JumpAir,10,10);
        step(NativeBorgMotionKind::JumpAir,20,-20);
        if(stage==3)return; //no accepted additive entry
        step(NativeBorgMotionKind::RelaunchEntry,15,stage?-5.f:-20.f,15);
        if(stage)step(NativeBorgMotionKind::RelaunchWindup,10,stage==2?-5.f:-20.f);
        if(stage==2)step(NativeBorgMotionKind::RelaunchActive,5,-20);
    };
    auto push=[&](NativeBorgMotionKind kind,f32 acceleration){
        NativeBorgStaticPush e;e.integrated_position=observation.position;e.integrated_position[1]=f32(observation.position[1]+observation.vertical_speed);
        e.vertical_speed=std::clamp(f32(observation.vertical_speed+acceleration),-30.f,18.f);e.damping=bits_to_f32(0x3f666666u);e.relaunch_acceleration=acceleration;
        check(terrain.static_surface(e.integrated_position[0],e.integrated_position[2],observation.position[1],&e.surface)&&
            source_surface_angle(e.surface.normal,&e.angle),"independent pre-push plane");
        offer={observation,{},observation.facing,BattleMvpAction::Jump,serial+1,producer};offer.kind=kind;offer.static_push=e;
        offer.vertical_speed=f32(e.vertical_speed*e.damping);
        check(source_surface_push_position(e.integrated_position,e.surface.normal,e.surface.height,e.angle,&offer.proposed_position),"shared source projection");
    };
    f32 cosine=0;source_heading_xz(8192,nullptr,&cosine);const f32 neutral=bits_to_f32(0x3f4ccccdu),directional=std::fma(neutral,cosine,0.f);
    for(auto policy:{NativeBorgMotionPolicy::Relaunch,NativeBorgMotionPolicy::StaticPush,NativeBorgMotionPolicy::FallHistoryPush,
                     NativeBorgMotionPolicy::RecoveryPush,NativeBorgMotionPolicy::WindupPush,NativeBorgMotionPolicy::ActivePush}){
        setup(policy);push(NativeBorgMotionKind::RelaunchWindupPush,neutral);const auto tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,"every older policy refuses continued relaunch projection");
    }
    setup(NativeBorgMotionPolicy::RelaunchPush,3);push(NativeBorgMotionKind::RelaunchWindupPush,neutral);const auto no_entry=world.tick();
    check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==no_entry,"cannot skip additive relaunch entry");
    setup(NativeBorgMotionPolicy::RelaunchPush);push(NativeBorgMotionKind::RelaunchWindupPush,neutral);const auto valid=offer;const auto tick=world.tick();
    auto refuses=[&](const NativeBorgMotionOffer& invalid,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&invalid)&&world.tick()==tick&&world.actors()[0].position==observation.position,why);
    };
    auto invalid=valid;invalid.entry_vertical_delta=15;refuses(invalid,"continued push cannot repeat additive entry");
    invalid=valid;invalid.kind=NativeBorgMotionKind::RelaunchEntry;refuses(invalid,"entry kind cannot borrow push evidence");
    invalid=valid;invalid.kind=NativeBorgMotionKind::RelaunchActivePush;refuses(invalid,"active relaunch cannot skip accepted wind-up update");
    invalid=valid;invalid.static_push->relaunch_acceleration.reset();refuses(invalid,"missing relaunch acceleration refuses");
    invalid=valid;invalid.static_push->relaunch_acceleration=-.07f;refuses(invalid,"dash acceleration cannot substitute for relaunch");
    invalid=valid;invalid.static_push->relaunch_acceleration=.7f;refuses(invalid,"unreviewed relaunch acceleration refuses");
    invalid=valid;invalid.static_push->integrated_position[1]+=.001f;refuses(invalid,"pre-push integration is exact");
    invalid=valid;invalid.static_push->surface.triangle++;refuses(invalid,"spoofed selected plane refuses");
    invalid=valid;invalid.proposed_position[0]+=.001f;refuses(invalid,"edited projection refuses");
    invalid=valid;invalid.kind=NativeBorgMotionKind::StaticPush;
    invalid.static_push->vertical_speed=std::max(-30.f,f32(observation.vertical_speed-1));
    invalid.vertical_speed=f32(invalid.static_push->vertical_speed*.9f);
    refuses(invalid,"otherwise-valid falling reset cannot carry relaunch-only evidence");
    terrain.metadata=false;refuses(valid,"height-only terrain refuses");terrain.metadata=true;
    check(world.step({},false,&terrain,nullptr,nullptr,&valid)&&world.motion_receipt(valid,&receipt)&&!world.actors()[0].grounded&&
        world.actors()[0].action==BattleMvpAction::Jump,"wind-up relaunch push stays airborne with separate jump action/receipt");
    invalid=valid;invalid.static_push->relaunch_acceleration=directional;
    check(!world.motion_receipt(invalid,&receipt),"changed acceleration cannot borrow exact receipt");
    check(world.observe_motion(&observation),"current continued relaunch observation");serial=valid.serial;
    push(NativeBorgMotionKind::RelaunchWindupPush,directional);
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"directional relaunch can continue with its reviewed acceleration");
    serial=offer.serial;push(NativeBorgMotionKind::RelaunchActivePush,neutral);
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"active15 request may follow wind-up push without an invented extra update");
    serial=offer.serial;push(NativeBorgMotionKind::RelaunchWindupPush,neutral);const auto active_tick=world.tick();
    check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==active_tick,"active relaunch cannot rewind its lower stage");
    for(unsigned stage:{1u,2u})for(f32 acceleration:{neutral,directional}){
        setup(NativeBorgMotionPolicy::RelaunchPush,stage);push(NativeBorgMotionKind::RelaunchActivePush,acceleration);
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&!world.actors()[0].grounded,
            "continued active relaunch accepts only reviewed direction-dependent acceleration and existing handoff");
    }
    terrain.lower=true;setup(NativeBorgMotionPolicy::RelaunchPush);push(NativeBorgMotionKind::RelaunchWindupPush,neutral);
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"relaunch push before release to falling");
    serial=offer.serial;
    step(NativeBorgMotionKind::JumpAir,f32(observation.position[1]+observation.vertical_speed),f32(observation.vertical_speed-1));
    std::printf("native continued relaunch acceleration/phase/receipt: %u checks PASS; native proxy candidates, no original whole-frame equivalence\n",checks);
}
