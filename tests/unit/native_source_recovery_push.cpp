#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"native recovery push: %s\n",why);std::abort();}}
struct Plane:gf::IBattleMvpTerrain{
    bool metadata=true,lower=false;
    bool height(float x,float,float,float* out)const override{*out=x>50?0:lower&&x>1?-100:-3*x;return true;}
    bool static_surface(float x,float,float,gf::NativeBorgStaticSurface* out)const override{
        if(!metadata)return false;
        const float length=std::sqrt(10.f);*out={-3*x,{3/length,1/length,0},7};return true;
    }
};
}
int main(){
    using namespace gf;Plane terrain;NativeBattleMvp world;NativeBorgMotionObservation observation;NativeBorgMotionOffer offer;
    NativeBorgMotionReceipt receipt;std::shared_ptr<const NativeBorgMotionProducer> producer;u64 serial=0;
    auto step=[&](NativeBorgMotionKind kind,BattleMvpAction action,f32 y,f32 speed,f32 entry=0){
        offer={observation,observation.position,observation.facing,action,++serial,producer};
        offer.kind=kind;offer.proposed_position[1]=y;offer.vertical_speed=speed;offer.entry_vertical_delta=entry;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"accepted native motion history");
    };
    auto setup=[&](NativeBorgMotionPolicy policy,unsigned phase){
        BattleMvpConfig config;config.spawn={BattlePoint{0,0},BattlePoint{400,400}};
        check(world.start(config,&terrain),"native static plane world");serial=0;
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"fresh same-world producer");
        step(NativeBorgMotionKind::JumpTakeoff,BattleMvpAction::Jump,0,0);
        step(NativeBorgMotionKind::JumpAir,BattleMvpAction::Jump,10,10);
        step(NativeBorgMotionKind::JumpAir,BattleMvpAction::Jump,20,-25);
        if(!phase)return;
        step(NativeBorgMotionKind::AirDashWindupEntry,BattleMvpAction::AirDash,20,-25);
    };
    //A separate explicit active-history builder retains native old-velocity
    //checks. Fixtures are native phase candidates, not source controller traces.
    auto active=[&](NativeBorgMotionPolicy policy){
        BattleMvpConfig config;config.spawn={BattlePoint{0,0},BattlePoint{400,400}};
        check(world.start(config,&terrain),"active fixture world");serial=0;
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"active fixture producer");
        step(NativeBorgMotionKind::JumpTakeoff,BattleMvpAction::Jump,0,0);
        step(NativeBorgMotionKind::JumpAir,BattleMvpAction::Jump,10,10);
        step(NativeBorgMotionKind::JumpAir,BattleMvpAction::Jump,20,0);
        step(NativeBorgMotionKind::AirDashWindupEntry,BattleMvpAction::AirDash,20,0);
        step(NativeBorgMotionKind::AirDashWindup,BattleMvpAction::AirDash,20,-.07f);
        step(NativeBorgMotionKind::AirDashActiveEntry,BattleMvpAction::AirDash,20,-.07f);
        step(NativeBorgMotionKind::AirDashActive,BattleMvpAction::AirDash,f32(20-.07f),-25);
    };
    auto push=[&](){
        NativeBorgStaticPush e;e.integrated_position=observation.position;e.integrated_position[1]=f32(observation.position[1]+observation.vertical_speed);
        e.vertical_speed=std::max(-30.f,f32(observation.vertical_speed-1.f));e.damping=bits_to_f32(0x3f666666u);
        check(terrain.static_surface(e.integrated_position[0],e.integrated_position[2],observation.position[1],&e.surface)&&
            source_surface_angle(e.surface.normal,&e.angle),"independent native pre-plane");
        offer={observation,{},observation.facing,BattleMvpAction::AirDash,serial+1,producer};
        offer.kind=NativeBorgMotionKind::AirDashRecoveryPush;offer.static_push=e;offer.vertical_speed=f32(e.vertical_speed*e.damping);
        check(source_surface_push_position(e.integrated_position,e.surface.normal,e.surface.height,e.angle,&offer.proposed_position),"reviewed shared projection");
    };
    for(auto policy:{NativeBorgMotionPolicy::AirDash,NativeBorgMotionPolicy::Relaunch,NativeBorgMotionPolicy::StaticPush,NativeBorgMotionPolicy::FallHistoryPush}){
        active(policy);push();const auto tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,"every narrower producer refuses new recovery-push kind");
    }
    for(unsigned phase:{0u,1u}){
        setup(NativeBorgMotionPolicy::RecoveryPush,phase);push();const auto tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,"recovery cannot skip active phase or replace windup");
    }
    terrain.lower=true;active(NativeBorgMotionPolicy::RecoveryPush);push();const auto valid=offer;const auto tick=world.tick();
    auto refuses=[&](const NativeBorgMotionOffer& invalid,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&invalid)&&world.tick()==tick&&world.actors()[0].position==observation.position,why);
    };
    auto invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashRecovery;refuses(invalid,"ordinary recovery cannot borrow projection evidence");
    invalid.static_push.reset();refuses(invalid,"ordinary recovery keeps its old-velocity check");
    invalid=valid;invalid.kind=NativeBorgMotionKind::StaticPush;invalid.action=BattleMvpAction::Jump;refuses(invalid,"falling kind cannot skip active-to-recovery callback contract");
    invalid=valid;invalid.static_push.reset();refuses(invalid,"recovery push requires selected-plane evidence");
    invalid=valid;invalid.entry_vertical_delta=15;refuses(invalid,"recovery push cannot borrow additive relaunch step");
    invalid=valid;invalid.static_push->integrated_position[1]+=.001f;refuses(invalid,"recovery pre-integration is exact, not ordinary-air tolerance");
    invalid=valid;invalid.static_push->surface.triangle++;refuses(invalid,"foreign selected plane refuses");
    invalid=valid;invalid.proposed_position[0]+=.001f;refuses(invalid,"edited projection cannot become accepted recovery");
    terrain.metadata=false;refuses(valid,"height-only terrain refuses recovery evidence");terrain.metadata=true;
    check(world.step({},false,&terrain,nullptr,nullptr,&valid)&&world.motion_receipt(valid,&receipt)&&!world.actors()[0].grounded&&
        world.actors()[0].action==BattleMvpAction::AirDash&&world.actors()[0].position==valid.proposed_position,
        "distinct recovery push remains airborne and retains recovery action/receipt");
    invalid=valid;invalid.kind=NativeBorgMotionKind::StaticPush;invalid.action=BattleMvpAction::Jump;
    check(!world.motion_receipt(invalid,&receipt),"recovery receipt cannot be relabelled as falling");
    check(world.observe_motion(&observation),"after recovery push observation");
    serial=valid.serial;
    step(NativeBorgMotionKind::AirDashRecovery,BattleMvpAction::AirDash,f32(observation.position[1]+observation.vertical_speed),-24.4f);
    step(NativeBorgMotionKind::JumpAir,BattleMvpAction::Jump,f32(observation.position[1]+observation.vertical_speed),-25.4f);
    check(!world.actors()[0].grounded,"continued recovery can finish then enter ordinary falling without native gravity fallback");
    terrain.lower=false;active(NativeBorgMotionPolicy::RecoveryPush);push();
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"first push on continuous plane");
    serial=offer.serial;push();
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation)&&!world.actors()[0].grounded,
        "recovery can continue with another independently validated projection, not just a one-shot push");
    terrain.lower=true;
    active(NativeBorgMotionPolicy::RecoveryPush);push();
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"recovery push at completion boundary");
    serial=offer.serial;
    step(NativeBorgMotionKind::JumpAir,BattleMvpAction::Jump,f32(observation.position[1]+observation.vertical_speed),f32(observation.vertical_speed-1));
    check(!world.actors()[0].grounded,"completed recovery may enter falling directly after push without a fabricated extra recovery tick");
    for(bool relaunch:{false,true}){
        active(NativeBorgMotionPolicy::RecoveryPush);push();
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"recovery push before source windup/relaunch restart");
        serial=offer.serial;
        if(relaunch)step(NativeBorgMotionKind::RelaunchEntry,BattleMvpAction::Jump,
            f32(observation.position[1]+f32(observation.vertical_speed+15)),f32(observation.vertical_speed+15+.8f),15);
        else step(NativeBorgMotionKind::AirDashWindupEntry,BattleMvpAction::AirDash,observation.position[1],observation.vertical_speed*.5f);
    }
    std::printf("native recovery-push capability/phase/evidence: %u checks PASS; native proxies, no original whole-frame equivalence\n",checks);
}
