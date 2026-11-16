#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"wind-up entry push: %s\n",why);std::abort();}}
struct Plane:gf::IBattleMvpTerrain{
    bool metadata=true,blocked=false;
    bool height(float x,float,float,float* out)const override{*out=blocked?100:x>50?0:-3*x;return true;}
    bool static_surface(float x,float,float,gf::NativeBorgStaticSurface* out)const override{
        if(!metadata)return false;
        const float length=std::sqrt(10.f);*out={-3*x,{3/length,1/length,0},7};return true;
    }
};
}
int main(){
    using namespace gf;Plane terrain;NativeBattleMvp world;NativeBorgMotionObservation observation;
    NativeBorgMotionOffer offer;NativeBorgMotionReceipt receipt;std::shared_ptr<const NativeBorgMotionProducer> producer;u64 serial=0;
    auto step=[&](NativeBorgMotionKind kind,f32 y,f32 speed){
        offer={observation,observation.position,observation.facing,BattleMvpAction::Jump,++serial,producer};
        offer.kind=kind;offer.proposed_position[1]=y;offer.vertical_speed=speed;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"ordered native candidate");
    };
    auto setup=[&](NativeBorgMotionPolicy policy){
        BattleMvpConfig config;config.spawn={BattlePoint{0,0},BattlePoint{400,400}};
        check(world.start(config,&terrain),"fresh native plane world");serial=0;
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"same-world producer");
        step(NativeBorgMotionKind::JumpTakeoff,0,0);step(NativeBorgMotionKind::JumpAir,10,10);step(NativeBorgMotionKind::JumpAir,20,-20);
        NativeBorgStaticPush e;e.integrated_position=observation.position;e.integrated_position[1]=0;e.vertical_speed=-21;e.damping=bits_to_f32(0x3f666666u);
        check(terrain.static_surface(0,0,0,&e.surface)&&source_surface_angle(e.surface.normal,&e.angle),"selected steep floor");
        offer={observation,{},observation.facing,BattleMvpAction::Jump,++serial,producer};offer.kind=NativeBorgMotionKind::StaticPush;
        offer.static_push=e;offer.vertical_speed=f32(e.vertical_speed*e.damping);
        check(source_surface_push_position(e.integrated_position,e.surface.normal,e.surface.height,e.angle,&offer.proposed_position)&&
            world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation)&&!world.actors()[0].grounded,
            "ordinary falling ret0 reaches the steep plane without a fabricated landing");
    };
    auto entry=[&](){
        NativeBorgStaticPush e;e.integrated_position=observation.position;e.vertical_speed=f32(observation.vertical_speed*.5f);e.damping=bits_to_f32(0x3f666666u);
        check(terrain.static_surface(observation.position[0],observation.position[2],observation.position[1],&e.surface)&&
            source_surface_angle(e.surface.normal,&e.angle),"independent entry surface");
        offer={observation,{},observation.facing,BattleMvpAction::AirDash,serial+1,producer};offer.kind=NativeBorgMotionKind::AirDashWindupEntryPush;
        offer.static_push=e;offer.vertical_speed=f32(e.vertical_speed*e.damping);
        check(source_surface_push_position(e.integrated_position,e.surface.normal,e.surface.height,e.angle,&offer.proposed_position),"entry projection");
    };
    for(auto policy:{NativeBorgMotionPolicy::StaticPush,NativeBorgMotionPolicy::FallHistoryPush,NativeBorgMotionPolicy::RecoveryPush,
                     NativeBorgMotionPolicy::WindupPush,NativeBorgMotionPolicy::ActivePush,NativeBorgMotionPolicy::RelaunchPush,
                     NativeBorgMotionPolicy::RelaunchEntryPush,NativeBorgMotionPolicy::RelaunchFloor,NativeBorgMotionPolicy::AirDashFloor,
                     NativeBorgMotionPolicy::WindupEntryFloor,NativeBorgMotionPolicy::WindupPositiveFloor}){
        setup(policy);entry();const auto tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,"all older push/floor policies refuse entry-push authority");
    }
    setup(NativeBorgMotionPolicy::WindupEntryPush);entry();const auto valid=offer;const auto tick=world.tick();
    auto refuses=[&](NativeBorgMotionOffer candidate,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&candidate)&&world.tick()==tick&&world.actors()[0].position==observation.position,why);
    };
    auto invalid=valid;invalid.static_push.reset();refuses(invalid,"entry push requires exact projection evidence");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashWindupEntry;refuses(invalid,"ordinary entry cannot borrow push evidence");
    invalid.static_push.reset();refuses(invalid,"ordinary entry retains its strict above-floor check");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashWindupPush;refuses(invalid,"entry cannot masquerade as continued wind-up");
    invalid=valid;invalid.static_push->vertical_speed+=bits_to_f32(0xbd8f5c29u);refuses(invalid,"entry does not apply future50 acceleration");
    invalid=valid;invalid.static_push->vertical_speed=observation.vertical_speed;refuses(invalid,"entry halves old speed before contact");
    invalid=valid;invalid.static_push->integrated_position[1]+=valid.static_push->vertical_speed;
    refuses(invalid,"entry does not translate by its half-speed");
    invalid=valid;invalid.static_push->damping=.5f;refuses(invalid,"entry factor cannot replace steep .9 response");
    invalid=valid;invalid.static_push->damping=.8f;refuses(invalid,"relaunch factor cannot replace steep response");
    invalid=valid;invalid.vertical_speed=0;refuses(invalid,"steep ret0 does not trigger wind-up ret1 zero48");
    invalid=valid;invalid.static_push->surface.triangle++;refuses(invalid,"selected surface independently matches");
    invalid=valid;invalid.proposed_position[0]+=.001f;refuses(invalid,"projected position is exact");
    invalid=valid;invalid.static_push->relaunch_acceleration=.8f;refuses(invalid,"entry cannot borrow relaunch operands");
    invalid=valid;invalid.entry_vertical_delta=15;refuses(invalid,"entry cannot add relaunch speed");
    terrain.metadata=false;refuses(valid,"height-only provider cannot authorize entry projection");terrain.metadata=true;
    terrain.blocked=true;refuses(valid,"native terrain clipping remains a refusal");terrain.blocked=false;
    offer=valid;
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt)&&world.observe_motion(&observation)&&
        !world.actors()[0].grounded&&observation.vertical_speed==f32(f32(valid.observed.vertical_speed*.5f)*bits_to_f32(0x3f666666u)),
        "entry half-speed then .9 remains airborne, not zero48, landing or refill");
    serial=offer.serial;
    invalid=offer;invalid.kind=NativeBorgMotionKind::AirDashWindupPush;
    check(!world.motion_receipt(invalid,&receipt),"continued wind-up cannot borrow entry receipt");
    NativeBorgStaticPush e;e.integrated_position=observation.position;e.integrated_position[1]=f32(observation.position[1]+observation.vertical_speed);
    e.vertical_speed=std::max(-30.f,f32(observation.vertical_speed+bits_to_f32(0xbd8f5c29u)));e.damping=bits_to_f32(0x3f666666u);
    check(terrain.static_surface(e.integrated_position[0],e.integrated_position[2],observation.position[1],&e.surface)&&
        source_surface_angle(e.surface.normal,&e.angle),"next wind-up surface");
    offer={observation,{},observation.facing,BattleMvpAction::AirDash,serial+1,producer};offer.kind=NativeBorgMotionKind::AirDashWindupPush;
    offer.static_push=e;offer.vertical_speed=f32(e.vertical_speed*e.damping);
    check(source_surface_push_position(e.integrated_position,e.surface.normal,e.surface.height,e.angle,&offer.proposed_position)&&
        world.step({},false,&terrain,nullptr,nullptr,&offer),"continued integration/acceleration follows distinct entry push");
    std::printf("native wind-up entry half-speed/steep ret0/receipt: %u checks PASS; numeric native proxy, not original actor execution\n",checks);
}
