#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"native active push: %s\n",why);std::abort();}}
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
    auto step=[&](NativeBorgMotionKind kind,BattleMvpAction action,f32 y,f32 speed,f32 entry=0){
        offer={observation,observation.position,observation.facing,action,++serial,producer};
        offer.kind=kind;offer.proposed_position[1]=y;offer.vertical_speed=speed;offer.entry_vertical_delta=entry;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"accepted native phase history");
    };
    auto setup=[&](NativeBorgMotionPolicy policy,bool active=true){
        BattleMvpConfig config;config.spawn={BattlePoint{0,0},BattlePoint{400,400}};
        check(world.start(config,&terrain),"fresh native plane world");serial=0;
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"fresh same-world producer");
        step(NativeBorgMotionKind::JumpTakeoff,BattleMvpAction::Jump,0,0);
        step(NativeBorgMotionKind::JumpAir,BattleMvpAction::Jump,10,10);
        step(NativeBorgMotionKind::JumpAir,BattleMvpAction::Jump,20,0);
        step(NativeBorgMotionKind::AirDashWindupEntry,BattleMvpAction::AirDash,20,0);
        step(NativeBorgMotionKind::AirDashWindup,BattleMvpAction::AirDash,20,active?-.07f:-25.f);
        if(active){
            step(NativeBorgMotionKind::AirDashActiveEntry,BattleMvpAction::AirDash,20,-.07f);
            step(NativeBorgMotionKind::AirDashActive,BattleMvpAction::AirDash,f32(20-.07f),-25);
        }
    };
    auto push=[&](NativeBorgMotionKind kind=NativeBorgMotionKind::AirDashActivePush){
        NativeBorgStaticPush e;e.integrated_position=observation.position;e.integrated_position[1]=f32(observation.position[1]+observation.vertical_speed);
        const f32 acceleration=kind==NativeBorgMotionKind::AirDashRecoveryPush?-1:bits_to_f32(0xbd8f5c29u);
        e.vertical_speed=std::max(-30.f,f32(observation.vertical_speed+acceleration));e.damping=bits_to_f32(0x3f666666u);
        check(terrain.static_surface(e.integrated_position[0],e.integrated_position[2],observation.position[1],&e.surface)&&
            source_surface_angle(e.surface.normal,&e.angle),"independent native pre-plane");
        offer={observation,{},observation.facing,BattleMvpAction::AirDash,serial+1,producer};
        offer.kind=kind;offer.static_push=e;offer.vertical_speed=f32(e.vertical_speed*e.damping);
        check(source_surface_push_position(e.integrated_position,e.surface.normal,e.surface.height,e.angle,&offer.proposed_position),"shared reviewed projection");
    };
    for(auto policy:{NativeBorgMotionPolicy::AirDash,NativeBorgMotionPolicy::Relaunch,NativeBorgMotionPolicy::StaticPush,
                     NativeBorgMotionPolicy::FallHistoryPush,NativeBorgMotionPolicy::RecoveryPush,NativeBorgMotionPolicy::WindupPush}){
        setup(policy);push();const auto tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,"every narrower producer refuses active-push kind");
    }
    setup(NativeBorgMotionPolicy::ActivePush,false);push();const auto before_entry=world.tick();
    check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==before_entry,"active push cannot skip the separate active-entry reset");
    setup(NativeBorgMotionPolicy::ActivePush);push();const auto valid=offer;const auto tick=world.tick();
    auto refuses=[&](const NativeBorgMotionOffer& invalid,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&invalid)&&world.tick()==tick&&world.actors()[0].position==observation.position,why);
    };
    auto invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashActiveEntry;refuses(invalid,"entry cannot borrow continued active push");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashActive;refuses(invalid,"ordinary active packet cannot borrow evidence");
    invalid.static_push.reset();refuses(invalid,"ordinary active integration keeps old-velocity check");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashWindupPush;refuses(invalid,"same acceleration does not grant wind-up history");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashRecoveryPush;refuses(invalid,"recovery cannot borrow active acceleration");
    invalid=valid;invalid.static_push.reset();refuses(invalid,"missing plane evidence refuses");
    invalid=valid;invalid.static_push->integrated_position[1]+=.001f;refuses(invalid,"pre-integration is exact");
    invalid=valid;invalid.static_push->surface.triangle++;refuses(invalid,"spoofed selected plane refuses");
    invalid=valid;invalid.proposed_position[0]+=.001f;refuses(invalid,"edited projection refuses");
    invalid=valid;invalid.entry_vertical_delta=15;refuses(invalid,"active push cannot borrow additive relaunch");
    terrain.metadata=false;refuses(valid,"height-only provider cannot certify active push");terrain.metadata=true;
    check(world.step({},false,&terrain,nullptr,nullptr,&valid)&&world.motion_receipt(valid,&receipt)&&!world.actors()[0].grounded&&
        world.actors()[0].action==BattleMvpAction::AirDash,"active push retains airborne action and exact receipt");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashWindupPush;
    check(!world.motion_receipt(invalid,&receipt),"wind-up receipt cannot replace accepted active receipt");
    check(world.observe_motion(&observation),"post-push observation");serial=valid.serial;push();
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&!world.actors()[0].grounded,"continued active may project repeatedly without a reset");
    terrain.lower=true;
    for(unsigned follow=0;follow<4;++follow){
        setup(NativeBorgMotionPolicy::ActivePush);push();
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"active push before continuation/recovery/relaunch");
        serial=offer.serial;
        if(follow==0)step(NativeBorgMotionKind::AirDashActive,BattleMvpAction::AirDash,
            f32(observation.position[1]+observation.vertical_speed),f32(observation.vertical_speed+bits_to_f32(0xbd8f5c29u)));
        else if(follow==1)step(NativeBorgMotionKind::AirDashRecovery,BattleMvpAction::AirDash,
            f32(observation.position[1]+observation.vertical_speed),f32(observation.vertical_speed-1));
        else if(follow==2){
            terrain.lower=false;push(NativeBorgMotionKind::AirDashRecoveryPush);
            check(world.step({},false,&terrain,nullptr,nullptr,&offer),"recovery push uses its own reset -1 acceleration after active");
            terrain.lower=true;
        }else step(NativeBorgMotionKind::RelaunchEntry,BattleMvpAction::Jump,
            f32(observation.position[1]+f32(observation.vertical_speed+15)),f32(observation.vertical_speed+15+.8f),15);
    }
    std::printf("native continued active-push capability/phase/evidence: %u checks PASS; native proxy candidates, not source controller traces\n",checks);
}
