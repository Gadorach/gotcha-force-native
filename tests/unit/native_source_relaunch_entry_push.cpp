#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"native relaunch-entry push: %s\n",why);std::abort();}}
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
    auto step=[&](NativeBorgMotionKind kind,f32 y,f32 speed){
        offer={observation,observation.position,observation.facing,BattleMvpAction::Jump,++serial,producer};
        offer.kind=kind;offer.proposed_position[1]=y;offer.vertical_speed=speed;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"accepted native falling phase candidate");
    };
    auto setup=[&](NativeBorgMotionPolicy policy,f32 old=-30){
        BattleMvpConfig config;config.spawn={BattlePoint{0,0},BattlePoint{400,400}};
        check(world.start(config,&terrain),"fresh static native plane");serial=0;
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"fresh same-world producer");
        step(NativeBorgMotionKind::JumpTakeoff,0,0);
        step(NativeBorgMotionKind::JumpAir,10,10);
        step(NativeBorgMotionKind::JumpAir,20,-10);
        step(NativeBorgMotionKind::JumpAir,10,old);
    };
    auto push=[&](f32 acceleration){
        NativeBorgStaticPush e;const f32 additive=f32(observation.vertical_speed+15);
        e.integrated_position=observation.position;e.integrated_position[1]=f32(observation.position[1]+additive);
        e.vertical_speed=std::clamp(f32(additive+acceleration),-30.f,18.f);e.damping=bits_to_f32(0x3f666666u);e.relaunch_acceleration=acceleration;
        check(terrain.static_surface(e.integrated_position[0],e.integrated_position[2],observation.position[1],&e.surface)&&
            source_surface_angle(e.surface.normal,&e.angle),"independent native selected plane");
        offer={observation,{},observation.facing,BattleMvpAction::Jump,serial+1,producer};
        offer.kind=NativeBorgMotionKind::RelaunchEntryPush;offer.entry_vertical_delta=15;offer.static_push=e;offer.vertical_speed=f32(e.vertical_speed*.9f);
        check(source_surface_push_position(e.integrated_position,e.surface.normal,e.surface.height,e.angle,&offer.proposed_position),"shared quantized projection");
    };
    const f32 neutral=bits_to_f32(0x3f4ccccdu);f32 cosine=0;source_heading_xz(8192,nullptr,&cosine);const f32 directional=std::fma(neutral,cosine,0.f);
    for(auto policy:{NativeBorgMotionPolicy::Relaunch,NativeBorgMotionPolicy::StaticPush,NativeBorgMotionPolicy::FallHistoryPush,
                     NativeBorgMotionPolicy::RecoveryPush,NativeBorgMotionPolicy::WindupPush,NativeBorgMotionPolicy::ActivePush,NativeBorgMotionPolicy::RelaunchPush}){
        setup(policy);push(neutral);const auto tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,"every earlier capability refuses additive entry projection");
    }
    setup(NativeBorgMotionPolicy::RelaunchEntryPush);push(neutral);const auto valid=offer;const auto tick=world.tick();
    auto refuses=[&](const NativeBorgMotionOffer& invalid,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&invalid)&&world.tick()==tick&&world.actors()[0].position==observation.position,why);
    };
    auto invalid=valid;invalid.entry_vertical_delta=0;refuses(invalid,"entry projection requires additive15");
    invalid=valid;invalid.entry_vertical_delta=14;refuses(invalid,"arbitrary declared entry delta cannot replace qualified source operand");
    invalid=valid;invalid.static_push->integrated_position[1]=f32(observation.position[1]+observation.vertical_speed);refuses(invalid,"entry displacement must include15 before integration");
    invalid=valid;invalid.static_push->vertical_speed=std::clamp(f32(observation.vertical_speed+neutral),-30.f,18.f);
    invalid.vertical_speed=f32(invalid.static_push->vertical_speed*.9f);refuses(invalid,"acceleration cannot omit additive entry velocity");
    invalid=valid;invalid.kind=NativeBorgMotionKind::RelaunchWindupPush;refuses(invalid,"continued relaunch cannot borrow entry delta/evidence");
    invalid=valid;invalid.kind=NativeBorgMotionKind::RelaunchEntry;refuses(invalid,"ordinary entry cannot borrow projection evidence");
    invalid=valid;invalid.static_push->relaunch_acceleration=-.07f;refuses(invalid,"wind-up acceleration cannot replace positive relaunch acceleration");
    invalid=valid;invalid.static_push->surface.triangle++;refuses(invalid,"spoofed plane refuses");
    invalid=valid;invalid.proposed_position[0]+=.001f;refuses(invalid,"edited projection refuses");
    terrain.metadata=false;refuses(valid,"height-only provider cannot certify entry projection");terrain.metadata=true;
    check(world.step({},false,&terrain,nullptr,nullptr,&valid)&&world.motion_receipt(valid,&receipt)&&!world.actors()[0].grounded,
        "entry adds before movement/acceleration then damps, remains airborne");
    invalid=valid;invalid.entry_vertical_delta=14;
    check(!world.motion_receipt(invalid,&receipt),"edited additive delta cannot borrow receipt");
    invalid=valid;invalid.static_push->relaunch_acceleration=directional;
    check(!world.motion_receipt(invalid,&receipt),"edited acceleration cannot borrow entry receipt");
    for(f32 acceleration:{neutral,directional}){
        setup(NativeBorgMotionPolicy::RelaunchEntryPush,-50);push(acceleration);
        check(offer.static_push->integrated_position[1]==-25&&offer.static_push->vertical_speed==-30&&offer.vertical_speed==-27,
            "displacement uses unclamped additive speed; lower clamp precedes .9 damping (native boundary fixture, not original trace)");
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"accepted bounded clamp-order fixture");
        serial=offer.serial;push(acceleration);const auto once=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==once,"cannot repeat additive entry from an already-entered relaunch");
    }
    terrain.lower=true;setup(NativeBorgMotionPolicy::RelaunchEntryPush);push(neutral);
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"entry push before continued motion");
    serial=offer.serial;
    step(NativeBorgMotionKind::RelaunchWindup,f32(observation.position[1]+observation.vertical_speed),f32(observation.vertical_speed+neutral));
    terrain.lower=true;setup(NativeBorgMotionPolicy::RelaunchEntryPush);push(neutral);
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"entry push before falling release");
    serial=offer.serial;
    step(NativeBorgMotionKind::JumpAir,f32(observation.position[1]+observation.vertical_speed),f32(observation.vertical_speed-1));
    std::printf("native relaunch-entry additive/projection/clamp/receipt: %u checks PASS; native proxy candidates, no whole-frame equivalence\n",checks);
}
